cmake_minimum_required(VERSION 3.14)
project(isotp_cpp CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8 /W3)
endif()

# 关闭 MSVC 对 sprintf/sscanf 等"不安全"函数的弃用告警
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# ISO-TP 协议核心 + 设备层
add_executable(isotp_demo
    main.cpp
    isotp/isotp.cpp
    isotp/isotp_wrapper.cpp
    isotp/isotp_device.cpp
    devices/device_creator.cpp
    devices/can_usbcan2.cpp
)

# lib/zlgcan.h 为 GBK 编码，在 /utf-8 下会产生大量 C4828 告警，对包含它的源文件单独关闭
set_source_files_properties(devices/can_usbcan2.cpp PROPERTIES
    COMPILE_OPTIONS "/wd4828")

# size_t -> uint16_t 截断告警（ISO-TP 单帧最大 4095 字节，转换安全），单独关闭
set_source_files_properties(isotp/isotp_wrapper.cpp PROPERTIES
    COMPILE_OPTIONS "/wd4267")

# 源码以项目根目录为 include 基准
# （内部使用 "isotp/..."、"common/..."、"devices/..."、"lib/..." 相对包含）
target_include_directories(isotp_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 线程支持（std::thread / std::mutex / std::condition_variable）
find_package(Threads REQUIRED)
target_link_libraries(isotp_demo PRIVATE Threads::Threads)

# ZLG CAN 驱动（仅 USBCAN2 设备层需要；运行时需 zlgcan.dll 在可执行文件目录）
if(WIN32)
    target_link_libraries(isotp_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlgcan.lib)
endif()
