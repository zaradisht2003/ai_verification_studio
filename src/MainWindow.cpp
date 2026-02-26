#include "MainWindow.h"
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), llmClient(new LlmClient(this)),
      simulationRunner(new SimulationRunner(this)) {
  setupUi();

  // Connect LLM Client signals
  connect(llmClient, &LlmClient::responseReceived, this,
          &MainWindow::onLlmResponseReceived);
  connect(llmClient, &LlmClient::errorOccurred, this,
          &MainWindow::onLlmErrorOccurred);
  connect(llmClient, &LlmClient::uploadProgressChanged, this,
          &MainWindow::onLlmUploadProgress);
  connect(llmClient, &LlmClient::diagnosticLog, this,
          &MainWindow::onLlmDiagnosticLog);

  // Setup API Wait Timer
  apiWaitTimer = new QTimer(this);
  connect(apiWaitTimer, &QTimer::timeout, this, &MainWindow::onApiTimerTick);

  // Connect Simulation Runner signals
  connect(simulationRunner, &SimulationRunner::outputReceived, this,
          &MainWindow::onSimulationOutputReceived);
  connect(simulationRunner, &SimulationRunner::errorReceived, this,
          &MainWindow::onSimulationErrorReceived);
  connect(simulationRunner, &SimulationRunner::processFinished, this,
          [this](int exitCode) {
            statusBar()->showMessage(
                tr("Simulation finished with code: %1").arg(exitCode), 5000);
          });
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
  resize(1200, 800);
  createToolbar();
  createCentralWidget();
  createStatusBar();
}

void MainWindow::createToolbar() {
  QToolBar *toolbar = addToolBar("Main Actions");

  btnGeneratePlan = new QPushButton("1. Generate Test Plan", this);
  btnGenerateCode = new QPushButton("2. Generate SV Code", this);
  btnRunSimulation = new QPushButton("3. Run Simulation", this);

  toolbar->addWidget(btnGeneratePlan);
  toolbar->addWidget(btnGenerateCode);
  toolbar->addWidget(btnRunSimulation);

  connect(btnGeneratePlan, &QPushButton::clicked, this,
          &MainWindow::onGeneratePlanClicked);
  connect(btnGenerateCode, &QPushButton::clicked, this,
          &MainWindow::onGenerateCodeClicked);
  connect(btnRunSimulation, &QPushButton::clicked, this,
          &MainWindow::onRunSimulationClicked);
}

void MainWindow::createCentralWidget() {
  QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);

  // Left Pane: Input Specification
  QWidget *leftWidget = new QWidget(this);
  QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);

  QHBoxLayout *pdfSelectLayout = new QHBoxLayout();
  btnSelectPdf = new QPushButton("Browse PDF", this);
  pdfPathLabel = new QLabel("No PDF selected", this);
  pdfPathLabel->setStyleSheet("color: gray; font-style: italic;");
  pdfSelectLayout->addWidget(btnSelectPdf);
  pdfSelectLayout->addWidget(pdfPathLabel);
  pdfSelectLayout->addStretch();
  connect(btnSelectPdf, &QPushButton::clicked, this,
          &MainWindow::onSelectPdfClicked);

  leftLayout->addWidget(new QLabel("Hardware Specification (Input):"));
  leftLayout->addLayout(pdfSelectLayout);

  specInputTextEdit = new QTextEdit(this);
  specInputTextEdit->setPlaceholderText(
      "Paste your hardware specification here OR select a PDF file above...");
  QFont monoFont("Consolas");
  monoFont.setStyleHint(QFont::Monospace);
  specInputTextEdit->setFont(monoFont);
  leftLayout->addWidget(specInputTextEdit);

  // Right Pane: Output Tabs & Config
  QWidget *rightWidget = new QWidget(this);
  QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);

  // Config Group
  QGroupBox *configGroup = new QGroupBox("Configuration", this);
  QFormLayout *configLayout = new QFormLayout(configGroup);

  llmProviderCombo = new QComboBox(this);
  llmProviderCombo->addItem(
      "OpenRouter",
      QVariant::fromValue(static_cast<int>(LlmClient::Provider::OpenRouter)));
  llmProviderCombo->addItem(
      "Google Gemini",
      QVariant::fromValue(static_cast<int>(LlmClient::Provider::Gemini)));

  llmApiKeyInput = new QLineEdit(this);
  llmApiKeyInput->setEchoMode(QLineEdit::Password);
  llmApiKeyInput->setPlaceholderText("Enter API Key here");

  sshHostInput = new QLineEdit(this);
  sshHostInput->setPlaceholderText("e.g. user@linux-server.local");

  configLayout->addRow("LLM Provider:", llmProviderCombo);
  configLayout->addRow("API Key:", llmApiKeyInput);
  configLayout->addRow("SSH Target:", sshHostInput);

  rightLayout->addWidget(configGroup);

  // Tabs
  mainTabWidget = new QTabWidget(this);

  testPlanTextEdit = new QTextEdit(this);
  testPlanTextEdit->setFont(monoFont);
  testPlanTextEdit->setReadOnly(true);

  generatedCodeTextEdit = new QTextEdit(this);
  generatedCodeTextEdit->setFont(monoFont);

  simulationLogTextEdit = new QTextEdit(this);
  simulationLogTextEdit->setFont(monoFont);
  simulationLogTextEdit->setReadOnly(true);
  simulationLogTextEdit->setStyleSheet(
      "background-color: #1e1e1e; color: #d4d4d4;");

  mainTabWidget->addTab(testPlanTextEdit, "Test Plan JSON");
  mainTabWidget->addTab(generatedCodeTextEdit, "SystemVerilog Code");
  mainTabWidget->addTab(simulationLogTextEdit, "Simulation Logs");

  rightLayout->addWidget(mainTabWidget);

  mainSplitter->addWidget(leftWidget);
  mainSplitter->addWidget(rightWidget);
  mainSplitter->setStretchFactor(0, 1);
  mainSplitter->setStretchFactor(1, 2);

  setCentralWidget(mainSplitter);
}

