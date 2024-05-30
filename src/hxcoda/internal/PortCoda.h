#pragma once

// ZIB
#include <hxcore/HxPort.h>

// Qt
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QWidget>

// Local
#include <hxcoda/api.h>


/**
 * @brief The PortCoda class
 *
 * This class implements an Amira port showing the current Coda configuration.
 * The user can launch Coda directly from within Amira or check where the data
 * is stored.
 */
class PortCoda : public HxPort
{
    HX_PORT(PortCoda);

public:

    PortCoda(HxObject* object, const QString& name, const QString& label, QxPortNotifier* notifier = NULL);
    PortCoda(HxEditor* editor, const QString& name, const QString& label, QxPortNotifier* notifier = NULL);
    ~PortCoda();

protected:

    void createWidget(QWidget* parent) override;
    void destroyWidget() override;
    void commonInit(const QString& name, const QString& label, QxPortNotifier* notifier);

    void updateUi();

    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_urlLabel_clicked();
    void on_folderLabel_clicked();

    void on_codaProcess_started();
    void on_codaProcess_finished();

private:

    QWidget* m_widget;
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QLabel* m_urlLabel;
    QLabel* m_folderLabel;
};