/*
 * isotp_wrapper_defines.h
 *
 *  Created on: 06-Dec-2023
 *      Author: shash
 */

#pragma once

#include <vector>

namespace isotp
{
#define ISOTP_BUFSIZE 4096

/*isotp message*/
struct isotp_message
{
    void *unused;
    std::vector<unsigned char> data;
};
}
