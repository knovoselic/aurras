#include <QCoreApplication>
#include <QDebug>
#include <iostream>
#include <string.h>
#include "pulseaudio.h"
#include <QtGlobal>

void logToStderr(QtMsgType type, const QMessageLogContext& context,
                 const QString& msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);

    // now output to debugger console
#ifdef Q_OS_WIN
    OutputDebugString(msg.toStdWString().c_str());
#else
    std::cerr << msg.toStdString() << std::endl << std::flush;
#endif
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(&logToStderr);
    qDebug() << "Main thread" << QThread::currentThreadId();
    QCoreApplication a(argc, argv);
    PulseAudio pa;

    int ctr;

    // This is where we'll store the input device list
    pa_devicelist_t* pa_input_devicelist = pa.GetInputDevices();

    for (ctr = 0; ctr < PulseAudio::MAX_DEVICES; ctr++) {
        if (! pa_input_devicelist[ctr].initialized) {
            break;
        }
//        printf("=======[ Input Device #%d ]=======\n", ctr+1);
//        printf("Description: %s\n", pa_input_devicelist[ctr].description);
//        printf("Name: %s\n", pa_input_devicelist[ctr].name);
//        printf("Index: %d\n", pa_input_devicelist[ctr].index);
//        printf("\n");
    }

//    return a.exec();
}
