#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "LlmClient.h"
#include "SimulationRunner.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void onSelectPdfClicked();
  void onGeneratePlanClicked();
  void onGenerateCodeClicked();
  void onRunSimulationClicked();
  void onLlmResponseReceived(const QString &response, const QString &type);
  void onLlmErrorOccurred(const QString &err);
  void onLlmUploadProgress(qint64 bytesSent, qint64 bytesTotal);
  void onLlmDiagnosticLog(const QString &msg);
  void onApiTimerTick();
  void onSimulationOutputReceived(const QString &output);
  void onSimulationErrorReceived(const QString &error);
  void onScpFinished(int exitCode);
  void onSshFinished(int exitCode);
  void handleSimulationLoop(int exitCode);

private:
  void setupUi();
  void createToolbar();
  void createCentralWidget();
  void createStatusBar();
  void configureSettingsWidget();

  // UI Elements
  QTextEdit *specInputTextEdit;
  QPushButton *btnSelectPdf;
  QLabel *pdfPathLabel;
  QString selectedPdfPath;

  QTextEdit *testPlanTextEdit;
  QTextEdit *generatedCodeTextEdit;
  QTextEdit *simulationLogTextEdit;
  QTextEdit *finalReportTextEdit;

  QTabWidget *mainTabWidget;
  QPushButton *btnGeneratePlan;
  QPushButton *btnGenerateCode;
  QPushButton *btnRunSimulation;

  // Config inputs
  QComboBox *llmProviderCombo;
  QLineEdit *llmApiKeyInput;
  QLineEdit *sshHostInput;
  QLineEdit *sshKeyInput;
  QLineEdit *remoteDirInput;
  QCheckBox *autoImproveCheckbox;

  // Core Logic
  LlmClient *llmClient;
  SimulationRunner *simulationRunner;
  QTimer *apiWaitTimer;
  int apiElapsedSeconds;
  int lastUploadLogPercent = -1;

  // Feedback Loop State
  enum class SimState { Idle, RunningSimulation, WaitingForLlm, WaitingForReport };
  SimState currentSimState = SimState::Idle;
  QString lastGeneratedCode;
};

#endif // MAINWINDOW_H
