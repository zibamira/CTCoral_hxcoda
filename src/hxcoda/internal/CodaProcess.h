#pragma once

// Qt
#include <QProcess>
#include <QString>
#include <QTimer>


namespace coda
{


/**
 * @brief The CodaProcess class
 *
 * This class wraps a Coda process. It allows to launch and manage the runtime 
 * of Coda, making it easier for end-users to use the tool by just having it 
 * installed.
 */
class CodaProcess : public QObject
{
    Q_OBJECT

public:

    CodaProcess(const QString& data_directory, QObject* parent = nullptr);
    virtual ~CodaProcess();

    QString findPython();

    void start();
    void stop();

    bool isRunning();
    QString codaUrl();

    void startBrowser();
    
protected slots:

    void on_process_started();
    void on_process_finished(int exitCode);
    void on_process_errorOccurred(QProcess::ProcessError error);
    void on_process_readyReadStandardError();
    void on_process_readyReadStandardOutput();

signals:

    void started();
    void finished();

private:

    QString m_data_directory;
    QString m_coda_url;

    QTimer m_kill_timer;
    QProcess* m_process;
};


} // namespace coda