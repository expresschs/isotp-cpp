#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "lib/zlgcan.h"
#include "can_usbcan2.h"

static std::unordered_map<int , usbcan2> devs_umap;

//params: "index:channel"
can_usbcan2::can_usbcan2(const std::string& name)
{
    int id = 0;
    int ch = 0;
    void *hdl = NULL;

    sscanf(name.c_str(), "%d:%d", &id, &ch);
    for (auto& it : devs_umap) {
        if (it.second.u._dev.id == id) {
            if (it.second.u._dev.ch == ch) {
                IdeviceLog("Device is in use.\n");
                valid = false;
                return;
            }
            hdl = it.second.handle;
        }
    }
    if (NULL != hdl) {
        can.handle = hdl;
        can.u._dev.id = id;
        can.u._dev.ch = ch;
        devs_umap[can.u.dev] = can;
    } else {
        hdl = ZCAN_OpenDevice(ZCAN_USBCAN2, id, 0);
        if (INVALID_DEVICE_HANDLE != hdl) {
            can.handle = hdl;
            can.u._dev.id = id;
            can.u._dev.ch = ch;
            devs_umap[can.u.dev] = can;
        } else {
            IdeviceLog("Device open error.\n", id, ch);
            valid = false;
            return;
        }
    }
    valid = true;
}

can_usbcan2::~can_usbcan2()
{
    int id = -1;
    void *handle = NULL;
    bool found = false;

    auto it = devs_umap.find(can.u.dev);
    if (it != devs_umap.end()) {
        if (STATUS_OK != ZCAN_ResetCAN((CHANNEL_HANDLE)it->second.channel)) {
            IdeviceLog("Channel reset error.\n");
        }
        id = it->second.u._dev.id;
        handle = it->second.handle;
        devs_umap.erase(it);
        for (auto& _it : devs_umap) {
            if (_it.second.u._dev.id == id) {
                found = true;
                break;
            }
        }
        if (false == found) {
            if (STATUS_OK != ZCAN_CloseDevice((DEVICE_HANDLE)handle)) {
                IdeviceLog("Device close error.\n");
            }
        }
    }
}

int can_usbcan2::control(int cmd, void *arg)
{
    int ret = IDEVICE_RET_ERROR;

    auto it = devs_umap.find(can.u.dev);
    if (it == devs_umap.end()) {
        return ret;
    }
    switch (cmd) {
        case CAN_CMD_BAUD: {
            ZCAN_CHANNEL_INIT_CONFIG cfg;
            cfg.can_type = 0;
            cfg.can.mode = 0;
            cfg.can.acc_code = 0;
            cfg.can.acc_mask = 0xFFFFFFFF;
            char val[32] = {0};
            char baud[16] = {0};
            snprintf(val, sizeof(val) - 1, "%d/baud_rate", it->second.u._dev.ch);
            snprintf(baud, sizeof(baud) - 1, "%d", *(int *)arg);
            if (STATUS_OK == ZCAN_SetValue((CHANNEL_HANDLE)it->second.handle, val, baud)) {
                it->second.channel = ZCAN_InitCAN((DEVICE_HANDLE)it->second.handle, it->second.u._dev.ch, &cfg);
                if (INVALID_CHANNEL_HANDLE != it->second.channel) {
                    ret = IDEVICE_RET_OK;
                }
            }
            break;
        }
        case CAN_CMD_START:
            if (NULL != (CHANNEL_HANDLE)it->second.channel) {
                if (STATUS_OK == ZCAN_StartCAN((CHANNEL_HANDLE)it->second.channel)) {
                    ret = IDEVICE_RET_OK;
                }
            }
            break;
        case CAN_CMD_RESET:
            if (NULL != (CHANNEL_HANDLE)it->second.channel) {
                if (STATUS_OK == ZCAN_ResetCAN((CHANNEL_HANDLE)it->second.channel)) {
                    ret = IDEVICE_RET_OK;
                }
            }
            break;
    }

    return ret;
}

int can_usbcan2::transmit(uint32_t id, const uint8_t *payload, uint8_t len)
{
    int ret = IDEVICE_RET_ERROR;
    ZCAN_Transmit_Data data;

    data.transmit_type = 0;
    /* IDs above 11 bits (0x7FF) must be sent as extended frames. */
    data.frame.can_id = MAKE_CAN_ID(id, (id > 0x7FF) ? 1 : 0, 0, 0);
    for (int i = 0; i < len; i++) {
        data.frame.data[i] = payload[i];
    }
    data.frame.can_dlc = len;
    auto it = devs_umap.find(can.u.dev);
    if (it != devs_umap.end()) {
        if (1 ==  ZCAN_Transmit((CHANNEL_HANDLE)it->second.channel, &data, 1)) {
            ret = IDEVICE_RET_OK;
        }
#if 0
        using namespace std::chrono;
        printf("%ldms ", duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
        printf("Tx:0x%x ", id);
        for (uint8_t i = 0; i < len; i++) {
            printf("%02x ", payload[i]);
        }
        printf("\n");
        fflush(stdout);
#endif
    }

    return ret;
}

int can_usbcan2::receive(uint32_t *id, uint8_t *payload, uint8_t *len, int timeout)
{
    int ret = IDEVICE_RET_ERROR;
    ZCAN_Receive_Data data;

    auto it = devs_umap.find(can.u.dev);
    if (it != devs_umap.end()) {
        if (1 == ZCAN_Receive((CHANNEL_HANDLE)it->second.channel, &data, 1, timeout)) {
            *id = GET_ID(data.frame.can_id);
            for (uint8_t i = 0; i < data.frame.can_dlc; i++) {
                payload[i] = data.frame.data[i];
            }
            *len = data.frame.can_dlc;
            ret = IDEVICE_RET_OK;
#if 0
            using namespace std::chrono;
            printf("%ldms ", duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
            printf("Rx:0x%x ", *id);
            for (uint8_t i = 0; i < *len; i++) {
                printf("%02x ", payload[i]);
            }
            printf("\n");
            fflush(stdout);
#endif
        }
    }

    return ret;
}


