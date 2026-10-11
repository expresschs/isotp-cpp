#ifndef _DEVICE_CREATOR_H
#define _DEVICE_CREATOR_H

#include <memory>
#include <vector>
#include "Idevice.h"

#define DEVICE_NONE             (0)
#define DEVICE_CAN_USBCAN2      (1)
#define DEVICE_CAN_PCANUSB      (2)

typedef struct  {
    int type;
    std::unique_ptr<Idevice> (*creator)(const std::string&);
} device_t;

class device_creator {
public:
    static const std::vector<device_t> config;
    static std::unique_ptr<Idevice> create(int type, const std::string& param);
};

#endif

