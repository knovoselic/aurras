#include "pulseaudio.h"

PulseAudio::PulseAudio()
{
    int pa_ready = 0;

    // Create a mainloop API and connection to the default server
    pa_ml = pa_mainloop_new();
    pa_mlapi = pa_mainloop_get_api(pa_ml);
    pa_ctx = pa_context_new(pa_mlapi, "test");

    // This function connects to the pulse server
    pa_context_connect(pa_ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);


    // This function defines a callback so the server will tell us it's state.
    // Our callback will wait for the state to be ready.  The callback will
    // modify the variable to 1 so we know when we have a connection and it's
    // ready.
    // If there's an error, the callback will set pa_ready to 2
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

    while(pa_ready == 0) {
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }
    // Assert that we were able to get a connection
    assert(pa_ready == 1);
}

PulseAudio::~PulseAudio()
{
    pa_context_disconnect(pa_ctx);
    pa_context_unref(pa_ctx);
    pa_mainloop_free(pa_ml);
}

// This callback gets called when our context changes state. We really only
// care about when it's ready or if it has failed
void PulseAudio::pa_state_cb(pa_context *c, void *userdata) {
    pa_context_state_t state;
    int *pa_ready = (int*) userdata;

    qDebug() << __FUNCTION__ << QThread::currentThreadId();
    state = pa_context_get_state(c);
    switch  (state) {
    // There are just here for reference
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    default:
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        *pa_ready = 2;
        break;
    case PA_CONTEXT_READY:
        *pa_ready = 1;
        break;
    }
}

void PulseAudio::pa_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
    Q_UNUSED(c);
    Q_UNUSED(userdata);

    qDebug() << __FUNCTION__ << QThread::currentThreadId();
    // interesting, will get source IDX and when has happened (new source, removed source or property change)
    qDebug() << "got something" << t << idx;
    if ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SOURCE) {
        qDebug() << "PA_SUBSCRIPTION_EVENT_SOURCE";
    }
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW) {
        qDebug() << "new source added";
    }
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
        qDebug() << "source removed";
    }if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE) {
        qDebug() << "source property changed";
    }

    qDebug() << Qt::endl;
}

// pa_mainloop will call this function when it's ready to tell us about a source.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void PulseAudio::pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata) {
    Q_UNUSED(c);

    qDebug() << __FUNCTION__ << QThread::currentThreadId();
    pa_devicelist_t *pa_devicelist = (pa_devicelist_t*)userdata;
    int ctr = 0;

    if (eol > 0) {
        return;
    }

    for (ctr = 0; ctr < 100; ctr++) {
        if (! pa_devicelist[ctr].initialized) {
            strncpy(pa_devicelist[ctr].name, l->name, 511);
            strncpy(pa_devicelist[ctr].description, l->description, 255);
            pa_devicelist[ctr].index = l->index;
            pa_devicelist[ctr].initialized = 1;
            break;
        }
    }
}

void PulseAudio::pa_mute_cb(pa_context *c, int success, void *userdata) {
    Q_UNUSED(c);
    Q_UNUSED(success);

    int *ops_to_complete = (int*) userdata;

    qDebug() << __FUNCTION__ << QThread::currentThreadId();
    --*ops_to_complete;
    qDebug() << *ops_to_complete;
}

pa_devicelist_t* PulseAudio::GetInputDevices() {
    memset(inputDevices, 0, sizeof(pa_devicelist_t) * MAX_DEVICES);

    this->pa_get_devicelist();
    return inputDevices;
}

int PulseAudio::pa_get_devicelist() {
    // We'll need these state variables to keep track of our requests
    int state = 0;
    int ops_to_complete = 0;

    // Initialize our device lists

    // Now we'll enter into an infinite loop until we get the data we receive
    // or if there's an error
    for (;;) {
        // At this point, we're connected to the server and ready to make
        // requests
        switch (state) {
        // State 0: we haven't done anything yet
        case 0:
            // This sends an operation to the server.  pa_sourcelist_cb is
            // our callback function and a pointer to our devicelist will
            // be passed to the callback. The operation ID is stored in the
            // pa_op variable
            pa_op = pa_context_get_source_info_list(pa_ctx,
                                                    pa_sourcelist_cb,
                                                    inputDevices
                                                    );

            pa_context_set_subscribe_callback(pa_ctx, pa_subscribe_cb, NULL);
            pa_operation_unref(pa_context_subscribe(pa_ctx, PA_SUBSCRIPTION_MASK_SOURCE, NULL, NULL));
            // Update state for next iteration through the loop
            state++;
            break;
        case 1:
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                // test mute
                for (int i = 0; i < 100; i++) {
                    if (! inputDevices[i].initialized) {
                        break;
                    }

                    printf("Index: %d\n", inputDevices[i].index);
                    pa_operation_unref(pa_context_set_source_mute_by_index(pa_ctx, inputDevices[i].index, 0, pa_mute_cb, &ops_to_complete));
                    ++ops_to_complete;
                }
                state++;
            }
            break;
        case 2:
            if (ops_to_complete == 0) {
                // Now we're done, clean up and disconnect and return
                return 0;
            }
            break;
        default:
            Q_ASSERT_X(false, __FUNCTION__, QString("We should never be in this state: %1").arg(state).toStdString().c_str());
        }
        // Iterate the main loop and go again.  The second argument is whether
        // or not the iteration should block until something is ready to be
        // done.  Set it to zero for non-blocking.
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }
}
