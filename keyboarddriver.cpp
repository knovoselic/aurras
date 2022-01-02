#include "keyboarddriver.h"

KeyboardDriver::KeyboardDriver()
{
    if (hid_init()) {
        qFatal("Unable to initialize HID library!");
    }
}

KeyboardDriver::~KeyboardDriver() {
    if (device_handle) {
        hid_close(device_handle);
        device_handle = NULL;
    }
}

void KeyboardDriver::set_hsv(uint8_t hue, uint8_t saturation, uint8_t value, uint16_t duration) {
    rgb_timed_override_command command(hue, saturation, value, duration);

    auto xxxx = qDebug();
    for(size_t i = 0; i < sizeof(command); ++i) {
        uint8_t *pointer = (uint8_t *)&command + i;
        xxxx << QString("%1").arg(*pointer, 1, 16);
    }
    write((uint8_t *)&command, sizeof(command));
}

bool KeyboardDriver::write(const uint8_t *data, size_t length, bool should_retry) {
    if (!device_handle) {
        if (!find_device()) return false;
    }

    int result = hid_write(device_handle, data, length);

    if (result > 0) {
        qDebug() << "Bytes written:" << result;
        return true;
    }

    qDebug() << "Unable to write to the device, error:" << QString::fromWCharArray(hid_error(device_handle));

    if (!should_retry) return false;

    qDebug() << "Retrying";
    hid_close(device_handle);
    device_handle = NULL;
    return write(data, length, false);
}

bool KeyboardDriver::find_device() {
    struct hid_device_info *all_devices, *current;
    all_devices = hid_enumerate(0x0, 0x0);
    current = all_devices;
    while (current) {
        if (current->vendor_id == DEVICE_VID &&
            current->product_id == DEVICE_PID &&
            current->usage == DEVICE_USAGE &&
            current->usage_page == DEVICE_USAGE_PAGE) {
            device_handle = hid_open_path(current->path);
            break;
        }

        current = current->next;
    }
    hid_free_enumeration(all_devices);

    if (!device_handle) {
        qDebug() << "HID device not found";
    }
    return device_handle != NULL;
}
