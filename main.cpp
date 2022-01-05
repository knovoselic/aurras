#include <QCoreApplication>
#include <QtGlobal>
#include <QProcess>
#include <QtConcurrent/QtConcurrent>
#include <csignal>

#include "pulseaudio.h"
#include "keyboarddriver.h"
#include "simpleipc.h"

#ifdef QT_DEBUG
#define SHARED_MEMORY_KEY "Aurras driver shared memory - DEBUG"
#else
#define SHARED_MEMORY_KEY "Aurras driver shared memory"
#endif

bool mute = true;
bool anyInputDeviceActive = false;
PulseAudio *pa;
KeyboardDriver *keyboard;

void handleSignals(int signal) {
    Q_UNUSED(signal);

    qApp->quit();
}

void handleIPCCommand(SimpleIPC::ipc_command command) {
    switch (command) {
    case SimpleIPC::ipc_command::IPC_COMMAND_TOGGLE_MUTE:
        mute = !mute;
        pa->setMuteForAllInputDevices(mute);
        break;
    default:
        qWarning() << "Received unexpected IPC command:" << command;
        break;
    }
}

void handleActiveSourceOutputCountChanged(int new_count) {
    anyInputDeviceActive = new_count > 0;
}

int client_main(SimpleIPC &guard, QCoreApplication &app) {
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
        guard.writeToSharedMemory(SimpleIPC::ipc_command::IPC_COMMAND_TOGGLE_MUTE);
        return 0;
    }

    qCritical() << "Another instance is already running!\n";
    parser.showHelp();
    return 1;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    SimpleIPC ipc(SHARED_MEMORY_KEY);

    app.setApplicationName("Aurras");
    app.setApplicationVersion("v0.1");
    if (!ipc.initializeDaemon()) {
        return client_main(ipc, app);
    }

    pa = new PulseAudio();
    keyboard = new KeyboardDriver();

    QObject::connect(&ipc, &SimpleIPC::commandReceived, handleIPCCommand);
    QObject::connect(pa, &PulseAudio::activeSourceOutputCountChanged, &handleActiveSourceOutputCountChanged);

    pa->updateSourceOutputCount();

    signal(SIGTERM, handleSignals);
    signal(SIGABRT, handleSignals);
    signal(SIGINT, handleSignals);

    pa->setMuteForAllInputDevices(mute);
    keyboard->setHsv(80, 255, 255, 1000);

    int returnValue = app.exec();

    delete pa;
    delete keyboard;

    return returnValue;
}
