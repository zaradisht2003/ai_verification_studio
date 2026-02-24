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

  QUrl url("https://openrouter.ai/api/v1/chat/completions");
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setRawHeader("Authorization",
                       QString("Bearer %1").arg(apiKey).toUtf8());
  request.setRawHeader("HTTP-Referer", "http://ai-verification-studio.local");
  request.setRawHeader("X-Title", "AI Verification Studio");
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

  QFuture<QByteArray> future =
      QtConcurrent::run(LlmClient::buildMultimodalPayload, systemInstruction,
                        prompt, pdfFilePath);
  watcher->setFuture(future);
}

QByteArray LlmClient::buildMultimodalPayload(const QString &systemInstruction,
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

  // Explicitly ask OpenRouter for Gemini 2.5 Flash as it supports PDF over
  // base64
  requestBody["model"] = "google/gemini-2.5-flash";

  QJsonObject responseFormatObj;
  responseFormatObj["type"] = "json_object";
  requestBody["response_format"] = responseFormatObj;

  requestBody["max_tokens"] = 8192;

  QJsonArray messagesArray;

  // System message
  QJsonObject systemMessage;
  systemMessage["role"] = "system";
  systemMessage["content"] = systemInstruction;
  messagesArray.append(systemMessage);

  // User message with multimodal inline data
  QJsonObject userMessage;
  userMessage["role"] = "user";

  QJsonArray contentArray;

  // The text prompt part
  QJsonObject textPart;
  textPart["type"] = "text";
  textPart["text"] = prompt;
  contentArray.append(textPart);

  // The inline PDF part (formatted for OpenAI image_url standard)
  QJsonObject imagePart;
  imagePart["type"] = "image_url";
  QJsonObject imageUrlObj;
  // Construct the data URI for the PDF
  imageUrlObj["url"] = "data:application/pdf;base64," + base64Data;
  imagePart["image_url"] = imageUrlObj;
  contentArray.append(imagePart);

  userMessage["content"] = contentArray;
  messagesArray.append(userMessage);

  requestBody["messages"] = messagesArray;

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

void LlmClient::sendRequest(const QString &prompt,
                            const QString &systemInstruction,
                            const QString &responseType) {
  currentResponseType = responseType;

  if (apiKey.isEmpty()) {
    emit errorOccurred("API Key is missing.");
    return;
  }

  QUrl url("https://openrouter.ai/api/v1/chat/completions");
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setRawHeader("Authorization",
                       QString("Bearer %1").arg(apiKey).toUtf8());
  request.setRawHeader("HTTP-Referer", "http://ai-verification-studio.local");
  request.setRawHeader("X-Title", "AI Verification Studio");

  currentRequest = request;

  QFutureWatcher<QByteArray> *watcher = new QFutureWatcher<QByteArray>(this);
  connect(watcher, &QFutureWatcher<QByteArray>::finished, this,
          [this, watcher]() {
            QByteArray payload = watcher->future().result();
            this->onPayloadReady(payload);
            watcher->deleteLater();
          });

  QFuture<QByteArray> future =
      QtConcurrent::run(LlmClient::buildTextPayload, systemInstruction, prompt);
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
        if (jsonObject.contains("choices")) {
          emit diagnosticLog("Valid complete JSON captured from live stream! "
                             "Force-closing hanging socket.");

          // Stop timer and flag success
          if (timeoutTimer)
            timeoutTimer->stop();
          isEarlyCompletion = true;

          QJsonArray choices = jsonObject["choices"].toArray();
          if (!choices.isEmpty()) {
            QJsonObject firstChoice = choices.first().toObject();
            if (firstChoice.contains("message")) {
              QString textResponse =
                  firstChoice["message"].toObject()["content"].toString();
              emit responseReceived(textResponse.trimmed(),
                                    currentResponseType);
            }
          }

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

QByteArray LlmClient::buildTextPayload(const QString &systemInstruction,
                                       const QString &prompt) {
  QJsonObject requestBody;

  // Use a fast/default model on OpenRouter for pure text (e.g., Llama 3 or
  // Gemini)
  requestBody["model"] = "meta-llama/llama-3.3-70b-instruct";

  QJsonObject responseFormatObj;
  responseFormatObj["type"] = "json_object";
  requestBody["response_format"] = responseFormatObj;

  requestBody["max_tokens"] = 8192;

  QJsonArray messagesArray;

  QJsonObject systemMessage;
  systemMessage["role"] = "system";
  systemMessage["content"] = systemInstruction;
  messagesArray.append(systemMessage);

  QJsonObject userMessage;
  userMessage["role"] = "user";
  userMessage["content"] = prompt;
  messagesArray.append(userMessage);

  requestBody["messages"] = messagesArray;

  QJsonDocument doc(requestBody);
  return doc.toJson(QJsonDocument::Compact);
}

void LlmClient::onTimeout() {
  if (activeReply) {
    isTimeout = true;
    emit diagnosticLog("Hard timeout reached (90s). Aborting request.");
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
        "Request Timed Out after 5 minutes.\nOpenRouter API "
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

    // Check if OpenRouter embedded an explicit JSON error object
    if (jsonObject.contains("error")) {
      QJsonObject errObj = jsonObject["error"].toObject();
      QString apiMessage = errObj["message"].toString();
      QString apiStatus = errObj["status"].toString();
      int apiCode = errObj["code"].toInt();
      if (!apiMessage.isEmpty()) {
        explicitError = QString("OpenRouter API Error [%1 / %2]: %3")
                            .arg(apiStatus)
                            .arg(apiCode)
                            .arg(apiMessage);
      }
    } else if (httpStatusCode > 0) {
      explicitError =
          QString("HTTP %1: %2").arg(httpStatusCode).arg(reply->errorString());
      if (httpStatusCode == 429) {
        explicitError += "\n\nOpenRouter reports 'Too Many Requests'. "
                         "Please wait or check your credit balance.";
      }
    }

    emit errorOccurred(QString("Network Error: %1").arg(explicitError));
    reply->deleteLater();
    return;
  }

  if (jsonObject.contains("choices") && jsonObject["choices"].isArray()) {
    QJsonArray choices = jsonObject["choices"].toArray();
    if (!choices.isEmpty()) {
      QJsonObject firstChoice = choices.first().toObject();
      if (firstChoice.contains("message")) {
        QJsonObject message = firstChoice["message"].toObject();
        QString textResponse = message["content"].toString();
        emit responseReceived(textResponse.trimmed(), currentResponseType);
      }
    }
  } else {
    // Specifically catch chunked stream formats that OpenRouter failed to
    // disable
    if (responseData.trimmed().startsWith("data: {") ||
        responseData.trimmed().startsWith("id: gen-")) {
      emit errorOccurred(
          "Application Error: OpenRouter returned a server-sent stream (SSE), "
          "which the app cannot parse. The raw response was logged to "
          "api_responses.log for review.");
    } else {
      emit errorOccurred("Unexpected JSON structure from OpenRouter API.");
    }
  }

  reply->deleteLater();
}
