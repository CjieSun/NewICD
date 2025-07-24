#include "driver/uart_driver.h"
#include "sim_interface/sim_interface.h"
#include "simulator/plugin_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(s) Sleep((s)*1000)
#else
    #include <unistd.h>
#endif

// 外部函数声明
extern simulator_plugin_t* create_uart_plugin(void);
extern int register_plugin(simulator_plugin_t *plugin);
extern int handle_sim_message(const sim_message_t *msg, sim_message_t *response);

// 测试用例
void test_uart_basic(void) {
    printf("[%s:%s] \n=== UART Basic Test ===\n", __FILE__, __func__);
    
    // 首先启用UART
    printf("[%s:%s] Enabling UART (setting control register)\n", __FILE__, __func__);
    volatile uint32_t *uart_ctrl = (uint32_t*)0x4000100C;
    *uart_ctrl = 0x01;  // 启用UART，这会触发监控线程
    
    sleep(1);  // 给监控线程时间启动
    
    // 发送单个字节
    printf("[%s:%s] Sending byte 0x41 ('A')\n", __FILE__, __func__);
    uart_send_byte(0x41);
    
    sleep(1);  // 等待TX中断
    
    // 发送字符串
    printf("[%s:%s] Sending string \"Hello\"\n", __FILE__, __func__);
    uart_send_string("Hello");
    
    printf("[%s:%s] UART basic test completed\n", __FILE__, __func__);
}

void test_uart_interrupt(void) {
    printf("[%s:%s] \n=== UART Interrupt Test ===\n", __FILE__, __func__);
    
    // 等待RX中断（每5秒触发一次）
    printf("[%s:%s] Waiting for UART RX interrupts (will trigger every 5 seconds)...\n", __FILE__, __func__);
    printf("[%s:%s] Attempting to receive data using driver API...\n", __FILE__, __func__);
    
    // 使用驱动API接收数据
    uint8_t received_data;
    for (int i = 0; i < 2; i++) {
        printf("[%s:%s] Trying to receive byte %d...\n", __FILE__, __func__, i + 1);
        if (uart_receive_byte(&received_data) == 0) {
            printf("[%s:%s] Received byte: 0x%02X ('%c')\n", __FILE__, __func__, received_data, 
                   (received_data >= 32 && received_data < 127) ? (char)received_data : '.');
        } else {
            printf("[%s:%s] No data received (timeout)\n", __FILE__, __func__);
        }
        sleep(3);  // 等待3秒
    }
    
    printf("[%s:%s] UART interrupt test completed\n", __FILE__, __func__);
}

int main(void) {
    printf("[%s:%s] IC Simulator Test Starting...\n", __FILE__, __func__);
    
    // 1. 初始化sim interface
    if (sim_interface_init() != 0) {
        printf("[%s:%s] Failed to initialize sim interface\n", __FILE__, __func__);
        return -1;
    }
    
    // 2. 注册UART插件
    simulator_plugin_t *uart_plugin = create_uart_plugin();
    if (!uart_plugin || register_plugin(uart_plugin) != 0) {
        printf("[%s:%s] Failed to register UART plugin\n", __FILE__, __func__);
        return -1;
    }
    
    // 3. 设置UART寄存器映射
    if (add_register_mapping(0x40001000, 0x40001050, "uart") != 0) {
        printf("[%s:%s] Failed to add UART register mapping\n", __FILE__, __func__);
        return -1;
    }
    
    // 4. 设置UART中断信号映射（使用实时信号，避免与系统默认信号冲突）
    if (add_signal_mapping(34, "uart", 5) != 0) {  // SIGRTMIN
        printf("[%s:%s] Failed to add UART TX signal mapping\n", __FILE__, __func__);
        return -1;
    }
    
    // 添加UART接收中断映射
    if (add_signal_mapping(35, "uart", 6) != 0) {  // SIGRTMIN+1
        printf("[%s:%s] Failed to add UART RX signal mapping\n", __FILE__, __func__);
        return -1;
    }
    
    // 5. 初始化UART驱动
    if (uart_init() != 0) {
        printf("[%s:%s] Failed to initialize UART driver\n", __FILE__, __func__);
        return -1;
    }
    
    // 6. 运行测试用例
    test_uart_basic();
    test_uart_interrupt();
    
    // 7. 清理资源
    uart_cleanup();
    sim_interface_cleanup();
    
    printf("[%s:%s] IC Simulator Test Completed Successfully!\n", __FILE__, __func__);
    return 0;
}
