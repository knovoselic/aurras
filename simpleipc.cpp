#include <QCryptographicHash>
#include "simpleipc.h"

static const int _SimpleIPCIpcCommandMetaTypeId = qRegisterMetaType<SimpleIPC::ipc_command>();

namespace {
    QString generate_key_hash(const QString& key, const QString& salt) {
        QByteArray data;

        data.append(key.toUtf8());
        data.append(salt.toUtf8());
        data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

        return data;
    }
}

SimpleIPC::SimpleIPC(const QString& key) :
    key(key),
    memory_lock_key(generate_key_hash(key, "_memory_lock_key")),
    shared_memory_key(generate_key_hash(key, "_shared_memory_key")),
    shared_memory(shared_memory_key),
    memory_lock(memory_lock_key, 1) {
    memory_lock.acquire();
    {
        QSharedMemory fix(shared_memory_key);    // Fix for *nix: http://habrahabr.ru/post/173281/
        fix.attach();
    }
    memory_lock.release();
}

SimpleIPC::~SimpleIPC() {
    requestInterruption();
    wait();
    release();
}

void SimpleIPC::run() {
    while(!isInterruptionRequested()) {
        shared_memory.lock();
        quint64 value = *(quint64 *)shared_memory.constData();
        shared_memory.unlock();
        if (value != ipc_command::IPC_COMMAND_WAITING) {
            writeToSharedMemory(ipc_command::IPC_COMMAND_WAITING);
            emit commandReceived((ipc_command)value);
        }
        msleep(100);
    }
}

/*!
  \fn bool SimpleIPC::initializeDaemon()

  Initializes SimpleIPC and application shared memory. If this is the first
  instance of the application, it will also start the shared memory watcher thread.

  If this is the first instance of the application, it returns \c true.
  If an instance of the application is already running, returns \c false.
*/
bool SimpleIPC::initializeDaemon() {
    bool isFirst = true;

    memory_lock.acquire();
    const bool result = shared_memory.create(sizeof(quint64));
    if (result) {
        start();
    } else {
        // unable to create the shared memory - we are probably not the first instance
        // attach to existing shared memory
        if (!shared_memory.attach()) {
            qFatal("Unable to attach to the shared memory!");
        }
        isFirst = false;
    }
    memory_lock.release();

    return isFirst;
}

void SimpleIPC::writeToSharedMemory(quint64 value) {
    shared_memory.lock();
    quint64 *shared_value_pointer = (quint64 *) shared_memory.data();
    *shared_value_pointer = value;
    shared_memory.unlock();
}

void SimpleIPC::release() {
    memory_lock.acquire();
    if (shared_memory.isAttached()) {
        shared_memory.detach();
    }
    memory_lock.release();
}
