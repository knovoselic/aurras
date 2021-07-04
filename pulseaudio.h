#ifndef PULSEAUDIO_H
#define PULSEAUDIO_H

#include <pulse/pulseaudio.h>
#include <QDebug>
#include <QThread> // to remove

typedef struct pa_devicelist {
    uint8_t initialized;
    char name[512];
    uint32_t index;
    char description[256];
} pa_devicelist_t;

class PulseAudio
{
public:
    static const int MAX_DEVICES = 100;

    PulseAudio();
    ~PulseAudio();

    pa_devicelist_t* GetInputDevices();

private:
    static void pa_state_cb(pa_context *c, void *userdata);
    static void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata);
    static void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata);
    static void pa_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
    static void pa_mute_cb(pa_context *c, int success, void *userdata);
    int pa_get_devicelist();

    pa_devicelist_t inputDevices[MAX_DEVICES];
    pa_mainloop *pa_ml;
    pa_mainloop_api *pa_mlapi;
    pa_operation *pa_op;
    pa_context *pa_ctx;
};


#endif // PULSEAUDIO_H
