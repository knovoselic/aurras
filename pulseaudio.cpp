#include "pulseaudio.h"

PulseAudio::PulseAudio()
{
    pa_operation *op;
    // Create a mainloop API and connection to the default server
    mainloop = pa_threaded_mainloop_new();
    Q_ASSERT(pa_threaded_mainloop_start(mainloop) == 0);

    mainloop_api = pa_threaded_mainloop_get_api(mainloop);
    context = pa_context_new(mainloop_api, "Aurras");

    pa_threaded_mainloop_lock(mainloop);
    pa_context_set_state_callback(context, pa_state_cb, mainloop);
    Q_ASSERT(pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL) == 0);
    // Here we're waiting for pa_context to connecto to the running server. Once that happenes, the
    // callback function will go into PA_CONTEXT_READY state and will send a signal to which
    // unblocks this waiting call.
    // Not sure if waiting inside of a while loop is needed for context_connect, but we are doing
    // it to be safe, as per docs: https://freedesktop.org/software/pulseaudio/doxygen/threaded_mainloop.html
    while(pa_context_get_state(context) != PA_CONTEXT_READY) {
        pa_threaded_mainloop_wait(mainloop);
    }

    // Set up event subscription. PA will call our callback (pa_subscribe_cb) any time
    // an event happens that matches our mask - in our case on any source device event
    pa_context_set_subscribe_callback(context, pa_subscribe_cb, this);
    op = pa_context_subscribe(context, PA_SUBSCRIPTION_MASK_SOURCE, NULL, NULL);
    Q_ASSERT(op);
    pa_operation_unref(op);
    pa_threaded_mainloop_unlock(mainloop);

    pa_update_source_list_blocking();
}

PulseAudio::~PulseAudio()
{
    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_threaded_mainloop_stop(mainloop);
    pa_threaded_mainloop_free(mainloop);
}

void PulseAudio::pa_state_cb(pa_context *c, void *userdata) {
    pa_context_state_t state;
    pa_threaded_mainloop *m = static_cast<pa_threaded_mainloop*>(userdata);

    state = pa_context_get_state(c);
    qDebug() << __FUNCTION__ << QThread::currentThreadId() << state;
    switch  (state) {
    // There are more states than we care about, we'll just ignore them
    default:
        break;
    case PA_CONTEXT_FAILED:
        Q_ASSERT_X(false, __FUNCTION__, "Connection to PulseAudio has failed.");
        break;
    case PA_CONTEXT_TERMINATED:
        qDebug() << "PulseAudio connection has been terminated.";
        break;
    case PA_CONTEXT_READY:
        qDebug() << "PulseAudio connection is ready.";
        pa_threaded_mainloop_signal(m, 0);
        break;
    }
}

void PulseAudio::pa_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
    Q_UNUSED(c);

    PulseAudio *instance = static_cast<PulseAudio *>(userdata);

    qDebug() << __FUNCTION__ << QThread::currentThreadId();
    // interesting, will get source IDX and when has happened (new source, removed source or property change)
    qDebug() << "got something" << idx;

    Q_ASSERT_X((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SOURCE,
               __FUNCTION__, "Received unexpected event");

    switch((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK)) {
    case PA_SUBSCRIPTION_EVENT_NEW:
    case PA_SUBSCRIPTION_EVENT_REMOVE: {
        // TODO(Kristijan): We should probably automatically update mute state of the new device
        qDebug() << "source add/remove";
        pa_operation *op = instance->pa_update_source_list();
        Q_ASSERT(op);
        pa_operation_unref(op);
        break;
    }
    case PA_SUBSCRIPTION_EVENT_CHANGE: {
        qDebug() << "source property changed";
        int i;
        for(i = 0; i < instance->inputDeviceCount; ++i) {
            if (instance->inputDevices[i].index == idx) break;
        }
        qDebug() << "Old value:" << instance->inputDevices[i].mute;
        pa_operation *op = pa_context_set_source_mute_by_index(c, idx,
                                                               instance->inputDevices[i].mute,
                                                               NULL, NULL);
        Q_ASSERT(op);
        pa_operation_unref(op);
        break;
    }
    }

    qDebug();
}

// pa_mainloop will call this function when it's ready to tell us about a source.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void PulseAudio::pa_source_list_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata) {
    Q_UNUSED(c);

    PulseAudio *instance = static_cast<PulseAudio*>(userdata);

    if (eol) {
        pa_threaded_mainloop_signal(instance->mainloop, 0);
        instance->print_source_list();

        return;
    }

    strncpy(instance->inputDevices[instance->inputDeviceCount].name, l->name, sizeof(instance->inputDevices[0].name) / sizeof(char) - 1);
    instance->inputDevices[instance->inputDeviceCount].index = l->index;
    instance->inputDevices[instance->inputDeviceCount].mute = l->mute;

    ++instance->inputDeviceCount;
}

void PulseAudio::pa_mute_cb(pa_context *c, int success, void *userdata) {
    Q_UNUSED(c);

    qDebug() << __FUNCTION__ << QThread::currentThreadId();
    Q_ASSERT(success);

    PulseAudio *instance = static_cast<PulseAudio*>(userdata);

    pa_threaded_mainloop_signal(instance->mainloop, 0);
}

void PulseAudio::setInputDeviceMuteByIndex(uint32_t index, int mute) {
    pa_operation *op;

    pa_threaded_mainloop_lock(mainloop);
    op = pa_context_set_source_mute_by_index(context, index, mute, pa_mute_cb, this);
    Q_ASSERT(op);
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
        pa_threaded_mainloop_wait(mainloop);
    }
    pa_operation_unref(op);
    pa_threaded_mainloop_unlock(mainloop);
}

void PulseAudio::setAllInputDevicesMute(int mute) {
    for(int i = 0; i < inputDeviceCount; ++i) {
        // we need to update our own sink mute state before starting the PA operations
        // otherwise our change callback will compare the new mute state with our old, outdated
        // state and revert the change
        inputDevices[i].mute = mute;
        setInputDeviceMuteByIndex(inputDevices[i].index, mute);
    }
}

pa_operation* PulseAudio::pa_update_source_list() {
    memset(inputDevices, 0, sizeof(pa_devicelist_t) * MAX_DEVICES);
    inputDeviceCount = 0;

    return pa_context_get_source_info_list(context, pa_source_list_cb, this);
}

void PulseAudio::pa_update_source_list_blocking() {
    pa_operation *op;

    pa_threaded_mainloop_lock(mainloop);
    op = pa_update_source_list();
    Q_ASSERT(op);
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
        pa_threaded_mainloop_wait(mainloop);
    }
    pa_operation_unref(op);
    pa_threaded_mainloop_unlock(mainloop);
}
