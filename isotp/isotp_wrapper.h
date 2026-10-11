#pragma once

/******************************************************************************
 * OS includes
 *****************************************************************************/
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
/******************************************************************************
 * Local includes
 *****************************************************************************/
#include "isotp.h"
#include "isotp_device.h"
#include "Iisotp.h"
#include "isotp_wrapper_defines.h"

/******************************************************************************
 * Classes
 *****************************************************************************/
namespace isotp
{

class isotp15765 : public Iisotp15765
{
public:
    isotp15765(isotp::isotpDevice *dev, unsigned int srcAddr, unsigned int destAddr);
    ~isotp15765();
    int send(const isotp_message &message, int timeout = -1) override;
    //int send(int id, const isotp_message &message, int timeout) override;
    int receive(isotp_message &message, int timeout = -1) override;

private:
    isotpDevice *device;

    std::mutex mtx;
    unsigned int srcID;
    unsigned int destID;
    bool exitThread = false;
    std::thread pollingThread;
    int poll_ms = ISO_TP_DEFAULT_RESPONSE_TIMEOUT;

    /* IsoTpLink statically in RAM */
    IsoTpLink link;

    /* send and receive buffer statically in RAM */
    uint8_t isotpRecvBuf[ISOTP_BUFSIZE];
    uint8_t isotpSendBuf[ISOTP_BUFSIZE];
    uint8_t receivedPayload[ISOTP_BUFSIZE];
    uint16_t receivedPayloadSize = 0;

    /* is sending consecutive_frame */
    bool is_sending_cf = false;

    /* isotp layer send ready */
    std::mutex send_mtx;
    std::condition_variable send_cv;
    bool send_ready = false;
    /* real result of the last multi-frame send: ISOTP_RET_OK (finished) or
     * ISOTP_RET_ERROR (failed: timeout / FC overflow / WFT overrun ...),
     * polled out of link.send_status by the polling thread */
    int send_result = ISOTP_RET_ERROR;

    /* isotp layer receive ready */
    std::mutex receive_mtx;
    std::condition_variable receive_cv;
    bool receive_ready = false;
    /* published together with receive_ready: OK = full message, ERROR = aborted midway */
    int receive_result = ISOTP_RET_ERROR;

    void pollingFunction(void);
};

}

