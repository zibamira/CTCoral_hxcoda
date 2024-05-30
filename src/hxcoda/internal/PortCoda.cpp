// STL
#include <iostream>

// Qt
#include <QDebug>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <QUrl>

// Local
#include <hxcoda/internal/Coda.h>
#include <hxcoda/internal/PortCoda.h>


HX_PORT_INIT(PortCoda, HxPort)


PortCoda::PortCoda(
    HxObject* object, 
    const QString& name, 
    const QString& label, 
    QxPortNotifier* notifier
)
    : HxPort(object, name, label, notifier)
    , m_widget(nullptr)
    , m_startButton(nullptr)
    , m_stopButton(nullptr)
    , m_urlLabel(nullptr)
    , m_folderLabel(nullptr)
{}


PortCoda::PortCoda(
    HxEditor* editor, 
    const QString& name, 
    const QString& label, 
    QxPortNotifier* notifier
)
    : HxPort(editor, name, label, notifier)
    , m_widget(nullptr)
    , m_startButton(nullptr)
    , m_stopButton(nullptr)
    , m_urlLabel(nullptr)
    , m_folderLabel(nullptr)
{}


PortCoda::~PortCoda()
{}


void PortCoda::createWidget(QWidget* parent)
{
    auto coda = coda::theCoda();
    auto codaProcess = coda->process();
    auto codaUrl = codaProcess->codaUrl();
    auto codaDataDir = coda->dataDirectory();

    HxPort::createWidget(parent);

    // Widgets
    m_startButton = new QPushButton();
    m_startButton->setText(QObject::tr("Start"));
    QObject::connect(m_startButton, &QPushButton::clicked, parent, [this]() {
        this->on_startButton_clicked();
    });

    m_stopButton = new QPushButton();
    m_stopButton->setText(QObject::tr("Stop"));
    QObject::connect(m_stopButton, &QPushButton::clicked, parent, [this]() {
        this->on_stopButton_clicked();
    });

    m_urlLabel = new QLabel();
    m_urlLabel->setTextFormat(Qt::RichText);
    m_urlLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_urlLabel->setOpenExternalLinks(true);
    QObject::connect(m_urlLabel, &QLabel::linkActivated, parent, [this]() {
        this->on_urlLabel_clicked();
    });

    m_folderLabel = new QLabel();
    m_folderLabel->setTextFormat(Qt::RichText);
    m_folderLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_folderLabel->setOpenExternalLinks(true);
    QObject::connect(m_folderLabel, &QLabel::linkActivated, parent, [this]() {
        this->on_folderLabel_clicked();
    });

    // Layout
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(m_startButton);
    layout->addWidget(m_stopButton);
    layout->addWidget(m_urlLabel);
    layout->addWidget(m_folderLabel);

    m_widget = new QWidget(m_baseWidget);
    m_widget->setLayout(layout);
    m_widget->adjustSize();
    layout->setContentsMargins(0, 0, 0, 0);

    m_baseWidget->layout()->addWidget(m_widget);

    // Listen to changes.
    QObject::connect(codaProcess, &coda::CodaProcess::started, parent, [this]() {
        this->on_codaProcess_started();
    });
    QObject::connect(codaProcess, &coda::CodaProcess::finished, parent, [this]() {
        this->on_codaProcess_finished();
    });

    // Perform an initial update of the UI.
    updateUi();
}


void PortCoda::destroyWidget()
{
    HxPort::destroyWidget();
}


void PortCoda::commonInit(const QString& name, const QString& label, QxPortNotifier* notifier)
{
    HxPort::commonInit(name, label, notifier);
}


void PortCoda::updateUi()
{
    auto coda = coda::theCoda();
    if(!coda)
    {
        qWarning() << "Coda is not initialized within Amira.";
        return;
    }

    // The start/stop feature from inside Amira is only available
    // if a CodaProcess is attached to the Amira coda instance.
    //
    // Usually, this is the case.
    if(coda->process() != nullptr)
    {
        m_startButton->setVisible(!coda->process()->isRunning());
        m_stopButton->setVisible(coda->process()->isRunning());

        auto codaUrl = coda->process()->codaUrl();
        m_urlLabel->setText(QString("<a href=\"%1\">%1</a>").arg(codaUrl));
    }
    else
    {
        m_startButton->setVisible(false);
        m_stopButton->setVisible(false);
        m_urlLabel->setVisible(false);
    }
    
    // The shared data directory should always be available.
    auto codaDataDir = coda->dataDirectory();
    m_folderLabel->setText(QString("<a href=\"%1\">%1</a>").arg(codaDataDir));
}


void PortCoda::on_startButton_clicked()
{
    auto coda = coda::theCoda();
    if(!coda || !coda->process())
    {
        qWarning() << "Coda has no attached process.";
        return;
    }

    coda->process()->start();
    coda->process()->startBrowser();
}


void PortCoda::on_stopButton_clicked()
{
    auto coda = coda::theCoda();
    if(!coda || !coda->process())
    {
        qWarning() << "Coda has no attached process.";
        return;
    }

    coda->process()->stop();
}


void PortCoda::on_urlLabel_clicked()
{
    auto coda = coda::theCoda();
    if(!coda || !coda->process())
    {
        qWarning() << "Coda has no attached process.";
        return;
    }

    coda->process()->start();
    coda->process()->startBrowser();
}


void PortCoda::on_folderLabel_clicked()
{
    auto coda = coda::theCoda();
    if(!coda)
    {
        qWarning() << "No Amira coda instance detected.";
        return;
    }
    
    const QUrl url(coda->dataDirectory());
    QDesktopServices::openUrl(url);
}


void PortCoda::on_codaProcess_started()
{
    updateUi();
}


void PortCoda::on_codaProcess_finished()
{
    updateUi();
}