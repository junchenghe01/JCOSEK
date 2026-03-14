cmake_minimum_required(VERSION 3.10)
project(RTOS VERSION 1.0.0 LANGUAGES C ASM)

# 设置交叉编译工具链（根据实际情况调整）
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 工具链设置（示例，请根据你的工具链修改）
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_LINKER arm-none-eabi-ld)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)

# 编译选项
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 目标平台特定选项
add_compile_options(
    -mcpu=cortex-a9
    # -mthumb
    # -mfloat-abi=hard
    # -mfpu=vfpv3
    # -ffunction-sections
    # -fdata-sections
    # -Wall
    # -Wextra
    # -Wpedantic
    -g
    # -O0
    -marm
    -O0
    -ffreestanding
    -fno-omit-frame-pointer
)

# 链接选项
add_link_options(
    -mcpu=cortex-a9
    -marm 
    # -mthumb
    # -mfloat-abi=hard
    # -mfpu=vfpv3
    -nostartfiles
    -nodefaultlibs
    -nostdlib
    # -Wl,--gc-sections
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    # -specs=nano.specs
    # -specs=nosys.specs
    -T/home/jche/JCOSEK/src/portable/vexpress_a9/linker.ld

)

# 包含目录
include_directories(
    /home/jche/JCOSEK/src/bsw
    /home/jche/JCOSEK/src/bsw/Os
    /home/jche/JCOSEK/src/bsw/Std
    /home/jche/JCOSEK/src/portable/vexpress_a9
)

# 源文件分组
set(OS_SOURCES
    /home/jche/JCOSEK/src/bsw/Os/Os.c
)

set(PORTABLE_SOURCES
    /home/jche/JCOSEK/src/portable/vexpress_a9/main.c
    # /home/jche/JCOSEK/src/portable/vexpress_a9/portable.S
    /home/jche/JCOSEK/src/portable/vexpress_a9/startup.S
)

set(HEADER_FILES
    /home/jche/JCOSEK/src/bsw/Os/Os.h
    /home/jche/JCOSEK/src/bsw/Std/Std_Types.h
    /home/jche/JCOSEK/src/bsw/Compiler.h
    /home/jche/JCOSEK/src/bsw/Platform_Types.h
    /home/jche/JCOSEK/src/portable/vexpress_a9/portable.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${OS_SOURCES}
    ${PORTABLE_SOURCES}
)

# 设置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".elf"
    # LINK_DEPENDS /home/jche/project/JCOSEK/src/portable/vexpress_a9/linker.ld
    LINK_DEPENDS /home/jche/JCOSEK/src/portable/vexpress_a9/linker.ld
)

# 添加自定义目标用于生成二进制文件
add_custom_target(${PROJECT_NAME}.bin ALL
    DEPENDS ${PROJECT_NAME}
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generating binary file"
)

# # 添加自定义目标用于生成反汇编
# add_custom_target(${PROJECT_NAME}.dis ALL
#     DEPENDS ${PROJECT_NAME}
#     COMMAND ${CMAKE_OBJDUMP} -d $<TARGET_FILE:${PROJECT_NAME}> > ${PROJECT_NAME}.dis
#     COMMENT "Generating disassembly"
# )

# # 安装规则（可选）
# install(TARGETS ${PROJECT_NAME}
#     DESTINATION bin
# )

# install(FILES ${HEADER_FILES}
#     DESTINATION include
# )

# # 测试配置（可选）
# enable_testing()
# add_test(NAME memory_check
#     COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${PROJECT_NAME}>
# )

# # 添加自定义命令以显示构建信息
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E echo "Build completed: $<TARGET_FILE:${PROJECT_NAME}>"
#     COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${PROJECT_NAME}> | grep -E "^\s*\d+\s*\."
#     COMMENT "Displaying section sizes"
# )
