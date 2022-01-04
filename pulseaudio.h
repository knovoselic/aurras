#ifndef PULSEAUDIO_H
#define PULSEAUDIO_H

#include <pulse/pulseaudio.h>
#include <QObject>
#include <QDebug>
#include <QThread> // to remove

struct PulseAudioDevice {
    QString name; // not really necessary, but makes debugging easier
    uint32_t index;

    PulseAudioDevice(const char *name, uint32_t index) :
        name(QString(name)), index(index) {}
};

class PulseAudio : public QObject
{
    Q_OBJECT

public:
    PulseAudio();
    ~PulseAudio();

    void setMuteForAllInputDevices(bool muted);

signals:
    void source_added(quint32 idx);
    void source_removed(quint32 idx);
    void source_updated(quint32 idx);

    void source_output_added(quint32 idx);
    void source_output_removed(quint32 idx);
    void source_output_updated(quint32 idx);
    void active_source_output_count_changed(int new_count);

private:
    static void pa_state_cb(pa_context *c, void *userdata);
    static void pa_source_list_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata);
    static void pa_source_output_list_cb(pa_context *c, const pa_source_output_info *l, int eol, void *userdata);
    static void pa_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
    static void pa_mute_cb(pa_context *c, int success, void *userdata);

    void pa_update_source_list();
    void pa_update_source_output_count();
    void pa_set_source_mute_to_master_mute_by_index(quint32 idx);

#ifdef QT_DEBUG
    void print_source_list() {
        for(int i = 0; i < sources.size(); ++i) {
            qDebug() << sources[i].index << sources[i].name;
        }
    }
#endif

    QList<PulseAudioDevice> sources;
    int active_source_output_count;
    pa_threaded_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;
    pa_context *context;
    int masterMute = 0;
};


#endif // PULSEAUDIO_H
