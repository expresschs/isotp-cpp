/******************************************************************************
 * Name        : isotp_wrapper.h
 * Author      : shashiwakale
 * Version     : 1.0.0
 * Copyright   : Copyright (c) 2024 shashiwakale
 * Description : ISOTP Wrapper Implementation
*****************************************************************************/
#pragma once

/******************************************************************************
 * OS includes
 *****************************************************************************/

/******************************************************************************
 * Local includes
 *****************************************************************************/
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
/******************************************************************************
 * Defines
 *****************************************************************************/
    typedef struct {
        uint8_t type;
        uint32_t acc_code;
        uint32_t acc_mask;
        uint8_t filter;
        uint8_t mode;
    } can_cfg;

/******************************************************************************
 * Global variables
 *****************************************************************************/

/******************************************************************************
 * Classes
 *****************************************************************************/

enum {
    CAN_CMD_NONE = 0,
    CAN_CMD_START,
    CAN_CMD_BAUD,
    CAN_CMD_FILTER,
    CAN_CMD_RESET,
    CAN_CMD_SAMPLE_POINT,
};

enum {
    IDEVICE_RET_OK = 0,
    IDEVICE_RET_ERROR,
};

inline void IdeviceLog(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    fflush(stdout);
}

class Idevice
{
public:
    virtual ~Idevice(){}
    virtual bool isValid(void) = 0;
    virtual int control(int cmd, void *arg) = 0;
    virtual int transmit(uint32_t id, const uint8_t *payload, uint8_t len) = 0;
    virtual int receive(uint32_t *id, uint8_t *payload, uint8_t *len, int timeout) = 0;
};


