#ifndef RUNGUARD_H
#define RUNGUARD_H

// Based on code from https://stackoverflow.com/questions/5006547/qt-best-practice-for-a-single-instance-app-protection

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

class RunGuard
{

public:
    RunGuard(const QString& key);
    ~RunGuard();

    bool isAnotherRunning();
    bool tryToRun();
    void release();

private:
    const QString key;
    const QString memory_lock_key;
    const QString shared_memory_key;

    QSharedMemory shared_memory;
    QSystemSemaphore memory_lock;

    Q_DISABLE_COPY(RunGuard)
};
#endif // RUNGUARD_H
