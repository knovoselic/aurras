#ifndef PULSEAUDIO_H
#define PULSEAUDIO_H

#include <pulse/pulseaudio.h>
#include <QDebug>
#include <QThread> // to remove

typedef struct pa_devicelist {
    char name[512];
    uint32_t index;
    int mute;
} pa_devicelist_t;

class PulseAudio
{
public:
    static const int MAX_DEVICES = 64;

    PulseAudio();
    ~PulseAudio();

    pa_devicelist_t* getInputDevices();
    uint8_t getInputDeviceCount() const;

    void setInputDeviceMuteByIndex(uint32_t index, int mute);
    void setAllInputDevicesMute(int mute);

private:
    static void pa_state_cb(pa_context *c, void *userdata);
    static void pa_source_list_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata);
    static void pa_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
    static void pa_mute_cb(pa_context *c, int success, void *userdata);
    pa_operation* pa_update_source_list();
    void pa_update_source_list_blocking();

#ifdef QT_DEBUG
    void print_source_list() {
        for(int i = 0; i < inputDeviceCount; ++i) {
            qDebug() << inputDevices[i].index << inputDevices[i].name << inputDevices[i].mute;
        }
    }
#endif

    pa_devicelist_t inputDevices[MAX_DEVICES];
    uint8_t inputDeviceCount = 0;
    pa_threaded_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;
    pa_context *context;
};


#endif // PULSEAUDIO_H
