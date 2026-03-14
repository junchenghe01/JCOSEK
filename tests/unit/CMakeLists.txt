cmake_minimum_required(VERSION 3.15)
project(HeRTOS_UnitTests C)

# 设置C标准
set(CMAKE_C_STANDARD 99)
# set(CMAKE_C_STANDARD_REQUIRED ON)

# # 根据编译器类型设置不同的编译选项
# if(MSVC)
#     # MSVC编译器特定的选项
#     message(STATUS "Using MSVC compiler")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")  # 警告等级3
#     # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")  # 将警告视为错误（可选）
#     # 添加更多MSVC警告选项
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996")  # 禁用不安全函数警告
# else()
#     # GCC/Clang编译器选项（原Makefile中的选项）
#     message(STATUS "Using GCC/Clang compiler")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wwrite-strings")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wswitch-default")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunreachable-code")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Winit-self")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-field-initializers")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wundef")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wold-style-definition")
# endif()

# # 设置输出目录
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# # 根据新目录结构设置路径
# set(UNITY_ROOT ${CMAKE_SOURCE_DIR}/third_party/Unity)
# set(UNITY_SRC ${UNITY_ROOT}/src/unity.c)
# set(UNITY_INC ${UNITY_ROOT}/src)

# # 源文件路径
# set(DIO_SRC ${CMAKE_SOURCE_DIR}/src/bsw/Mcal/Dio/Dio.c)
# set(DIO_INC ${CMAKE_SOURCE_DIR}/src/bsw/Mcal/Dio)
# set(TEST_SRC ${CMAKE_SOURCE_DIR}/test/TestMaclDio.c)
# set(RUNNER_SRC ${CMAKE_SOURCE_DIR}/test/test_runners/TestMaclDio_Runner.c)
# set(UNITY_CONFIG ${CMAKE_SOURCE_DIR}/test/unity_config.h)
# set(BSW_INC ${CMAKE_SOURCE_DIR}/src/bsw)
# # 检查关键文件是否存在
# foreach(file ${UNITY_SRC} ${DIO_SRC} ${TEST_SRC} ${RUNNER_SRC})
#     if(NOT EXISTS ${file})
#         message(WARNING "File not found: ${file}")
#     endif()
# endforeach()

# # 添加单元测试宏定义
# target_compile_definitions(test_dio PRIVATE UNIT_TEST)

# 主测试目标
add_executable(test_HeRTOS
    ${CMAKE_SOURCE_DIR}/src/Os_Alarm.c
    ${CMAKE_SOURCE_DIR}/src/Os_Counter.c
    ${CMAKE_SOURCE_DIR}/src/Os_Event.c
    ${CMAKE_SOURCE_DIR}/src/Os_Interrupt.c
    ${CMAKE_SOURCE_DIR}/src/Os_Lifecycle.c
    ${CMAKE_SOURCE_DIR}/src/Os_Resource.c
    ${CMAKE_SOURCE_DIR}/src/Os_Scheduler.c
    ${CMAKE_SOURCE_DIR}/src/Os_Task.c
    ${CMAKE_SOURCE_DIR}/test/test_runners/TestMaclOs_Runner.c
    ${CMAKE_SOURCE_DIR}/test/TestMaclOs.c
    ${CMAKE_SOURCE_DIR}/third_party/Unity/src/unity.c
    ${CMAKE_SOURCE_DIR}/test/TestMockOs.c
)

# 设置包含目录
target_include_directories(test_HeRTOS PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    # ${CMAKE_SOURCE_DIR}/portable/cpu/cortex_a9/gcc
    ${CMAKE_SOURCE_DIR}/third_party/Unity/src        # unity.h, unity_internals.h
    ${CMAKE_SOURCE_DIR}/test  # unity_config.h
)

# # 如果使用MSVC，可能需要禁用特定警告
# if(MSVC)
#     # 禁用Unity可能产生的警告
#     target_compile_options(test_HeRTOS PRIVATE 
#         /wd4100  # 未引用的形参
#         /wd4706  # 条件表达式中的赋值
#         /wd4127  # 条件表达式是常量
#         /wd4996  # 不安全函数警告
#     )
# endif()

# # 添加构建后自动运行测试的目标
# add_custom_target(run_tests
#     COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_HeRTOS$<$<CONFIG:Debug>:>.exe$<$<CONFIG:Release>:>.exe>
#     DEPENDS test_HeRTOS
#     COMMENT "Running HeRTOS unit tests"
#     VERBATIM
# )

# # 显示构建信息
# add_custom_target(info
#     COMMAND ${CMAKE_COMMAND} -E echo "=== Project Structure ==="
#     COMMAND ${CMAKE_COMMAND} -E echo "  Unity:        ${UNITY_ROOT}"
#     COMMAND ${CMAKE_COMMAND} -E echo "  HeRTOS Source:   ${HERTOS_SRC}"
#     COMMAND ${CMAKE_COMMAND} -E echo "  HeRTOS Include:  ${HERTOS_INC}"
#     COMMAND ${CMAKE_COMMAND} -E echo "  Test Source:  ${TEST_SRC}"
#     COMMAND ${CMAKE_COMMAND} -E echo "  Test Runner:  ${RUNNER_SRC}"
#     COMMAND ${CMAKE_COMMAND} -E echo ""
#     COMMAND ${CMAKE_COMMAND} -E echo "=== Build Targets ==="
#     COMMAND ${CMAKE_COMMAND} -E echo "  test_HeRTOS:     Build test executable"
#     COMMAND ${CMAKE_COMMAND} -E echo "  run_tests:    Build and run tests"
#     COMMAND ${CMAKE_COMMAND} -E echo ""
#     COMMAND ${CMAKE_COMMAND} -E echo "=== Current Configuration ==="
#     COMMAND ${CMAKE_COMMAND} -E echo "  Compiler:     ${CMAKE_C_COMPILER_ID}"
#     COMMAND ${CMAKE_COMMAND} -E echo "  Flags:        ${CMAKE_C_FLAGS}"
#     COMMENT "Display project information"
# )

# 添加单元测试宏定义
target_compile_definitions(test_HeRTOS PRIVATE UNIT_TEST)