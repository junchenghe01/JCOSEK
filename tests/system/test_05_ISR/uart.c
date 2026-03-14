#include"uart.h"


// 发送原始字节
void Uart_SendChar(uint8 c) {
    // 等待 TX FIFO 不满 (TXFF 标志位)
    while (UART_FR & (1u << 5)); 
    UART_DR = c;
}

// 发送字符串 (最常用的调试手段)
void Uart_SendString(const char* str) {
    while (str && *str) {
        if (*str == '\n') Uart_SendChar('\r'); // 兼容 QEMU 终端换行
        Uart_SendChar((uint8)*str++);
    }
}
void Uart_SendHex(uint32 val) {
    const char hex_map[] = "0123456789ABCDEF";
    Uart_SendString("0x");
    for (int i = 28; i >= 0; i -= 4) {
        Uart_SendChar(hex_map[(val >> i) & 0x0F]);
    }
}

/**
 * @brief 支持 %x 和 %d 的轻量级打印
 */
void Uart_Printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++; // 跳过 '%'
            if (*format == 'x') {
                uint32_t val = va_arg(args, uint32_t);
                Uart_SendHex(val);
                format++;
            } 
            else if (*format == 'd') {
                int32_t val = va_arg(args, int32_t);
                
                // 处理负数
                if (val < 0) {
                    Uart_SendChar('-');
                    val = -val;
                }
                
                // 处理 0 的特殊情况
                if (val == 0) {
                    Uart_SendChar('0');
                } else {
                    // 简单的十进制拆解（逆序存入临时数组再正序打印）
                    char buf[11]; // int32 最大 10 位 + 结束符
                    int i = 0;
                    while (val > 0) {
                        buf[i++] = (val % 10) + '0';
                        val /= 10;
                    }
                    while (--i >= 0) {
                        Uart_SendChar(buf[i]);
                    }
                }
                format++;
            }
        } else {
            Uart_SendChar(*format);
            if (*format == '\n') Uart_SendChar('\r');
            format++;
        }
    }
    va_end(args);
}
