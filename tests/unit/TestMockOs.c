/* test/test_mocks.c */
#include "TestMockDio.h"

/* Dio_ReadPort 模拟记录 */
Dio_ReadPort_Mock dio_read_port_mock = {0, 0, 0};
Dio_WritePort_Mock dio_write_port_mock = {0, 0, 0};
/* 设置 Dio_ReadPort 返回值 */
void mock_Dio_ReadPort_SetReturnValue(Dio_PortType PortId, Dio_PortLevelType value)
{
    dio_read_port_mock.port_id = PortId;
    dio_read_port_mock.return_value = value;
}
void mock_Dio_WritePort_SetExpectedValue(Dio_PortType PortId, Dio_PortLevelType value)
{
    dio_write_port_mock.port_id = PortId;
    dio_write_port_mock.port_level = value;
}

/* ========== 添加缺失的函数 ========== */

/* Dio_ReadPort 模拟函数 */
Dio_PortLevelType mock_Dio_ReadPort(Dio_PortType PortId)
{
    /* 记录调用信息 */
    dio_read_port_mock.port_id = PortId;
    dio_read_port_mock.call_count++;
    
    /* 返回预设的值 */
    return dio_read_port_mock.return_value;
}
/* Dio_WritePort 模拟函数 */
void mock_Dio_WritePort(Dio_PortType PortId, Dio_PortLevelType Level)
{
    /* 记录调用信息 */
    dio_write_port_mock.port_id = PortId;
    dio_write_port_mock.port_level = Level;
    dio_write_port_mock.call_count++;
}

/* 获取调用次数 */
uint32 Dio_ReadPort_GetCallCount(void)
{
    return dio_read_port_mock.call_count;
}
uint32 Dio_WritePort_GetCallCount(void)
{
    return dio_write_port_mock.call_count;
}
/* 重置模拟状态 */
void reset_Dio_ReadPort_Mock(void)
{
    dio_read_port_mock.port_id = 0;
    dio_read_port_mock.return_value = 0;
    dio_read_port_mock.call_count = 0;
}
void reset_Dio_WritePort_Mock(void)
{
    dio_write_port_mock.port_id = 0;
    dio_write_port_mock.port_level = 0;
    dio_write_port_mock.call_count = 0;
}
