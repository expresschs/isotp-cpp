/*
 * isotp_device.h
 *
 *  Description  :
 *  Created on   : 30-Apr-2023
 *      Author   : shashi
 *  Organization : 
 */

#ifndef ISOTP_DEVICE_H_
#define ISOTP_DEVICE_H_

#include <functional>
#include <unordered_map>
#include <vector>
#include <thread>
#include "common/BlockingQueue.h"
#include "devices/Idevice.h"

namespace isotp
{

typedef struct {
    //name open close ioctl read write
    char *name;
    int (*open) (void);
    int (*close) (void);
    int (*ctrl) (void *, int);
    int (*tx) (const uint32_t, const uint8_t *, const uint8_t);
    int (*rx) (uint32_t *, uint8_t *, uint8_t *, const uint32_t);
} deviceOps;

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t payload[8];
} frame;

class isotpDevice
{
public:
    isotpDevice(int type, const std::string& name, int baud);// open and init
    ~isotpDevice(); //close
    bool isValid(void) const { return valid; }

    void addRoute(uint32_t uid);
    void removeRoute(uint32_t uid);
    int send(const uint32_t id, const uint8_t *payload, const uint8_t len);
    int receive(uint32_t uid, uint32_t *id, uint8_t *payload, uint8_t *len, const uint32_t timeout);

private:
    bool valid = false;
    std::unique_ptr<Idevice> device = nullptr;
    /* keyed by rx CAN id, so polling dispatches a frame only to the route
     * registered for that id instead of broadcasting to all routes */
    std::unordered_map<uint32_t, std::unique_ptr<BlockingQueue<frame>>> umap;
    std::mutex mtx;
    bool exitThread = false;
    std::thread pollingThread;
    void pollingFunction(void);
};

} /* namespace isotpDevice */


#endif