void MainWindow::createStatusBar() { statusBar()->showMessage("Ready"); }

void MainWindow::onSelectPdfClicked() {
  QString filePath = QFileDialog::getOpenFileName(
      this, "Select Hardware Specification PDF", "", "PDF Files (*.pdf)");

  if (!filePath.isEmpty()) {
    selectedPdfPath = filePath;
    pdfPathLabel->setText(filePath);
    pdfPathLabel->setStyleSheet(""); // Remove gray italic style
    specInputTextEdit->clear();
    specInputTextEdit->setPlaceholderText(
        "Using PDF file: " + filePath +
        "\n\nYou can still add supplementary instructions here.");
  }
}

void MainWindow::onGeneratePlanClicked() {
  QString spec = specInputTextEdit->toPlainText();
  if (spec.isEmpty() && selectedPdfPath.isEmpty()) {
    QMessageBox::warning(this, "Missing Input",
                         "Please enter a specification text or select a PDF.");
    return;
  }

  QString apiKey = llmApiKeyInput->text();
  if (apiKey.isEmpty()) {
    QMessageBox::warning(this, "Missing API Key",
                         "Please enter a Gemini API Key.");
    return;
  }

  statusBar()->showMessage("Preparing to send request...");
  simulationLogTextEdit->append("--- Starting Test Plan Generation ---");
  apiElapsedSeconds = 0;
  lastUploadLogPercent = -1;
  apiWaitTimer->start(1000); // Tick every 1 second

  btnGeneratePlan->setEnabled(false);
  btnGenerateCode->setEnabled(false);

  LlmClient::Provider selectedProvider =
      static_cast<LlmClient::Provider>(llmProviderCombo->currentData().toInt());
  llmClient->setProvider(selectedProvider);
  llmClient->setApiKey(apiKey);

  if (!selectedPdfPath.isEmpty()) {
    llmClient->generateTestPlanFromPdf(selectedPdfPath, spec);
  } else {
    llmClient->generateTestPlan(spec);
  }

  mainTabWidget->setCurrentWidget(testPlanTextEdit);
  testPlanTextEdit->setPlainText("Thinking...");
}

