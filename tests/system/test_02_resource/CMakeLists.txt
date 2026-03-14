add_executable(JCOSEK_Test_02_Resource main.c)

# 链接各个模块库
target_link_libraries(JCOSEK_Test_02_Resource PRIVATE BSW_OS BSW_STD PORTABLE_ARCH)
# 应用链接脚本
get_property(ACTUAL_LD_PATH GLOBAL PROPERTY GLOBAL_LINKER_SCRIPT)
# message(STATUS "DEBUG: PLATFORM_LINKER_SCRIPT IS [${ACTUAL_LD_PATH}]")
target_link_options(JCOSEK_Test_02_Resource PRIVATE "-T${ACTUAL_LD_PATH}")
# target_link_options(JCOSEK.elf PRIVATE "-T${PLATFORM_LINKER_SCRIPT}")

# 设置输出后缀
# set_target_properties(JCOSEK.elf PROPERTIES SUFFIX ".elf")
set_target_properties(JCOSEK_Test_02_Resource PROPERTIES 
    OUTPUT_NAME "JCOSEK_Test_02_Resource"
    SUFFIX ".elf"
)

add_custom_command(TARGET JCOSEK_Test_02_Resource POST_BUILD
    # 1. 生成 bin 到 build 目录
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:JCOSEK_Test_02_Resource> ${CMAKE_BINARY_DIR}/JCOSEK_Test_02_Resource.bin
    # # 2. 拷贝 bin 到源码根目录
    # COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/JCOSEK_Test_02_Resource.bin ${CMAKE_SOURCE_DIR}/build/JCOSEK_Test_02_Resource.bin
    # 3. 拷贝 elf 到源码根目录
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:JCOSEK_Test_02_Resource> ${CMAKE_BINARY_DIR}/JCOSEK_Test_02_Resource.elf
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:JCOSEK_Test_02_Resource> > ${CMAKE_BINARY_DIR}/JCOSEK_Test_02_Resource.list
    COMMENT ">>> Artifacts [ELF/BIN] copied to ${CMAKE_BINARY_DIR}"
)