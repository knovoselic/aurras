#include <QCoreApplication>
#include <QtGlobal>
#include <QTimer>
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
#define STATE_UPDATE_INTERVAL 500

bool mute = true;
bool anyInputDeviceActive = false;
PulseAudio *pa;
KeyboardDriver *keyboard;
QTimer *updateKeyboardIndicatorTimer;

void handleSignals(int signal) {
    Q_UNUSED(signal);

    qApp->quit();
}

void updateKeyboardIndicator() {
    if (mute) {
        keyboard->setHsv(0, 255, 255, STATE_UPDATE_INTERVAL * 2);
    } else {
        keyboard->setHsv(80, 255, 255, STATE_UPDATE_INTERVAL * 2);
    }
}

void updateMute() {
    pa->setMuteForAllInputDevices(mute);
    updateKeyboardIndicator();
}

void handleIPCCommand(SimpleIPC::ipc_command command) {
    switch (command) {
    case SimpleIPC::ipc_command::IPC_COMMAND_TOGGLE_MUTE:
        if (anyInputDeviceActive) {
            mute = !mute;
        }
        updateMute();
        break;
    default:
        qWarning() << "Received unexpected IPC command:" << command;
        break;
    }
}

void handleActiveSourceOutputCountChanged(int new_count) {
    anyInputDeviceActive = new_count > 0;
    if (anyInputDeviceActive) {
        updateKeyboardIndicatorTimer->start();
    } else {
        updateKeyboardIndicatorTimer->stop();
        mute = true;
        updateMute();
    }

    updateKeyboardIndicator();
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

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    SimpleIPC ipc(SHARED_MEMORY_KEY);

    app.setApplicationName("Aurras");
    app.setApplicationVersion("v1.0");
    if (!ipc.initializeDaemon()) {
        return client_main(ipc, app);
    }

    pa = new PulseAudio();
    keyboard = new KeyboardDriver();
    updateKeyboardIndicatorTimer = new QTimer();

    updateKeyboardIndicatorTimer->setInterval(STATE_UPDATE_INTERVAL);

    QObject::connect(&ipc, &SimpleIPC::commandReceived, handleIPCCommand);
    QObject::connect(pa, &PulseAudio::activeSourceOutputCountChanged, handleActiveSourceOutputCountChanged);
    QObject::connect(updateKeyboardIndicatorTimer, &QTimer::timeout, updateKeyboardIndicator);

    pa->updateSourceOutputCount();

    signal(SIGTERM, handleSignals);
    signal(SIGABRT, handleSignals);
    signal(SIGINT, handleSignals);

    updateMute();

    int returnValue = app.exec();

    delete pa;
    delete keyboard;
    delete updateKeyboardIndicatorTimer;

    return returnValue;
}