void MainWindow::onGenerateCodeClicked() {
  QString plan = testPlanTextEdit->toPlainText();
  if (plan.isEmpty() || plan == "Thinking...") {
    QMessageBox::warning(this, "Missing Plan",
                         "Please generate a test plan first.");
    return;
  }

  QString apiKey = llmApiKeyInput->text();
  if (apiKey.isEmpty()) {
    QMessageBox::warning(this, "Missing API Key",
                         "Please enter a Gemini API Key.");
    return;
  }

  statusBar()->showMessage("Preparing to generate code...");
  simulationLogTextEdit->append("--- Starting Code Generation ---");
  apiElapsedSeconds = 0;
  lastUploadLogPercent = -1;
  apiWaitTimer->start(1000); // Tick every 1 second

  btnGeneratePlan->setEnabled(false);
  btnGenerateCode->setEnabled(false);

  LlmClient::Provider selectedProvider =
      static_cast<LlmClient::Provider>(llmProviderCombo->currentData().toInt());
  llmClient->setProvider(selectedProvider);
  llmClient->setApiKey(apiKey);
  llmClient->generateSystemVerilog(plan);

  mainTabWidget->setCurrentWidget(generatedCodeTextEdit);
  generatedCodeTextEdit->setPlainText("Writing SystemVerilog code...");
}

void MainWindow::onRunSimulationClicked() {
  QString code = generatedCodeTextEdit->toPlainText();
  if (code.isEmpty() || code.startsWith("Writing")) {
    QMessageBox::warning(this, "Missing Code", "Please generate code first.");
    return;
  }

  QString sshTarget = sshHostInput->text();
  if (sshTarget.isEmpty()) {
    QMessageBox::warning(this, "Missing SSH config",
                         "Please configure SSH target.");
    return;
  }

  statusBar()->showMessage("Running Simulation via SSH...");
  mainTabWidget->setCurrentWidget(simulationLogTextEdit);
  simulationLogTextEdit->clear();

  QString command =
      QString("echo '%1' > tb.sv && vcs -sverilog tb.sv && ./simv")
          .arg(code.replace("'", "'\\''"));
  simulationRunner->runSshCommand(sshTarget, command);
}

void MainWindow::onLlmResponseReceived(const QString &response,
                                       const QString &type) {
  apiWaitTimer->stop();
  statusBar()->showMessage(QString("LLM Generation Complete! Took %1 seconds.")
                               .arg(apiElapsedSeconds),
                           5000);

  btnGeneratePlan->setEnabled(true);
  btnGenerateCode->setEnabled(true);

  if (type == "plan") {
    testPlanTextEdit->setPlainText(response);
  } else if (type == "code") {
    generatedCodeTextEdit->setPlainText(response);
  }
}

void MainWindow::onLlmErrorOccurred(const QString &err) {
  apiWaitTimer->stop();
  statusBar()->showMessage("LLM Generation Failed.", 5000);

  btnGeneratePlan->setEnabled(true);
  btnGenerateCode->setEnabled(true);

  QMessageBox::critical(this, "LLM API Error", err);
}

void MainWindow::onLlmUploadProgress(qint64 bytesSent, qint64 bytesTotal) {
  if (bytesTotal > 0) {
    int percent = static_cast<int>((bytesSent * 100) / bytesTotal);

    // Log progress at roughly 25% intervals
    int interval = (percent / 25) * 25;
    if (interval != lastUploadLogPercent && interval <= 100) {
      simulationLogTextEdit->append(QString("<span style='color:cyan'>[Network "
                                            "Log] Upload progress: %1%</span>")
                                        .arg(interval));
      lastUploadLogPercent = interval;
    }

    statusBar()->showMessage(QString("Uploading document: %1%").arg(percent));
  }
}

void MainWindow::onLlmDiagnosticLog(const QString &msg) {
  simulationLogTextEdit->append(
      QString("<span style='color:cyan'>[Network Log] %1</span>").arg(msg));
}

void MainWindow::onApiTimerTick() {
  apiElapsedSeconds++;
  // Only show "Thinking" if we aren't actively showing an upload progress
  // message
  if (!statusBar()->currentMessage().startsWith("Uploading")) {
    statusBar()->showMessage(
        QString("Thinking... (%1s)").arg(apiElapsedSeconds));
  }
}

void MainWindow::onSimulationOutputReceived(const QString &output) {
  simulationLogTextEdit->append(output);
}

void MainWindow::onSimulationErrorReceived(const QString &error) {
  simulationLogTextEdit->append("<span style='color:red'>" + error + "</span>");
}
