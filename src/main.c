#include "driver/uart_driver.h"
#include "driver/interrupt_manager.h"
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

// 静态寄存器映射表
static const struct {
    uint32_t start_addr;
    uint32_t end_addr;
    const char *module;
} register_mappings[] = {
    {0x40001000, 0x40001050, "uart"},  // UART寄存器区域
    // 可以在这里添加更多模块的寄存器映射
};

// 静态信号映射表
static const struct {
    int signal_num;
    const char *module;
    uint32_t irq_num;
} signal_mappings[] = {
    {34, "uart", 5},  // UART TX中断 (SIGRTMIN)
    {35, "uart", 6},  // UART RX中断 (SIGRTMIN+1)
    // 可以在这里添加更多模块的中断映射
};

#define REGISTER_MAPPING_COUNT (sizeof(register_mappings) / sizeof(register_mappings[0]))
#define SIGNAL_MAPPING_COUNT (sizeof(signal_mappings) / sizeof(signal_mappings[0]))

// 外部函数声明
extern simulator_plugin_t* create_uart_plugin(void);
extern int register_plugin(simulator_plugin_t *plugin);
extern int handle_sim_message(const sim_message_t *msg, sim_message_t *response);

// 初始化静态寄存器映射
int init_register_mappings(void) {
    printf("[%s:%s] Initializing static register mappings...\n", __FILE__, __func__);
    
    for (size_t i = 0; i < REGISTER_MAPPING_COUNT; i++) {
        if (add_register_mapping(register_mappings[i].start_addr, 
                                register_mappings[i].end_addr, 
                                register_mappings[i].module) != 0) {
            printf("[%s:%s] Failed to add register mapping for %s\n", 
                   __FILE__, __func__, register_mappings[i].module);
            return -1;
        }
    }
    
    printf("[%s:%s] %zu register mappings initialized\n", __FILE__, __func__, REGISTER_MAPPING_COUNT);
    return 0;
}

// 初始化静态信号映射
int init_signal_mappings(void) {
    printf("[%s:%s] Initializing static signal mappings...\n", __FILE__, __func__);
    
    for (size_t i = 0; i < SIGNAL_MAPPING_COUNT; i++) {
        if (add_signal_mapping(signal_mappings[i].signal_num,
                              signal_mappings[i].module,
                              signal_mappings[i].irq_num) != 0) {
            printf("[%s:%s] Failed to add signal mapping for %s IRQ %d\n", 
                   __FILE__, __func__, signal_mappings[i].module, signal_mappings[i].irq_num);
            return -1;
        }
    }
    
    printf("[%s:%s] %zu signal mappings initialized\n", __FILE__, __func__, SIGNAL_MAPPING_COUNT);
    return 0;
}

// 系统初始化
int simulator_init(void) {
    printf("[%s:%s] IC Simulator initializing...\n", __FILE__, __func__);
    
    // 1. 初始化interrupt manager
    if (interrupt_manager_init() != 0) {
        printf("[%s:%s] Failed to initialize interrupt manager\n", __FILE__, __func__);
        return -1;
    }
    
    // 2. 初始化sim interface
    if (sim_interface_init() != 0) {
        printf("[%s:%s] Failed to initialize sim interface\n", __FILE__, __func__);
        return -1;
    }
    
    // 3. 注册插件
    simulator_plugin_t *uart_plugin = create_uart_plugin();
    if (!uart_plugin || register_plugin(uart_plugin) != 0) {
        printf("[%s:%s] Failed to register UART plugin\n", __FILE__, __func__);
        return -1;
    }
    
    // 4. 初始化静态寄存器映射
    if (init_register_mappings() != 0) {
        printf("[%s:%s] Failed to initialize register mappings\n", __FILE__, __func__);
        return -1;
    }
    
    // 5. 初始化静态信号映射
    if (init_signal_mappings() != 0) {
        printf("[%s:%s] Failed to initialize signal mappings\n", __FILE__, __func__);
        return -1;
    }
    
    // 6. 初始化驱动
    if (uart_init() != 0) {
        printf("[%s:%s] Failed to initialize UART driver\n", __FILE__, __func__);
        return -1;
    }
    
    printf("[%s:%s] IC Simulator initialized successfully\n", __FILE__, __func__);
    return 0;
}

// 系统清理
void simulator_cleanup(void) {
    printf("[%s:%s] IC Simulator cleaning up...\n", __FILE__, __func__);
    
    uart_cleanup();
    interrupt_manager_cleanup();
    sim_interface_cleanup();
    
    printf("[%s:%s] IC Simulator cleanup completed\n", __FILE__, __func__);
}

// 运行测试套件
void run_test_suite(void) {
    printf("[%s:%s] Running test suite...\n", __FILE__, __func__);
    
    test_uart_basic();
    test_uart_interrupt();
    
    printf("[%s:%s] Test suite completed\n", __FILE__, __func__);
}

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
    
    // 初始化系统
    if (simulator_init() != 0) {
        printf("[%s:%s] Failed to initialize simulator\n", __FILE__, __func__);
        return -1;
    }
    
    // 运行测试套件
    run_test_suite();
    
    // 清理系统
    simulator_cleanup();
    
    printf("[%s:%s] IC Simulator Test Completed Successfully!\n", __FILE__, __func__);
    return 0;
}
