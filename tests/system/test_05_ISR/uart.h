#ifndef  _UART_H_
#define  _UART_H_
#include <stdarg.h>
#include <stdint.h>
#include "Platform_Types.h"
#define UART0_BASE  0x10009000
#define UART_DR     (*(volatile uint32*)(UART0_BASE + 0x00))
#define UART_FR     (*(volatile uint32*)(UART0_BASE + 0x18))


void Uart_SendChar(uint8 c);
void Uart_SendString(const char* str);
void Uart_SendHex(uint32 val);
void Uart_Printf(const char* format, ...);
#endif /* _UART_H_ */