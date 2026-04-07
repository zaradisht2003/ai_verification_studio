#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtConcurrent>

class LlmClient : public QObject {
  Q_OBJECT

public:
  enum class Model { Gemini2_5_Flash, Gemini2_5_Flash_Lite, Gemini3, Gemini3_1_Flash_Lite };

  explicit LlmClient(QObject *parent = nullptr);
  ~LlmClient();

  void setApiKey(const QString &key);
  void setModel(Model m);

  // Sends a prompt to generate a JSON test plan
  void generateTestPlan(const QString &specificationText);
  void generateTestPlanFromPdf(const QString &pdfFilePath,
                               const QString &additionalPrompt = "");

  // Sends a test plan to generate SystemVerilog code
  void generateSystemVerilog(const QString &testPlanJson);

  // Sends current code and simulation logs to improve the testbench and fix errors
  void improveSystemVerilog(const QString &currentCode, const QString &simLogs, const QString &additionalPrompt = "");

  // Sends current code and simulation logs to generate a final design report
  void generateDesignReport(const QString &currentCode, const QString &simLogs, const QString &additionalPrompt = "");

signals:
  void responseReceived(const QString &response, const QString &type);
  void errorOccurred(const QString &errorString);
  void uploadProgressChanged(qint64 bytesSent, qint64 bytesTotal);
  void diagnosticLog(const QString &logMessage);

private slots:
  void onReplyFinished();
  void onPayloadReady(const QByteArray &payload);
  void onTimeout();

private:
  void sendRequest(const QString &prompt, const QString &systemInstruction,
                   const QString &responseType);

  static QByteArray
  buildGeminiMultimodalPayload(const QString &systemInstruction,
                               const QString &prompt,
                               const QString &pdfFilePath);
  static QByteArray buildGeminiTextPayload(const QString &systemInstruction,
                                           const QString &prompt);

  QString apiKey;
  Model currentModel = Model::Gemini3;
  QNetworkAccessManager *manager;
  QString currentResponseType;
  QNetworkRequest currentRequest;
  QTimer *timeoutTimer;
  QNetworkReply *activeReply;
  bool isTimeout;
  bool isEarlyCompletion;
  QByteArray accumulatedResponse;
};

#endif // LLMCLIENT_H
