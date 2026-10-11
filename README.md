# isotp-cpp

[中文](README.zh.md) | English

A C++ implementation of the ISO-TP (ISO 15765-2) protocol, transferring messages up to 4095 bytes over CAN. The protocol layer is based on [lishen2/isotp-c](https://github.com/lishen2/isotp-c), with a C++ wrapper and a CAN device abstraction layer.

## Features

- Single frame (SF) / first frame (FF) / consecutive frame (CF) / flow control frame (FC), full ISO 15765-2 frame handling
- Flow control (Block Size, STmin, WFT), timeouts, and sequence number (SN) checking
- Device abstraction layer; currently supports ZLG USBCAN2. Other CAN adapters can be added by implementing the send/receive interface.

## Quick Start

```cpp
#include "isotp/isotp_wrapper.h"
#include "isotp/isotp_device.h"
#include "devices/device_creator.h"

using namespace isotp;

int main()
{
    isotpDevice dev(DEVICE_CAN_USBCAN2, "0:0", 500 * 1000);  // device:channel, baud rate
    if (!dev.isValid()) return -1;

    isotp15765 tp(&dev, 0x7E0, 0x7E8);  // source address, destination address

    isotp_message tx;
    tx.data = { 0x22, 0xF1, 0x90 };

    if (ISOTP_RET_OK == tp.send(tx, 1000)) {
        isotp_message rx;
        if (ISOTP_RET_OK == tp.receive(rx, 2000)) {
            // rx.data contains the received message
        }
    }
    return 0;
}
```

> CAN transmission requires an ACK from a peer node. `main.cpp` provides a loopback test using two channels of one box (`"0:0"` sends, `"0:1"` receives) with 12 test cases.

## Build

```bash
cmake -G "Visual Studio 17 2022" -A Win32 -S . -B build
cmake --build build --config Release
```

> `lib/zlgcan.lib` is 32-bit, so use the Win32 architecture; at runtime, `zlgcan.dll` and the `kerneldlls/` directory must be placed next to the executable.

## License

MIT. The protocol layer is based on [lishen2/isotp-c](https://github.com/lishen2/isotp-c).
