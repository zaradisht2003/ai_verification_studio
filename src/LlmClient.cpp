#include "LlmClient.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

LlmClient::LlmClient(QObject *parent)
    : QObject(parent), manager(new QNetworkAccessManager(this)),
      activeReply(nullptr), isTimeout(false) {
  connect(manager, &QNetworkAccessManager::finished, this,
          &LlmClient::onReplyFinished);

  timeoutTimer = new QTimer(this);
  timeoutTimer->setSingleShot(true);
  connect(timeoutTimer, &QTimer::timeout, this, &LlmClient::onTimeout);

  isEarlyCompletion = false;
}

LlmClient::~LlmClient() {
  if (activeReply) {
    activeReply->abort();
    activeReply->deleteLater();
  }
}

void LlmClient::setApiKey(const QString &key) { apiKey = key; }

void LlmClient::setModel(LlmClient::Model m) { currentModel = m; }

void LlmClient::generateTestPlan(const QString &specificationText) {
  QString systemInstruction =
      "You are an expert SystemVerilog verification engineer. "
      "Read the hardware specification and output a detailed JSON-formatted "
      "test plan. "
      "Include 'module_name', 'inputs', 'outputs', and an array of 'tests' "
      "(each with name, type, and description). "
      "Output ONLY valid JSON without markdown wrapping.";

  sendRequest(specificationText, systemInstruction, "plan");
}

