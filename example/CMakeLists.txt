if(TARGET_PLATFORM STREQUAL "s32k144")
    # S32K144 example — minimal kernel test
    add_executable(JCOSEK_EXAMPLE
        s32k144/main.c)

    target_link_libraries(JCOSEK_EXAMPLE
        PRIVATE
            JCOSEK
            STARTUP
            PORTABLE)

    get_property(ACTUAL_LD_PATH GLOBAL PROPERTY GLOBAL_LINKER_SCRIPT)
    target_link_options(JCOSEK_EXAMPLE PRIVATE "-T${ACTUAL_LD_PATH}")

    set_target_properties(JCOSEK_EXAMPLE PROPERTIES
        OUTPUT_NAME "JCOSEK_EXAMPLE"
        SUFFIX ".elf"
    )

    add_custom_command(TARGET JCOSEK_EXAMPLE POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:JCOSEK_EXAMPLE> ${CMAKE_BINARY_DIR}/JCOSEK_EXAMPLE.bin
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:JCOSEK_EXAMPLE> ${CMAKE_BINARY_DIR}/JCOSEK_EXAMPLE.elf
        COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:JCOSEK_EXAMPLE> > ${CMAKE_BINARY_DIR}/JCOSEK_EXAMPLE.list
        COMMENT ">>> Artifacts [ELF/BIN/LIST] copied to ${CMAKE_BINARY_DIR}"
    )

else()
    # Default: Cortex-A9 / VExpress-A9 example
    add_executable(JCOSEK_EXAMPLE
                    main.c
                    isr.c
                    uart.c)

    # Link required modules to executable
    target_link_libraries(JCOSEK_EXAMPLE
        PRIVATE
            JCOSEK
            STARTUP
            PORTABLE)

    # Apply linker script
    get_property(ACTUAL_LD_PATH GLOBAL PROPERTY GLOBAL_LINKER_SCRIPT)
    target_link_options(JCOSEK_EXAMPLE PRIVATE "-T${ACTUAL_LD_PATH}")

    set_target_properties(JCOSEK_EXAMPLE PROPERTIES
        OUTPUT_NAME "JCOSEK_EXAMPLE"
        SUFFIX ".elf"
    )

    add_custom_command(TARGET JCOSEK_EXAMPLE POST_BUILD
        # 1. Generate BIN in build directory
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:JCOSEK_EXAMPLE> ${CMAKE_BINARY_DIR}/JCOSEK_EXAMPLE.bin
        # 2. Copy ELF to build directory
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:JCOSEK_EXAMPLE> ${CMAKE_BINARY_DIR}/JCOSEK_EXAMPLE.elf
        COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:JCOSEK_EXAMPLE> > ${CMAKE_BINARY_DIR}/JCOSEK_EXAMPLE.list
        COMMENT ">>> Artifacts [ELF/BIN] copied to ${CMAKE_BINARY_DIR}"
    )
endif()