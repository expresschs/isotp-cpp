# isotp-cpp

[English](README.md) | 中文

基于 C++ 的 ISO-TP（ISO 15765-2）协议库，通过 CAN 总线传输最多 4095 字节的报文。协议层参考 [lishen2/isotp-c](https://github.com/lishen2/isotp-c)，并封装了 C++ 接口与 CAN 设备抽象层。

## 特性

- 单帧（SF）/ 首帧（FF）/ 连续帧（CF）/ 流控帧（FC），完整 ISO 15765-2 帧处理
- 流控参数（Block Size、STmin、WFT）与超时、序号（SN）校验
- 设备抽象层，当前支持 ZLG USBCAN2，其他CAN盒加入收发实现即可。

## 快速开始

```cpp
#include "isotp/isotp_wrapper.h"
#include "isotp/isotp_device.h"
#include "devices/device_creator.h"

using namespace isotp;

int main()
{
    isotpDevice dev(DEVICE_CAN_USBCAN2, "0:0", 500 * 1000);  // 设备序号:通道, 波特率
    if (!dev.isValid()) return -1;

    isotp15765 tp(&dev, 0x7E0, 0x7E8);  // 源地址, 目标地址

    isotp_message tx;
    tx.data = { 0x22, 0xF1, 0x90 };

    if (ISOTP_RET_OK == tp.send(tx, 1000)) {
        isotp_message rx;
        if (ISOTP_RET_OK == tp.receive(rx, 2000)) {
            // rx.data 中即为收到的完整报文
        }
    }
    return 0;
}
```

> CAN 总线发送需对端 ACK。`main.cpp` 提供了一个盒子两通道（`"0:0"` 发、`"0:1"` 收）互连的自环测试，含 12 个用例。

## 构建

```bash
cmake -G "Visual Studio 17 2022" -A Win32 -S . -B build
cmake --build build --config Release
```

> `lib/zlgcan.lib` 为 32 位，需用 Win32 架构；运行时需 `zlgcan.dll` 及 `kerneldlls/` 目录与可执行文件同级。

## License

MIT。协议层参考 [lishen2/isotp-c](https://github.com/lishen2/isotp-c)。
