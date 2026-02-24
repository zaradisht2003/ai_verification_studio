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
  explicit LlmClient(QObject *parent = nullptr);
  ~LlmClient();

  void setApiKey(const QString &key);

  // Sends a prompt to generate a JSON test plan
  void generateTestPlan(const QString &specificationText);
  void generateTestPlanFromPdf(const QString &pdfFilePath,
                               const QString &additionalPrompt = "");

  // Sends a test plan to generate SystemVerilog code
  void generateSystemVerilog(const QString &testPlanJson);

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
  static QByteArray buildMultimodalPayload(const QString &systemInstruction,
                                           const QString &prompt,
                                           const QString &pdfFilePath);
  static QByteArray buildTextPayload(const QString &systemInstruction,
                                     const QString &prompt);

  QString apiKey;
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
