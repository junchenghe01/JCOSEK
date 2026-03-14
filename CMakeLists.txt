cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project(JCOSEK VERSION 1.0.0 LANGUAGES C ASM)

# 工具链配置
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
# 编译选项
add_compile_options(
    -mcpu=cortex-a9 
    -marm 
    -g 
    -O0 
    -ffreestanding 
    -fno-omit-frame-pointer)

# 链接选项
# 项目需要链接标准库，可以在全局编译选项中添加 --specs=nosys.specs，这会提供一套空的系统调用桩函数，解决 undefined reference to _exit 等问题。
add_link_options(
    -mcpu=cortex-a9 
    -marm 
    -nostartfiles 
    -nodefaultlibs 
    -nostdlib 
    --specs=nosys.specs
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map)

# --- 平台选择开关 ---
# 用户可以在命令行指定：cmake -DTARGET_PLATFORM=vexpress_a9 ..
set(TARGET_PLATFORM "vexpress_a9_qemu" CACHE STRING "Select the target platform")

# 根据平台自动映射对应的 CPU 架构
if(TARGET_PLATFORM STREQUAL "vexpress_a9_qemu")
    set(TARGET_CPU "cortex_a9")
    set(TARGET_COMPILER "gcc")
elseif(TARGET_PLATFORM STREQUAL "stm32f103")
    set(TARGET_CPU "cortex_m3")
    set(TARGET_COMPILER "gcc")
endif()
# 包含核心源码
add_subdirectory(portable)
add_subdirectory(src) # 内核源码

add_subdirectory(example)

# # 环境变量：选择测试
set(SYS_TEST "" CACHE STRING "Name of the system test case")
add_subdirectory(tests)


add_subdirectory(docs)