void LlmClient::generateTestPlanFromPdf(const QString &pdfFilePath,
                                        const QString &additionalPrompt) {
  QString systemInstruction =
      "You are an expert SystemVerilog verification engineer. "
      "Read the provided hardware specification document (and any additional "
      "notes) "
      "and output a detailed JSON-formatted test plan. "
      "Pay attention to any diagrams or tables in the document. "
      "Include 'module_name', 'inputs', 'outputs', and an array of 'tests' "
      "(each with name, type, and description). "
      "Output ONLY valid JSON without markdown wrapping.";

  QString prompt = "Please analyze this specification document.";
  if (!additionalPrompt.isEmpty()) {
    prompt += "\n\nAdditional notes from user:\n" + additionalPrompt;
  }

  currentResponseType = "plan";

  if (apiKey.isEmpty()) {
    emit errorOccurred("API Key is missing.");
    return;
  }

  QUrl url;
  QNetworkRequest request;

  QString modelName = "gemini-3-flash-preview";
  if (currentModel == Model::Gemini2_5_Flash) {
      modelName = "gemini-2.5-flash";
  } else if (currentModel == Model::Gemini2_5_Flash_Lite) {
      modelName = "gemini-2.5-flash-lite";
  } else if (currentModel == Model::Gemini3_1_Flash_Lite) {
      modelName = "gemini-3-1-flash-lite-preview";
  }

  QString apiVersion = modelName.contains("gemini-3") ? "v1alpha" : "v1beta";
  url = QUrl(QString("https://generativelanguage.googleapis.com/%1/models/%2:generateContent?key=%3")
                 .arg(apiVersion).arg(modelName).arg(apiKey));
  request.setUrl(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  currentRequest = request;

  QFutureWatcher<QByteArray> *watcher = new QFutureWatcher<QByteArray>(this);
  connect(watcher, &QFutureWatcher<QByteArray>::finished, this,
          [this, watcher]() {
            QByteArray payload = watcher->future().result();
            if (payload.isEmpty()) {
              emit errorOccurred("Failed to read or encode PDF document.");
            } else {
              this->onPayloadReady(payload);
            }
            watcher->deleteLater();
          });

  QFuture<QByteArray> future = QtConcurrent::run(LlmClient::buildGeminiMultimodalPayload,
                               systemInstruction, prompt, pdfFilePath);
  watcher->setFuture(future);
}


QByteArray
LlmClient::buildGeminiMultimodalPayload(const QString &systemInstruction,
                                        const QString &prompt,
                                        const QString &pdfFilePath) {
  QFile file(pdfFilePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return QByteArray();
  }

  QByteArray fileData = file.readAll();
  QString base64Data = fileData.toBase64();
  file.close();

  QJsonObject requestBody;

  // System Instruction
  QJsonObject sysInstObj;
  QJsonArray sysInstParts;
  QJsonObject sysInstPart;
  sysInstPart["text"] = systemInstruction;
  sysInstParts.append(sysInstPart);
  sysInstObj["parts"] = sysInstParts;
  requestBody["systemInstruction"] = sysInstObj;

  // Generation Config (JSON enforce)
  QJsonObject genConfig;
  genConfig["temperature"] = 0.2;
  genConfig["responseMimeType"] = "application/json";
  requestBody["generationConfig"] = genConfig;

  // Contents
  QJsonArray contentsArray;
  QJsonObject contentObj;
  contentObj["role"] = "user";

  QJsonArray partsArray;

  // Prompt text
  QJsonObject textPart;
  textPart["text"] = prompt;
  partsArray.append(textPart);

  // Inline PDF
  QJsonObject inlineDataPart;
  QJsonObject inlineDataObj;
  inlineDataObj["mimeType"] = "application/pdf";
  inlineDataObj["data"] = base64Data;
  inlineDataPart["inlineData"] = inlineDataObj;
  partsArray.append(inlineDataPart);

  contentObj["parts"] = partsArray;
  contentsArray.append(contentObj);
  requestBody["contents"] = contentsArray;

  QJsonDocument doc(requestBody);
  return doc.toJson(QJsonDocument::Compact);
}

void LlmClient::generateSystemVerilog(const QString &testPlanJson) {
  QString systemInstruction =
      "You are a senior SystemVerilog verification engineer. "
      "Given the structured test plan, generate a complete SystemVerilog "
      "testbench. "
      "Include a clock generator, reset logic, basic scoreboard, and "
      "assertions. "
      "Output ONLY the raw SystemVerilog code without markdown formatting (no "
      "```sv ... ```).";

  sendRequest(testPlanJson, systemInstruction, "code");
}

void LlmClient::improveSystemVerilog(const QString &currentCode,
                                     const QString &simLogs,
                                     const QString &additionalPrompt) {
  QString systemInstruction =
      "You are a senior SystemVerilog verification engineer. "
      "You will be provided with the current testbench code and the simulation "
      "output log. "
      "Your task is to analyze the simulation errors, identify failed test cases, and "
      "modify the testbench to fix them and ensure all test cases pass. "
      "Provide the complete, updated SystemVerilog code. "
      "Output ONLY the raw SystemVerilog code without markdown formatting (no "
      "```sv ... ```).";

  QString prompt =
      QString("Simulation Logs:\n%1\n\nCurrent Testbench Code:\n%2")
          .arg(simLogs)
          .arg(currentCode);

  if (!additionalPrompt.isEmpty()) {
      prompt += "\n\nUser Comments:\n" + additionalPrompt;
  }

  sendRequest(prompt, systemInstruction, "code");
}

void LlmClient::generateDesignReport(const QString &currentCode,
                                     const QString &simLogs,
                                     const QString &additionalPrompt) {
  QString systemInstruction =
      "You are a hardware verification expert. "
      "Analyze the provided testbench code and the successful simulation logs. "
      "Write a markdown report explaining if there are any issues in the design (RTL) revealed by the testbench, "
      "what the possible reasons for failed or edge cases might be, and any design recommendations. "
      "Output ONLY the textual markdown report.";

  QString prompt =
      QString("Simulation Logs:\n%1\n\nCurrent Testbench Code:\n%2")
          .arg(simLogs)
          .arg(currentCode);

  if (!additionalPrompt.isEmpty()) {
      prompt += "\n\nUser Comments:\n" + additionalPrompt;
  }

  sendRequest(prompt, systemInstruction, "report");
}

void LlmClient::sendRequest(const QString &prompt,
                            const QString &systemInstruction,
                            const QString &responseType) {
  currentResponseType = responseType;

  if (apiKey.isEmpty()) {
    emit errorOccurred("API Key is missing.");
    return;
  }

  QUrl url;
  QNetworkRequest request;

  QString modelName = "gemini-3-flash-preview";
  if (currentModel == Model::Gemini2_5_Flash) {
      modelName = "gemini-2.5-flash";
  } else if (currentModel == Model::Gemini2_5_Flash_Lite) {
      modelName = "gemini-2.5-flash-lite";
  } else if (currentModel == Model::Gemini3_1_Flash_Lite) {
      modelName = "gemini-3-1-flash-lite-preview";
  }

  QString apiVersion = modelName.contains("gemini-3") ? "v1alpha" : "v1beta";
  url = QUrl(QString("https://generativelanguage.googleapis.com/%1/models/%2:generateContent?key=%3")
                 .arg(apiVersion).arg(modelName).arg(apiKey));
  request.setUrl(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  currentRequest = request;

  QFutureWatcher<QByteArray> *watcher = new QFutureWatcher<QByteArray>(this);
  connect(watcher, &QFutureWatcher<QByteArray>::finished, this,
          [this, watcher]() {
            QByteArray payload = watcher->future().result();
            this->onPayloadReady(payload);
            watcher->deleteLater();
          });

  QFuture<QByteArray> future = QtConcurrent::run(LlmClient::buildGeminiTextPayload,
                             systemInstruction, prompt);
  watcher->setFuture(future);
}

void LlmClient::onPayloadReady(const QByteArray &payload) {
  emit diagnosticLog(
      QString("Payload ready in background. Size: %1 bytes (approx %2 MB)")
          .arg(payload.size())
          .arg(payload.size() / 1024.0 / 1024.0, 0, 'f', 2));

  // If payload is small enough, log its preview
  if (payload.size() < 10000) {
    emit diagnosticLog(
        QString("Raw Request Payload:\n%1").arg(QString::fromUtf8(payload)));
  } else {
    emit diagnosticLog(
        QString("Raw Request Payload omitted because it is larger than 10KB."));
  }

  // Write raw request to file in absolute path
  QString reqLogPath =
      QDir(QCoreApplication::applicationDirPath()).filePath("api_requests.log");
  QFile reqFile(reqLogPath);
  if (reqFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
    reqFile.write("=== NEW REQUEST ===\n");
    reqFile.write(payload);
    reqFile.write("\n===================\n\n");
    reqFile.close();
    emit diagnosticLog(QString("Saved request payload to: %1").arg(reqLogPath));
  }

  currentRequest.setHeader(QNetworkRequest::ContentLengthHeader,
                           payload.size());
  isTimeout = false;
  isEarlyCompletion = false;
  accumulatedResponse.clear();

  QString respLogPath = QDir(QCoreApplication::applicationDirPath())
                            .filePath("api_responses.log");
  QFile respFileInit(respLogPath);
  if (respFileInit.open(QIODevice::WriteOnly)) {
    respFileInit.write("=== NEW STREAM RESPONSE ===\n");
    respFileInit.close();
  }

  if (timeoutTimer) {
    timeoutTimer->start(300000); // 5 minutes for long generations
  }

  activeReply = manager->post(currentRequest, payload);

  // CRITICAL FIX: The user required 'curl -k' (Insecure SSL). This bypasses
  // MITM corporate proxies or missing Windows root certs. We must emulate '-k'
  // by explicitly instructing Qt to ignore SSL handshake verification errors,
  // or the socket immediately drops silently.
  activeReply->ignoreSslErrors();

  connect(activeReply, &QNetworkReply::readyRead, this, [this]() {
    if (!activeReply)
      return;
    QByteArray chunk = activeReply->readAll();
    accumulatedResponse.append(chunk);

    QString respLogPath = QDir(QCoreApplication::applicationDirPath())
                              .filePath("api_responses.log");
    QFile respFile(respLogPath);
    if (respFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
      respFile.write(chunk);
      respFile.close();
    }

    // Attempt to parse the stream live. If OpenRouter returned a perfect JSON
    // object but refused to close the socket, this will catch it instantly and
    // sever the hanging connection.
    QByteArray cleaned = accumulatedResponse.trimmed();
    if (!cleaned.isEmpty() && cleaned.startsWith('{') &&
        cleaned.endsWith('}')) {
      QJsonParseError parseError;
      QJsonDocument jsonDoc = QJsonDocument::fromJson(cleaned, &parseError);

      if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
        QJsonObject jsonObject = jsonDoc.object();

        bool foundCompleteText = false;
        QString textResponse;

          if (jsonObject.contains("candidates")) {
            QJsonArray candidates = jsonObject["candidates"].toArray();
            if (!candidates.isEmpty()) {
              QJsonObject firstCandidate = candidates.first().toObject();
              if (firstCandidate.contains("content")) {
                QJsonObject content = firstCandidate["content"].toObject();
                if (content.contains("parts")) {
                  QJsonArray parts = content["parts"].toArray();
                  if (!parts.isEmpty()) {
                    textResponse = parts.first().toObject()["text"].toString();
                    foundCompleteText = true;
                  }
                }
              }
            }
          }

        if (foundCompleteText) {
          emit diagnosticLog("Valid complete JSON captured from live stream! "
                             "Force-closing hanging socket.");

          // Stop timer and flag success
          if (timeoutTimer)
            timeoutTimer->stop();
          isEarlyCompletion = true;

          emit responseReceived(textResponse.trimmed(), currentResponseType);

          // Abort the socket to unlock the engine. This fires onReplyFinished
          // with an OperationCanceledError, but our isEarlyCompletion flag will
          // tell it to ignore the error.
          activeReply->abort();
        }
      }
    }
  });

  connect(activeReply, &QNetworkReply::uploadProgress, this,
          [this](qint64 bytesSent, qint64 bytesTotal) {
            if (bytesTotal > 0) {
              emit uploadProgressChanged(bytesSent, bytesTotal);
            }
          });
}


QByteArray LlmClient::buildGeminiTextPayload(const QString &systemInstruction,
                                             const QString &prompt) {
  QJsonObject requestBody;

  // System Instruction
  QJsonObject sysInstObj;
  QJsonArray sysInstParts;
  QJsonObject sysInstPart;
  sysInstPart["text"] = systemInstruction;
  sysInstParts.append(sysInstPart);
  sysInstObj["parts"] = sysInstParts;
  requestBody["systemInstruction"] = sysInstObj;

  // Generation Config
  QJsonObject genConfig;
  genConfig["temperature"] = 0.2;
  // If we're generating SystemVerilog code, Gemini prefers plain text, but for
  // the plan we asked for JSON. Actually, LlmClient always enforces JSON
  // natively on OpenRouter or relies on raw output. For safety, we allow it. If
  // it's pure SV, returning JSON could break. We'll omit responseMimeType for
  // text to let it return raw string for SV code, but if you requested JSON in
  // the prompt, Gemini can handle it.
  requestBody["generationConfig"] = genConfig;

  // Contents
  QJsonArray contentsArray;
  QJsonObject contentObj;
  contentObj["role"] = "user";

  QJsonArray partsArray;
  QJsonObject textPart;
  textPart["text"] = prompt;
  partsArray.append(textPart);

  contentObj["parts"] = partsArray;
  contentsArray.append(contentObj);
  requestBody["contents"] = contentsArray;

  QJsonDocument doc(requestBody);
  return doc.toJson(QJsonDocument::Compact);
}

void LlmClient::onTimeout() {
  if (activeReply) {
    isTimeout = true;
    emit diagnosticLog("Hard timeout reached (5 minutes). Aborting request.");
    activeReply->abort();
    // The abort will trigger onReplyFinished with an OperationCanceledError
  }
}

void LlmClient::onReplyFinished() {
  timeoutTimer->stop();
  activeReply = nullptr;

  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply)
    return;

  // If we already successfully parsed the JSON from the live stream and
  // manually aborted it, skip all error handling here.
  if (isEarlyCompletion) {
    reply->deleteLater();
    return;
  }

  if (isTimeout) {
    emit errorOccurred(
        "Request Timed Out after 5 minutes.\nAPI "
        "did not close the connection in time or the generation was too long.");
    reply->deleteLater();
    return;
  }

  int httpStatusCode = 0;
  QVariant statusCode =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
  if (statusCode.isValid()) {
    httpStatusCode = statusCode.toInt();
    emit diagnosticLog(
        QString("Received HTTP Response Code: %1").arg(httpStatusCode));
  }

  QByteArray responseData = accumulatedResponse;

  // Log termination line for the live stream file
  QString respLogPath = QDir(QCoreApplication::applicationDirPath())
                            .filePath("api_responses.log");
  QFile respFile(respLogPath);
  if (respFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
    respFile.write(
        QString(
            "\n=== RESPONSE FINISHED (HTTP %1) ===\n======================\n\n")
            .arg(httpStatusCode)
            .toUtf8());
    respFile.close();
    emit diagnosticLog(
        QString("Finalized raw response to: %1").arg(respLogPath));
  }

  QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
  QJsonObject jsonObject = jsonResponse.object();

  // Dump the raw JSON if available, or just raw bytes
  if (!responseData.isEmpty()) {
    emit diagnosticLog(QString("Raw API Response Snippet:\n%1...")
                           .arg(QString::fromUtf8(responseData).left(2000)));
  }

  if (reply->error() != QNetworkReply::NoError || httpStatusCode >= 400) {
    emit diagnosticLog(
        QString("Network Error Detail: %1").arg(reply->errorString()));

    QString explicitError = reply->errorString();

    // Check if OpenRouter/Gemini embedded an explicit JSON error object
    if (jsonObject.contains("error")) {
      QJsonObject errObj = jsonObject["error"].toObject();
      QString apiMessage = errObj["message"].toString();
      QString apiStatus =
          errObj.contains("status") ? errObj["status"].toString() : "Error";
      int apiCode =
          errObj.contains("code") ? errObj["code"].toInt() : httpStatusCode;

      if (!apiMessage.isEmpty()) {
        explicitError = QString("API Error [%1 / %2]: %3")
                            .arg(apiStatus)
                            .arg(apiCode)
                            .arg(apiMessage);
      }
    } else if (httpStatusCode > 0) {
      explicitError =
          QString("HTTP %1: %2").arg(httpStatusCode).arg(reply->errorString());
      if (httpStatusCode == 429) {
        explicitError += "\n\nToo Many Requests. "
                         "Please wait or check your credit balance/quotas.";
      }
    }

    emit errorOccurred(QString("Network Error: %1").arg(explicitError));
    reply->deleteLater();
    return;
  }

  bool parsedSuccessfully = false;
  QString textResponse;

  if (jsonObject.contains("candidates") &&
      jsonObject["candidates"].isArray()) {
    QJsonArray candidates = jsonObject["candidates"].toArray();
    if (!candidates.isEmpty()) {
      QJsonObject firstCandidate = candidates.first().toObject();
      if (firstCandidate.contains("content")) {
        QJsonObject content = firstCandidate["content"].toObject();
        if (content.contains("parts") && content["parts"].isArray()) {
          QJsonArray parts = content["parts"].toArray();
          if (!parts.isEmpty()) {
            textResponse = parts.first().toObject()["text"].toString();
            parsedSuccessfully = true;
          }
        }
      }
    }
  }

  if (parsedSuccessfully) {
    emit responseReceived(textResponse.trimmed(), currentResponseType);
  } else {
    // Specifically catch chunked stream formats
    if (responseData.trimmed().startsWith("data: {") ||
        responseData.trimmed().startsWith("id: gen-")) {
      emit errorOccurred(
          "Application Error: Server returned a server-sent stream (SSE), "
          "which the app cannot parse. The raw response was logged to "
          "api_responses.log for review.");
    } else {
      emit errorOccurred("Unexpected JSON structure from API.");
    }
  }

  reply->deleteLater();
}

