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

bool mute = false;

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

    PulseAudio pa;
    KeyboardDriver keyboard;

    qDebug() << "Main thread" << QThread::currentThreadId();
    pa.setMuteForAllInputDevices(true);
    keyboard.set_hsv(80, 255, 255, 1000);

    return a.exec();
}
