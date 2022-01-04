#include <QCoreApplication>
#include <QtGlobal>
#include <QProcess>
#include <QtConcurrent/QtConcurrent>

#include "pulseaudio.h"
#include "keyboarddriver.h"

KeyboardDriver keyboard;
PulseAudio pa;
bool mute = false;

#ifdef QT_DEBUG
#include "xhklib.h"

void keyboard_shortcut_pressed(xhkEvent e, void *a1, void *a2, void *a3) {
    Q_UNUSED(e);
    Q_UNUSED(a1);
    Q_UNUSED(a2);
    Q_UNUSED(a3);

    mute = !mute;
    pa.setMuteForAllInputDevices(mute);
}
#endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "Main thread" << QThread::currentThreadId();
    pa.setMuteForAllInputDevices(true);
    keyboard.set_hsv(80, 255, 255, 1000);

#ifdef QT_DEBUG
    // this is just used for easier testing
    QtConcurrent::run([&]{
        xhkConfig *hk_config = xhkInit(NULL);
        xhkBindKey(hk_config, 0, XK_F11, 0, xhkKeyPress, &keyboard_shortcut_pressed, 0, 0, 0);

        // get all recording devices into a known state (muted)
        keyboard_shortcut_pressed(xhkEvent(), NULL, NULL, NULL);

        while (1) {
            xhkPollKeys(hk_config, 1);
        }
        xhkClose(hk_config);
    });
#endif

    return a.exec();
}
