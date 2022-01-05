#ifndef SIMPLEIPC_H
#define SIMPLEIPC_H

// Based on code from https://stackoverflow.com/questions/5006547/qt-best-practice-for-a-single-instance-app-protection

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QSharedMemory>
#include <QSystemSemaphore>

class SimpleIPC : public QThread
{
    Q_OBJECT

public:
    enum ipc_command {
        IPC_COMMAND_WAITING = 0,
        IPC_COMMAND_TOGGLE_MUTE
    };
    Q_ENUM(ipc_command);

    SimpleIPC(const QString& key);
    ~SimpleIPC();

    void run() override;

    bool initializeDaemon();
    void writeToSharedMemory(quint64 value);
    void release();

signals:
    void commandReceived(SimpleIPC::ipc_command command);

private:
    const QString key;
    const QString memory_lock_key;
    const QString shared_memory_key;

    QSharedMemory shared_memory;
    QSystemSemaphore memory_lock;

    Q_DISABLE_COPY(SimpleIPC)
};

Q_DECLARE_METATYPE(SimpleIPC::ipc_command);

#endif // SIMPLEIPC_H
