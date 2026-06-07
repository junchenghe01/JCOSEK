add_library(PORTABLE
    STATIC
        portasm.S
        port.c)

target_include_directories(PORTABLE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
