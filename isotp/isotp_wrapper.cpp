/******************************************************************************
 * OS Includes
 *****************************************************************************/
#include <iostream>
#include <chrono>
#include <iomanip>
#include <functional>
/******************************************************************************
 * Local Includes
 *****************************************************************************/
#include "isotp_wrapper.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

/******************************************************************************
 * ISOTP Library Specific User Implemented Functions
 *****************************************************************************/
/* user implemented, print debug message */
void isotp_user_debug(const char *message, ...)
{
    std::cout << message << std::endl;
}

/* user implemented, get millisecond */
uint32_t isotp_user_get_ms(void)
{
    using namespace std::chrono;
    return (uint32_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

/******************************************************************************
 * ISOTP Implementation
 *****************************************************************************/
namespace isotp
{
isotp15765::isotp15765(isotp::isotpDevice *dev, unsigned int srcAddr, unsigned int destAddr) :
         device(dev), srcID(srcAddr), destID(destAddr)
{
    if (nullptr == device) {
        throw std::invalid_argument("Device pointer is null.");
    }
    if (!device->isValid()) {
        throw std::invalid_argument("Device is not valid (open/init failed).");
    }
    device->addRoute(destID);
    auto sendFunc = [this](uint32_t id, const uint8_t* payload, uint8_t len) -> int {
        return device->send(id, payload, len);
    };
    auto recvFunc = [this](uint32_t* id, uint8_t* payload, uint8_t* len, uint32_t timeout) -> int {
        return device->receive(destID, id, payload, len, timeout);
    };
    isotp_init_link(&link, sendFunc, recvFunc, \
        srcID, isotpSendBuf, sizeof(isotpSendBuf), isotpRecvBuf, sizeof(isotpRecvBuf));

    pollingThread = std::thread(&isotp15765::pollingFunction, this);
    /* 10ms wait for pollingFunction started and running. */
}

isotp15765::~isotp15765()
{
    exitThread = true;
    if (pollingThread.joinable()) {
        pollingThread.join();
    }
    device->removeRoute(destID);
}

void isotp15765::pollingFunction(void)
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];

    while (!exitThread) {
        auto last_receive_status = link.receive_status; /* link state only mutated by this thread */
        if (0 == link.device_receive(&id, data, &dlc, poll_ms)) {
            if (destID == id) {
                std::unique_lock<std::mutex> lock(mtx);
                isotp_on_can_message(&link, data, dlc);
            }
        }

        std::unique_lock<std::mutex> lock(mtx);
        isotp_poll(&link);

        if (ISOTP_RECEIVE_STATUS_FULL == link.receive_status) {
            if (ISOTP_RET_OK == isotp_receive(&link, receivedPayload, ISOTP_BUFSIZE, &receivedPayloadSize)) {
                std::unique_lock<std::mutex> _lock(receive_mtx);
                receive_result = ISOTP_RET_OK;
                receive_ready = true;
                receive_cv.notify_one();
            }
        } else if (ISOTP_RECEIVE_STATUS_INPROGRESS == last_receive_status && ISOTP_RECEIVE_STATUS_IDLE == link.receive_status) {
            std::unique_lock<std::mutex> _lock(receive_mtx);
            receive_result = ISOTP_RET_ERROR;
            receive_ready = true;
            receive_cv.notify_one();
        }
        if (is_sending_cf) {
            if (ISOTP_SEND_STATUS_INPROGRESS != link.send_status) {
                is_sending_cf = false;
                std::unique_lock<std::mutex> _lock(send_mtx);
                if (ISOTP_SEND_STATUS_IDLE == link.send_status) {
                    send_result = ISOTP_RET_OK;
                } else {
                    send_result = ISOTP_RET_ERROR;
                }
                send_ready = true;
                send_cv.notify_one();
            }
            poll_ms = link.send_st_min;
        } else {
            poll_ms = ISO_TP_DEFAULT_RESPONSE_TIMEOUT;
        }
    }
}

int isotp15765::send(const isotp_message &message, int timeout)
{
    int ret = ISOTP_RET_ERROR;

    if (timeout < 0) {
        timeout = std::numeric_limits<int>::max();
    }
    std::unique_lock<std::mutex> lock(mtx);
    {
        std::unique_lock<std::mutex> _lock(send_mtx);
        send_ready = false;
    }
    ret = isotp_send_with_id(&link, link.send_arbitration_id, message.data.data(), message.data.size());
    if (ISOTP_RET_OK == ret && message.data.size() >= 8) {
        is_sending_cf = true;
        lock.unlock();

        std::unique_lock<std::mutex> _lock(send_mtx);
        if (send_cv.wait_for(_lock, std::chrono::milliseconds(timeout), [this]{ return send_ready; })) {
            ret = send_result;
        }
    }

    return ret;
}

int isotp15765::receive(isotp_message &message, int timeout)
{
    int ret = ISOTP_RET_ERROR;

    if (timeout < 0) {
        timeout = std::numeric_limits<int>::max();
    }
    std::unique_lock<std::mutex> lock(receive_mtx);
    if (receive_cv.wait_for(lock, std::chrono::milliseconds(timeout), [this]{ return receive_ready; })) {
        receive_ready = false;
        ret = receive_result;
        if (ISOTP_RET_OK == ret) {
            lock.unlock(); /* release receive_mtx before taking mtx, so we never hold both
                              -> no inversion with polling thread's order (mtx -> receive_mtx) */
            std::unique_lock<std::mutex> _lock(mtx);
            //message.data.clear();
            //for (int i = 0; i < receivedPayloadSize; i++) {
            //    message.data.push_back(receivedPayload[i]);
            //}
            message.data.assign(receivedPayload, receivedPayload + receivedPayloadSize);
        }
    }

    return ret;
}

}

