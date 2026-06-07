add_library(STARTUP STATIC startup.c)

# Export linker script path for use by example and test targets
set_property(GLOBAL PROPERTY GLOBAL_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

# Export this directory's include path (for S32K144.h)
target_include_directories(STARTUP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
