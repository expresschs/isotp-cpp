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
#include <vector>
/******************************************************************************
 * Local includes
 *****************************************************************************/
#include "isotp_wrapper_defines.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

/******************************************************************************
 * Global variables
 *****************************************************************************/

/******************************************************************************
 * Classes
 *****************************************************************************/
namespace isotp
{

class Iisotp15765
{
public:
    virtual  ~Iisotp15765(){}
    //virtual isotp_message send(const std::vector<uint8_t>& data, bool wait=true) = 0;
    //virtual isotp_message receive(void) = 0;
    virtual int send(const isotp_message &message, int timeout = -1) = 0;
    //virtual int send(int id, const isotp_message &message, int timeout) = 0;
    virtual int receive(isotp_message &message, int timeout = -1) = 0;
};

}
