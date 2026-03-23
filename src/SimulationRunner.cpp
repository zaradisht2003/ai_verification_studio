#include "SimulationRunner.h"

SimulationRunner::SimulationRunner(QObject *parent)
    : QObject(parent), process(new QProcess(this)), isScpProcess(false) {
  connect(process, &QProcess::readyReadStandardOutput, this,
          &SimulationRunner::onReadyReadStandardOutput);
  connect(process, &QProcess::readyReadStandardError, this,
          &SimulationRunner::onReadyReadStandardError);
  connect(process, &QProcess::finished, this,
          &SimulationRunner::onProcessFinished);
}

SimulationRunner::~SimulationRunner() {
  if (process->state() == QProcess::Running) {
    process->kill();
    process->waitForFinished();
  }
}

void SimulationRunner::runSshCommand(const QString &sshTarget,
                                     const QString &sshKey,
                                     const QString &command) {
  if (process->state() == QProcess::Running) {
    emit errorReceived("A simulation is already running.");
    return;
  }
  isScpProcess = false;

  QString program = "ssh";
  QStringList arguments;
  if (!sshKey.isEmpty()) {
    arguments << "-i" << sshKey;
  }

  // Use -o StrictHostKeyChecking=no to avoid prompt blocks if user directories
  // are restricted or known_hosts is inaccessible
  arguments << "-o" << "StrictHostKeyChecking=no";
  arguments << "-T" << sshTarget << command;

  process->start(program, arguments);
}

void SimulationRunner::scpFile(const QString &sshTarget, const QString &sshKey,
                               const QString &localPath,
                               const QString &remotePath) {
  if (process->state() == QProcess::Running) {
    emit errorReceived("A process is already running.");
    return;
  }
  isScpProcess = true;

  QString program = "scp";
  QStringList arguments;
  if (!sshKey.isEmpty()) {
    arguments << "-i" << sshKey;
  }

  arguments << "-o" << "StrictHostKeyChecking=no";
  arguments << localPath << QString("%1:%2").arg(sshTarget).arg(remotePath);

  process->start(program, arguments);
}

void SimulationRunner::cancelSimulation() {
  if (process->state() == QProcess::Running) {
    process->kill();
    process->waitForFinished();
    emit errorReceived("Simulation cancelled.");
  }
}

void SimulationRunner::onReadyReadStandardOutput() {
  QByteArray output = process->readAllStandardOutput();
  emit outputReceived(QString::fromLocal8Bit(output).trimmed());
}

void SimulationRunner::onReadyReadStandardError() {
  QByteArray error = process->readAllStandardError();
  emit errorReceived(QString::fromLocal8Bit(error).trimmed());
}

void SimulationRunner::onProcessFinished(int exitCode,
                                         QProcess::ExitStatus exitStatus) {
  emit processFinished(exitCode);
  if (isScpProcess) {
    emit scpFinished(exitCode);
  } else {
    emit sshFinished(exitCode);
  }
}
