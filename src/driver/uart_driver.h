#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>

// UART寄存器地址定义
#define UART_BASE       0x40001000
#define UART_TX_REG     (UART_BASE + 0x00)
#define UART_RX_REG     (UART_BASE + 0x04)
#define UART_STATUS_REG (UART_BASE + 0x08)
#define UART_CTRL_REG   (UART_BASE + 0x0C)

// UART状态位定义
#define UART_TX_READY   (1 << 0)
#define UART_RX_READY   (1 << 1)

// UART驱动接口
int uart_init(void);
int uart_send_byte(uint8_t data);
int uart_receive_byte(uint8_t *data);
int uart_send_string(const char *str);
void uart_interrupt_handler(int sig);
void uart_cleanup(void);

#endif // UART_DRIVER_H
