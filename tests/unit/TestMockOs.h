#ifndef TEST_MOCK_DIO_H
#define TEST_MOCK_DIO_H

#include "Dio.h"

/* Dio_ReadPort 模拟结构 */
typedef struct {
    Dio_PortType port_id;
    Dio_PortLevelType return_value;  /* 预设的返回值 */
    uint32 call_count;             /* 添加调用次数计数器 */
} Dio_ReadPort_Mock;
typedef struct {
    Dio_PortType port_id;
    Dio_PortLevelType port_level;  /* 预设的返回值 */
    uint32 call_count;             /* 添加调用次数计数器 */
} Dio_WritePort_Mock;
/* 全局模拟记录变量 */
extern Dio_ReadPort_Mock dio_read_port_mock;
extern Dio_WritePort_Mock dio_write_port_mock;

/* 设置 Dio_ReadPort 返回值 */
void mock_Dio_ReadPort_SetReturnValue(Dio_PortType PortId, Dio_PortLevelType value);
void mock_Dio_WritePort_SetExpectedValue(Dio_PortType PortId, Dio_PortLevelType value);

/* Dio_ReadPort 模拟函数 - 这是缺失的函数！ */
Dio_PortLevelType mock_Dio_ReadPort(Dio_PortType PortId);
/* Dio_WritePort 模拟函数 - 这是缺失的函数！ */
void mock_Dio_WritePort(Dio_PortType PortId, Dio_PortLevelType Level);

/* 获取调用次数（可选，但推荐） */
uint32 Dio_ReadPort_GetCallCount(void);
uint32 Dio_WritePort_GetCallCount(void);

/* 重置模拟状态（可选，但推荐） */
void reset_Dio_ReadPort_Mock(void);
void reset_Dio_WritePort_Mock(void);

#endif /* TEST_MOCK_DIO_H */