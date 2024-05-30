// STL
#include <iostream>

// Qt
#include <QDebug>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>

// ZIB
#include <hxcoda/internal/CodaProcess.h>


namespace coda
{


CodaProcess::CodaProcess(const QString& data_directory, QObject* parent)
    : m_data_directory(data_directory)
    , m_coda_url("http://localhost:5006")
    , m_kill_timer()
    , m_process(nullptr)
{}


CodaProcess::~CodaProcess()
{}


QString CodaProcess::findPython()
{
    // ToDo: We can't just search for a "python3" executable because
    //       we would end up with the one included in Amira. 
    //       The *Coda* Python package is however installed in the 
    //       users Python directory and uses the system Python interpreter.
    //
    //       I think the best way to fix this is by installing a "coda-amira"
    //       script during the Python pip install. The script will not be 
    //       in the Amira prepacks and can thus be found by QStandardPaths.
    //
    return QString("/usr/bin/python3");
}


void CodaProcess::start()
{
    if(m_process != nullptr)
    {
        qDebug() << "Coda is already running.";
        return;
    }

    // Locate Python.
    const QString program = findPython();
    if(program.isEmpty())
    {
        qDebug() << "Could not find Python.";
        return;
    }

    qDebug() << "Using python " << program;

    // Setup the process.
    m_process = new QProcess(this);
    connect(m_process, &QProcess::started, this, &CodaProcess::on_process_started);
    connect(m_process, SIGNAL(finished(int)), this, SLOT(on_process_finished(int)));
    connect(m_process, &QProcess::errorOccurred, this, &CodaProcess::on_process_errorOccurred);
    connect(m_process, &QProcess::readyReadStandardError, this, &CodaProcess::on_process_readyReadStandardError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &CodaProcess::on_process_readyReadStandardOutput);

    // Launch.
    QStringList arguments;
    arguments
        << "-m" << "coda"
        << "--start-browser"
        << "amira"
        << "--directory" << m_data_directory;

    m_process->start(program, arguments);
    m_process->waitForStarted();
}


void CodaProcess::stop()
{
    if(m_process == nullptr)
    {
        qDebug() << "Coda is already stopped.";
        return;
    }

    // Stop Coda and also start the kill timer.
    qDebug() << "Terminating Coda ..."; 
    m_process->terminate();
    m_process->waitForFinished();
}


bool CodaProcess::isRunning()
{
    return m_process != nullptr;
}


QString CodaProcess::codaUrl()
{
    return m_coda_url;
}


void CodaProcess::startBrowser()
{   
    QDesktopServices::openUrl(m_coda_url); 
}


void CodaProcess::on_process_started()
{
    startBrowser();

    emit started();
}


void CodaProcess::on_process_finished(int exitCode)
{
    // The slot may also be called during deconstruction,
    // so be careful here.
    if(m_process != nullptr)
    {
        m_process->deleteLater();
        m_process = nullptr;
    }

    emit finished();
}


void CodaProcess::on_process_errorOccurred(QProcess::ProcessError error)
{
    qDebug() << "Coda process error " << error;
}


void CodaProcess::on_process_readyReadStandardError()
{
    qDebug() << m_process->readAllStandardError();   
}


void CodaProcess::on_process_readyReadStandardOutput()
{
    qDebug() << m_process->readAllStandardOutput();
}


} // namespace coda