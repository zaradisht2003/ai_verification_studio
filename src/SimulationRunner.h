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
  void runSshCommand(const QString &sshTarget, const QString &sshKey,
                     const QString &command);

  // Copy a local file to a remote Linux host via SCP
  void scpFile(const QString &sshTarget, const QString &sshKey,
               const QString &localPath, const QString &remotePath);

public slots:
  void cancelSimulation();

signals:
  void outputReceived(const QString &output);
  void errorReceived(const QString &error);
  void processFinished(int exitCode);
  void scpFinished(int exitCode);
  void sshFinished(int exitCode);

private slots:
  void onReadyReadStandardOutput();
  void onReadyReadStandardError();
  void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
  QProcess *process;
  bool isScpProcess; // Track if the current process is an scp
};

#endif // SIMULATIONRUNNER_H
