#include <QCoreApplication>
#include <QDebug>
#include <iostream>
#include <string.h>
#include <QtGlobal>
#include <QProcess>

#include "pulseaudio.h"
#include "xhklib.h"

void logToStderr(QtMsgType type, const QMessageLogContext& context,
                 const QString& msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);

    // now output to debugger console
#ifdef Q_OS_WIN
    OutputDebugString(msg.toStdWString().c_str());
#else
    if (type == QtMsgType::QtWarningMsg || type == QtMsgType::QtCriticalMsg || type == QtMsgType::QtFatalMsg) {
        std::cerr << msg.toStdString() << std::endl << std::flush;
    } else {
        std::cout << msg.toStdString() << std::endl << std::flush;
    }

#endif
}

PulseAudio pa;
int mute = 0;

void keyboard_shortcut_pressed(xhkEvent e, void *a1, void *a2, void *a3) {
    Q_UNUSED(e);
    Q_UNUSED(a1);
    Q_UNUSED(a2);
    Q_UNUSED(a3);

    QString osdArguments;
    if (mute) {
        mute = 0;
        osdArguments = QString("{'icon': <'microphone-sensitivity-high-symbolic'>, 'label': <'All recording devices'>}");
    } else {
        mute = 1;
        osdArguments = QString("{'icon': <'microphone-sensitivity-muted-symbolic'>, 'label': <'All recording devices'>}");
    }

    pa.setAllInputDevicesMute(mute);
    // Show gnome notification (just for testing, should fail silently on other DEs)
    QStringList arguments = {
        "call",
        "--session",
        "--dest",
        "org.gnome.Shell",
        "--object-path",
        "/org/gnome/Shell",
        "--method",
        "org.gnome.Shell.ShowOSD",
        osdArguments
    };
    QProcess::startDetached("gdbus", arguments);
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(&logToStderr);
    qDebug() << "Main thread" << QThread::currentThreadId();

#if 1
    // this is just used for easier testing
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    xhkConfig *hk_config = xhkInit(NULL);
    xhkBindKey(hk_config, 0, XK_F11, 0, xhkKeyPress, &keyboard_shortcut_pressed, 0, 0, 0);

    qDebug() <<  pa.getInputDeviceCount();
    // get all recording devices into a known state (muted)
    keyboard_shortcut_pressed(xhkEvent(), NULL, NULL, NULL);

    while (1) {
        xhkPollKeys(hk_config, 1);
    }

    xhkClose(hk_config);
    return 0;
#else
    QCoreApplication a(argc, argv);
    return a.exec();
#endif
}
