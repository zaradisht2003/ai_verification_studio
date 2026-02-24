#include "SimulationRunner.h"

SimulationRunner::SimulationRunner(QObject *parent)
    : QObject(parent), process(new QProcess(this)) {
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
                                     const QString &command) {
  if (process->state() == QProcess::Running) {
    emit errorReceived("A simulation is already running.");
    return;
  }

  // Typical Windows usage of SSH without strictly requiring PuTTY if Windows
  // 10+ ssh.exe is available
  QString program = "ssh";
  QStringList arguments;
  arguments << "-T" << sshTarget << command;

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
}
