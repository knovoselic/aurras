#ifndef RUNGUARD_H
#define RUNGUARD_H

// Based on code from https://stackoverflow.com/questions/5006547/qt-best-practice-for-a-single-instance-app-protection

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QSharedMemory>
#include <QSystemSemaphore>

class RunGuard : public QThread
{
    Q_OBJECT

public:
    enum ipc_commands {
        WAITING_FOR_COMMAND = 0,
        TOGGLE_MUTE
    };
    Q_ENUM(ipc_commands);

    RunGuard(const QString& key);
    ~RunGuard();

    void run() override;

    bool initialize();
    void writeToSharedMemory(quint64 value);
    void release();

signals:
    void commandReceived(RunGuard::ipc_commands command);

private:
    const QString key;
    const QString memory_lock_key;
    const QString shared_memory_key;

    QSharedMemory shared_memory;
    QSystemSemaphore memory_lock;

    Q_DISABLE_COPY(RunGuard)
};

Q_DECLARE_METATYPE(RunGuard::ipc_commands);

#endif // RUNGUARD_H
