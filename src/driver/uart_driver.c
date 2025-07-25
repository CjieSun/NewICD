#include "uart_driver.h"
#include "interrupt_manager.h"
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
    #define usleep(us) Sleep((us)/1000)
#else
    #include <unistd.h>
#endif

// UART寄存器访问宏
#define UART_TX_REG_PTR     ((volatile uint32_t*)UART_TX_REG)
#define UART_RX_REG_PTR     ((volatile uint32_t*)UART_RX_REG)
#define UART_STATUS_REG_PTR ((volatile uint32_t*)UART_STATUS_REG)
#define UART_CTRL_REG_PTR   ((volatile uint32_t*)UART_CTRL_REG)

static volatile int uart_tx_complete = 0;
static volatile int uart_rx_available = 0;

// UART TX中断处理函数
void uart_tx_interrupt_handler(void) {
    printf("[%s:%s] UART TX interrupt received.\n", __FILE__, __func__);
    uart_tx_complete = 1;
}

// UART RX中断处理函数  
void uart_rx_interrupt_handler(void) {
    printf("[%s:%s] UART RX interrupt received.\n", __FILE__, __func__);
    uart_rx_available = 1;
}

// UART初始化
int uart_init(void) {
    printf("[%s:%s] UART driver initializing...\n", __FILE__, __func__);

    // 注册UART中断处理函数
    if (register_interrupt_handler(5, uart_tx_interrupt_handler) != 0) {
        printf("[%s:%s] Failed to register TX interrupt handler\n", __FILE__, __func__);
        return -1;
    }
    
    if (register_interrupt_handler(6, uart_rx_interrupt_handler) != 0) {
        printf("[%s:%s] Failed to register RX interrupt handler\n", __FILE__, __func__);
        return -1;
    }

    // 配置UART控制寄存器
    *UART_CTRL_REG_PTR = 0x01;  // 启用UART
    
    printf("[%s:%s] UART driver initialized\n", __FILE__, __func__);
    return 0;
}

// 发送单个字节
int uart_send_byte(uint8_t data) {
    // 等待发送就绪
    while ((*UART_STATUS_REG_PTR & UART_TX_READY) == 0) {
        usleep(1000);  // 等待1ms
    }
    
    // 写入发送寄存器
    *UART_TX_REG_PTR = data;
    
    // 等待发送完成中断（可选）
    uart_tx_complete = 0;
    while (!uart_tx_complete) {
        usleep(1000);  // 简化的等待方式
        break;  // 在真实环境中，这里应该等待中断
    }
    
    return 0;
}

// 接收单个字节
int uart_receive_byte(uint8_t *data) {
    if (!data) {
        return -1;
    }
    
    // 等待接收中断或直接检查状态
    int timeout = 10;  // 10次检查后超时
    while (timeout-- > 0) {
        // 先检查中断标志
        if (uart_rx_available) {
            // 读取接收寄存器
            *data = (uint8_t)(*UART_RX_REG_PTR & 0xFF);
            uart_rx_available = 0;  // 清除中断标志
            return 0;
        }
        
        // 然后检查状态寄存器（只检查一次）
        if ((*UART_STATUS_REG_PTR & UART_RX_READY) != 0) {
            // 读取接收寄存器
            *data = (uint8_t)(*UART_RX_REG_PTR & 0xFF);
            return 0;
        }
        
        sleep(1);  // 等待1秒
    }
    
    return -1;  // 超时，没有数据可读
}

// 发送字符串
int uart_send_string(const char *str) {
    if (!str) {
        return -1;
    }
    
    while (*str) {
        if (uart_send_byte(*str) != 0) {
            return -1;
        }
        str++;
    }
    
    return 0;
}

// UART清理
void uart_cleanup(void) {
    // 禁用UART
    *UART_CTRL_REG_PTR = 0x00;
    
    printf("[%s:%s] UART driver cleanup completed\n", __FILE__, __func__);
}
