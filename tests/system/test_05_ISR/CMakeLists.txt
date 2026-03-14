add_executable(JCOSEK_Test_05_ISR 
                main.c
                isr.c
                uart.c)

# 链接各个模块库
target_link_libraries(JCOSEK_Test_05_ISR PRIVATE JCOSEK PORTABLE STARTUP)
# 应用链接脚本
get_property(ACTUAL_LD_PATH GLOBAL PROPERTY GLOBAL_LINKER_SCRIPT)
# message(STATUS "DEBUG: PLATFORM_LINKER_SCRIPT IS [${ACTUAL_LD_PATH}]")
target_link_options(JCOSEK_Test_05_ISR PRIVATE "-T${ACTUAL_LD_PATH}")
# target_link_options(JCOSEK.elf PRIVATE "-T${PLATFORM_LINKER_SCRIPT}")

# 设置输出后缀
# set_target_properties(JCOSEK.elf PROPERTIES SUFFIX ".elf")
set_target_properties(JCOSEK_Test_05_ISR PROPERTIES 
    OUTPUT_NAME "JCOSEK_Test_05_ISR"
    SUFFIX ".elf"
)

add_custom_command(TARGET JCOSEK_Test_05_ISR POST_BUILD
    # 1. 生成 bin 到 build 目录
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:JCOSEK_Test_05_ISR> ${CMAKE_BINARY_DIR}/JCOSEK_Test_05_ISR.bin
    # # 2. 拷贝 bin 到源码根目录
    # COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/JCOSEK_Test_05_ISR.bin ${CMAKE_SOURCE_DIR}/build/JCOSEK_Test_05_ISR.bin
    # 3. 拷贝 elf 到源码根目录
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:JCOSEK_Test_05_ISR> ${CMAKE_BINARY_DIR}/JCOSEK_Test_05_ISR.elf
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:JCOSEK_Test_05_ISR> > ${CMAKE_BINARY_DIR}/JCOSEK_Test_05_ISR.list
    COMMENT ">>> Artifacts [ELF/BIN] copied to ${CMAKE_BINARY_DIR}"
)