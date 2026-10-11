#ifndef __CAN_USBCAN2_H_
#define __CAN_USBCAN2_H_

#include <stdint.h>
#include <string>
#include "Idevice.h"

typedef struct {
    union {
        struct {
            uint8_t ch: 2;
            uint8_t id: 6;
        } _dev;
        uint8_t dev;
    } u;
    void *handle;
    void *channel;
} usbcan2;

class can_usbcan2 : public Idevice
{
public:
    can_usbcan2(const std::string& param);
    ~can_usbcan2();
    bool isValid(void) override {
        return valid;
    }
    int control(int cmd, void *arg) override;
    int transmit(uint32_t id, const uint8_t *payload, uint8_t len) override;
    int receive(uint32_t *id, uint8_t *payload, uint8_t *len, int timeout) override;

private:
    bool valid = false;
    usbcan2 can = {0};
};

#endif
