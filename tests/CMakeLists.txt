# 定义测试类型，默认为空，可以通过 cmake .. -DSYS_TEST=UNIT_TEST 开启
# set(SYS_TEST "" CACHE STRING "Type of test to run")

# if("${SYS_TEST}" STREQUAL "UNIT_TEST")
    # add_subdirectory(unit)
# elseif("${SYS_TEST}" STREQUAL "SYSTEM_TEST")
    add_subdirectory(system)
# endif()

