#ifndef KEYBOARDDRIVER_H
#define KEYBOARDDRIVER_H

#include <QDebug>
#include <QThread>
#include <hidapi/hidapi.h>
#include <arpa/inet.h>

#define DEVICE_VID        0x03A8
#define DEVICE_PID        0xA649
#define DEVICE_USAGE      0x0061
#define DEVICE_USAGE_PAGE 0xFF60
#define REPORT_ID         0x00

enum command_ids {
    COMMAND_ID_RGB_TIMED_OVERRIDE = 0,
};

struct __attribute__((__packed__)) rgb_timed_override_command {
    rgb_timed_override_command(uint8_t hue, uint8_t saturation, uint8_t value, uint16_t duration)
        : hue(hue), saturation(saturation), value(value), duration(duration) {}

    uint8_t report_id = REPORT_ID;
    uint8_t command_id = COMMAND_ID_RGB_TIMED_OVERRIDE;
    uint8_t hue;
    uint8_t saturation;
    uint8_t value;
    uint16_t duration;
};

class KeyboardDriver
{
public:
    KeyboardDriver();
    ~KeyboardDriver();

    void set_hsv(uint8_t hue, uint8_t saturation, uint8_t value, uint16_t duration);
private:
    bool find_device();
    bool write(const uint8_t *data, size_t length, bool should_retry = true);

    hid_device *device_handle = NULL;
};

#endif // KEYBOARDDRIVER_H
