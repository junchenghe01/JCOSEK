# Hosted x86_64 Windows port: pure C, no assembly required
add_library(PORTABLE
    STATIC
        port.c)

target_include_directories(PORTABLE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
