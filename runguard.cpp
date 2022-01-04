#include "runguard.h"

#include <QCryptographicHash>

namespace {
    QString generate_key_hash(const QString& key, const QString& salt) {
        QByteArray data;

        data.append(key.toUtf8());
        data.append(salt.toUtf8());
        data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

        return data;
    }
}

RunGuard::RunGuard(const QString& key) :
    key(key),
    memory_lock_key(generate_key_hash(key, "_memory_lock_key")),
    shared_memory_key(generate_key_hash(key, "_shared_memory_key")),
    shared_memory(shared_memory_key),
    memory_lock(memory_lock_key, 1)
{
    memory_lock.acquire();
    {
        QSharedMemory fix(shared_memory_key);    // Fix for *nix: http://habrahabr.ru/post/173281/
        fix.attach();
    }
    memory_lock.release();
}

RunGuard::~RunGuard()
{
    release();
}

bool RunGuard::isAnotherRunning()
{
    if (shared_memory.isAttached()) {
        return false;
    }

    memory_lock.acquire();
    const bool isRunning = shared_memory.attach();
    if (isRunning) {
        shared_memory.detach();
    }
    memory_lock.release();

    return isRunning;
}

bool RunGuard::tryToRun()
{
    if (isAnotherRunning()) {
        return false;
    }

    memory_lock.acquire();
    const bool result = shared_memory.create(sizeof(quint64));
    memory_lock.release();
    if (!result) {
        release();
        return false;
    }

    return true;
}

void RunGuard::release()
{
    memory_lock.acquire();
    if (shared_memory.isAttached()) {
        shared_memory.detach();
    }
    memory_lock.release();
}
