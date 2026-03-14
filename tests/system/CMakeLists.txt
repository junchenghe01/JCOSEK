if(NOT SYS_TEST)
    message(FATAL_ERROR "Please specify SYS_TEST name!")
endif()
add_subdirectory(${SYS_TEST})
