add_library(HeRTOS 
    STATIC 
        Os_Alarm.c
        Os_Counter.c
        Os_Event.c
        Os_Interrupt.c
        Os_Lifecycle.c
        Os_Resource.c
        Os_Scheduler.c
        Os_Task.c)

# 1. 导出 Os 自己的头文件路径
target_include_directories(HeRTOS 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include)

# 2. 关键：链接 Std 库。这样编译 Os.c 时能找到 Std 的头文件，
# 且后续链接 BSW_OS 的目标也会自动获得 BSW_STD 的头文件路径。
target_link_libraries(HeRTOS 
    PUBLIC 
        STARTUP 
        PORTABLE)
