#include <QCoreApplication>
#include <QtGlobal>
#include <QProcess>
#include <QtConcurrent/QtConcurrent>
#include <csignal>

#include "pulseaudio.h"
#include "keyboarddriver.h"
#include "runguard.h"

#ifdef QT_DEBUG
    #define SHARED_MEMORY_KEY "Aurras driver shared memory - DEBUG"
#else
    #define SHARED_MEMORY_KEY "Aurras driver shared memory"
#endif

KeyboardDriver *keyboard;
PulseAudio *pa;
bool mute = false;

#ifdef QT_DEBUG
#include "xhklib.h"

void keyboard_shortcut_pressed(xhkEvent e, void *a1, void *a2, void *a3) {
    Q_UNUSED(e);
    Q_UNUSED(a1);
    Q_UNUSED(a2);
    Q_UNUSED(a3);

    mute = !mute;
    pa->setMuteForAllInputDevices(mute);
}
#endif

void handle_signals(int signal) {
    Q_UNUSED(signal);

    qApp->quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    RunGuard guard(SHARED_MEMORY_KEY);

    if (!guard.tryToRun()) {
        qDebug() << "Another instance is already running!";
        return 1;
    }

    signal(SIGTERM, handle_signals);
    signal(SIGABRT, handle_signals);
    signal(SIGINT, handle_signals);

    pa = new PulseAudio();
    keyboard = new KeyboardDriver();

    qDebug() << "Main thread" << QThread::currentThreadId();
    pa->setMuteForAllInputDevices(true);
    keyboard->set_hsv(80, 255, 255, 1000);

#ifdef QT_DEBUG
    bool running = true;
    // this is just used for easier testing
    auto xhk_loop = QtConcurrent::run([&]{
        xhkConfig *hk_config = xhkInit(NULL);
        xhkBindKey(hk_config, 0, XK_F11, 0, xhkKeyPress, &keyboard_shortcut_pressed, 0, 0, 0);

        // get all recording devices into a known state (muted)
        keyboard_shortcut_pressed(xhkEvent(), NULL, NULL, NULL);

        while (running) {
            xhkPollKeys(hk_config, 0);
            QThread::msleep(100);
        }
        xhkClose(hk_config);
    });
#endif

    int return_value = a.exec();

#ifdef QT_DEBUG
    running = false;
    xhk_loop.waitForFinished();
#endif

    delete pa;
    delete keyboard;

    return return_value;
}
