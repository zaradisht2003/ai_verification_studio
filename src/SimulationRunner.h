#ifndef SIMULATIONRUNNER_H
#define SIMULATIONRUNNER_H

#include <QObject>
#include <QProcess>

class SimulationRunner : public QObject {
  Q_OBJECT

public:
  explicit SimulationRunner(QObject *parent = nullptr);
  ~SimulationRunner();

  // Run a command on a remote Linux host via SSH
  void runSshCommand(const QString &sshTarget, const QString &command);

public slots:
  void cancelSimulation();

signals:
  void outputReceived(const QString &output);
  void errorReceived(const QString &error);
  void processFinished(int exitCode);

private slots:
  void onReadyReadStandardOutput();
  void onReadyReadStandardError();
  void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
  QProcess *process;
};

#endif // SIMULATIONRUNNER_H
