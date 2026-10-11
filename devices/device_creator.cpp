#include "devices/can_usbcan2.h"
#include "device_creator.h"

const std::vector<device_t> device_creator::config = {
    {DEVICE_CAN_USBCAN2, \
    [](const std::string& param) -> std::unique_ptr<Idevice> {return std::make_unique<can_usbcan2>(param);}},
    {DEVICE_CAN_PCANUSB, \
    [](const std::string& param) -> std::unique_ptr<Idevice> {return nullptr;}},
};

std::unique_ptr<Idevice> device_creator::create(int type, const std::string& param) {
    for (const auto& cfg : device_creator::config) {
        if (cfg.type == type) {
            return cfg.creator(param);
        }
    }
    return nullptr;
}

