#include <QCoreApplication>
#include <QDebug>
#include <iostream>
#include <string.h>
#include <QtGlobal>
#include <QProcess>

#include "pulseaudio.h"
#include "keyboarddriver.h"
#include "xhklib.h"

KeyboardDriver keyboard;
PulseAudio pa;
bool mute = false;

void keyboard_shortcut_pressed(xhkEvent e, void *a1, void *a2, void *a3) {
    Q_UNUSED(e);
    Q_UNUSED(a1);
    Q_UNUSED(a2);
    Q_UNUSED(a3);

    QString osdArguments;
    mute = !mute;
    if (mute) {
        osdArguments = QString("{'icon': <'microphone-sensitivity-muted-symbolic'>, 'label': <'All recording devices'>}");
    } else {
        osdArguments = QString("{'icon': <'microphone-sensitivity-high-symbolic'>, 'label': <'All recording devices'>}");
    }

    pa.setMuteForAllInputDevices(mute);
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
    QProcess gdbus;
    gdbus.setStandardOutputFile(QProcess::nullDevice());
    gdbus.setStandardErrorFile(QProcess::nullDevice());
    gdbus.setProgram("gdbus");
    gdbus.setArguments(arguments);
    gdbus.startDetached();
}

int main(int argc, char *argv[])
{
    qDebug() << "Main thread" << QThread::currentThreadId();
    keyboard.set_hsv(80, 255, 255, 1000);

#if 0
    // this is just used for easier testing
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    xhkConfig *hk_config = xhkInit(NULL);
    xhkBindKey(hk_config, 0, XK_F11, 0, xhkKeyPress, &keyboard_shortcut_pressed, 0, 0, 0);

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
