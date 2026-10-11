#include <stdio.h>
#include "common/BlockingQueue.h"
#include "devices/device_creator.h"
#include "isotp_defines.h"
#include "isotp_device.h"

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t payload[8];;
} canFrame;


namespace isotp
{
//type:USBCAN2 dev:"0:0" baud:500 * 1000
isotpDevice::isotpDevice(int type, const std::string& name, int baud)
{
    std::unique_ptr<Idevice> dev = device_creator::create(type, name);
    if (nullptr == dev || !dev->isValid()) {
        IdeviceLog("isotpDevice: device create/open failed (type:%d name:%s).\n", type, name.c_str());
        return;
    }
    if (IDEVICE_RET_OK != dev->control(CAN_CMD_BAUD, &baud)) {
        IdeviceLog("isotpDevice: set baud %d failed.\n", baud);
        return;
    }
    if (IDEVICE_RET_OK != dev->control(CAN_CMD_START, NULL)) {
        IdeviceLog("isotpDevice: start can channel failed.\n");
        return;
    }
    device = std::move(dev);
    valid = true;
    pollingThread = std::thread(&isotpDevice::pollingFunction, this);
}

isotpDevice::~isotpDevice()
{
    //delete device;
    exitThread = true;
    if (pollingThread.joinable()) {
        pollingThread.join();
    }
}

void isotpDevice::addRoute(uint32_t uid)
{
    if (umap.find(uid) != umap.end()) {
        IdeviceLog("isotpDevice: route for id 0x%x already exists, keep existing.\n", uid);
        return;
    }
    umap[uid] = std::make_unique<BlockingQueue<frame>>();
}

void isotpDevice::removeRoute(uint32_t uid)
{
    umap.erase(uid);
}

int isotpDevice::send(const uint32_t id, const uint8_t *payload, const uint8_t len)
{
    if (!valid) {
        return ISOTP_RET_ERROR;
    }
    /* Normalize the device-layer error code (IDEVICE_RET_*) into the protocol
     * error code (ISOTP_RET_*) expected by the ISO-TP layer. */
    return (IDEVICE_RET_OK == device->transmit(id, payload, len))
               ? ISOTP_RET_OK
               : ISOTP_RET_ERROR;
}

int isotpDevice::receive(uint32_t uid, uint32_t *id, uint8_t *payload, uint8_t *len, const uint32_t timeout)
{
    frame f;

    if (!valid) {
        return ISOTP_RET_ERROR;
    }
    auto it = umap.find(uid);

    if (it == umap.end()) {
        return ISOTP_RET_ERROR;
    }
    if (it->second->take(f, timeout)) {
        *id = f.id;
        *len = f.dlc;
        memcpy(payload, f.payload, f.dlc);
        return ISOTP_RET_OK;
    }

    return ISOTP_RET_ERROR;
}

void isotpDevice::pollingFunction(void)
{
    uint32_t id;
    uint8_t payload[8];
    uint8_t len;
    frame f;

    while (!exitThread) {
        if (0 == device->receive(&id, payload, &len, 100)) {
            f.id = id;
            f.dlc = len;
            memcpy(f.payload, payload, len);

            std::unique_lock<std::mutex> lock(mtx);
            auto it = umap.find(f.id);
            if (it != umap.end()) {
                it->second->put(f);
            }
            lock.unlock();
        }
    }
}

}

