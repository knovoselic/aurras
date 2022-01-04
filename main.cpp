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

int client_main(RunGuard &guard, QCoreApplication &app) {
    QCommandLineParser parser;
    parser.setApplicationDescription("Aurras driver.\nTo start the daemon, "
                                     "run without any options. If the daemon "
                                     "is already running, use the one of the "
                                     "available options to send a command to the daemon.");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions({
        {{"t", "toggle-mute"}, QCoreApplication::translate("main", "Toggle mute state of all input devices")}
    });
    parser.process(app);

    if (parser.isSet("toggle-mute")) {
        guard.writeToSharedMemory(RunGuard::ipc_commands::TOGGLE_MUTE);
        return 0;
    }

    qCritical() << "Another instance is already running!\n";
    parser.showHelp();
    return 1;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    RunGuard guard(SHARED_MEMORY_KEY);

    app.setApplicationName("Aurras");
    app.setApplicationVersion("v0.1");
    if (!guard.initialize()) {
        return client_main(guard, app);
    }

    PulseAudio pa;
    KeyboardDriver keyboard;

    QObject::connect(&guard, &RunGuard::commandReceived, [](RunGuard::ipc_commands command) {
        qDebug() << "Command received:" << command;
    });

    signal(SIGTERM, handle_signals);
    signal(SIGABRT, handle_signals);
    signal(SIGINT, handle_signals);

    pa.setMuteForAllInputDevices(true);
    keyboard.set_hsv(80, 255, 255, 1000);

    return app.exec();
}
