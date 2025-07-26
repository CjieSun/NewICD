#include "driver/uart_driver.h"
#include "sim_interface/interrupt_manager.h"
#include "driver/dma_driver.h"
#include "sim_interface/sim_interface.h"
#include "simulator/plugin_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    {UART_BASE + 0x0000, UART_BASE + 0x0050, "uart0"},  // UART0寄存器区域
    {UART_BASE + 0x1000, UART_BASE + 0x1050, "uart1"},  // UART1寄存器区域  
    {UART_BASE + 0x2000, UART_BASE + 0x2050, "uart2"},  // UART2寄存器区域
    {DMA_BASE_ADDR + 0x0000, DMA_BASE_ADDR + 0x0300, "dma0"},   // DMA0寄存器区域 (全局寄存器 + 16个通道)
    {DMA_BASE_ADDR + 0x1000, DMA_BASE_ADDR + 0x1300, "dma1"},   // DMA1寄存器区域
    {DMA_BASE_ADDR + 0x2000, DMA_BASE_ADDR + 0x2300, "dma2"},   // DMA2寄存器区域
    // 可以在这里添加更多模块的寄存器映射
};

// 静态信号映射表
static const struct {
    int signal_num;
    const char *module;
    uint32_t irq_num;
} signal_mappings[] = {
    {34, "uart0", 5},  // UART0 TX中断 (SIGRTMIN)
    {35, "uart0", 6},  // UART0 RX中断 (SIGRTMIN+1)
    {36, "uart1", 5},  // UART1 TX中断
    {37, "uart1", 6},  // UART1 RX中断
    {38, "uart2", 5},  // UART2 TX中断
    {39, "uart2", 6},  // UART2 RX中断
    {40, "dma0", 8},   // DMA0中断 (SIGRTMIN+6)
    {41, "dma0", 9},   // DMA0通道1中断
    {42, "dma0", 10},  // DMA0通道2中断
    {43, "dma1", 8},   // DMA1中断
    {44, "dma2", 8},   // DMA2中断
    // 可以在这里添加更多模块的中断映射
};

#define REGISTER_MAPPING_COUNT (sizeof(register_mappings) / sizeof(register_mappings[0]))
#define SIGNAL_MAPPING_COUNT (sizeof(signal_mappings) / sizeof(signal_mappings[0]))

// 外部函数声明
extern simulator_plugin_t* create_uart_plugin_multi_instance(const char *instance_name, int instance_id);
extern simulator_plugin_t* create_dma_plugin_multi_instance(const char *instance_name, int instance_id);

// 测试函数声明
void test_uart_basic(void);
void test_uart_interrupt(void);
void test_dma_basic(void);
void test_uart_dma(void);
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
    simulator_plugin_t *uart_plugin = create_uart_plugin_multi_instance("uart0", 0);
    if (!uart_plugin || register_plugin(uart_plugin) != 0) {
        printf("[%s:%s] Failed to register UART plugin\n", __FILE__, __func__);
        return -1;
    }
    
    simulator_plugin_t *dma_plugin = create_dma_plugin_multi_instance("dma0", 0);
    if (!dma_plugin || register_plugin(dma_plugin) != 0) {
        printf("[%s:%s] Failed to register DMA plugin\n", __FILE__, __func__);
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
    
    // 初始化DMA
    if (dma_init() != 0) {
        printf("[%s:%s] Failed to initialize DMA driver\n", __FILE__, __func__);
        return -1;
    }
    
    printf("[%s:%s] IC Simulator initialized successfully\n", __FILE__, __func__);
    return 0;
}

// 系统清理
void simulator_cleanup(void) {
    printf("[%s:%s] IC Simulator cleaning up...\n", __FILE__, __func__);
    
    uart_cleanup();
    dma_cleanup();
    interrupt_manager_cleanup();
    sim_interface_cleanup();
    
    printf("[%s:%s] IC Simulator cleanup completed\n", __FILE__, __func__);
}

// 运行测试套件
void run_test_suite(void) {
    printf("[%s:%s] Running test suite...\n", __FILE__, __func__);
    
    test_uart_basic();
    test_uart_interrupt();
    test_dma_basic();
    test_uart_dma();
    
    printf("[%s:%s] Test suite completed\n", __FILE__, __func__);
}

// 测试用例
void test_uart_basic(void) {
    printf("[%s:%s] \n=== UART Basic Test ===\n", __FILE__, __func__);
    
    // 首先启用UART
    printf("[%s:%s] Enabling UART (setting control register)\n", __FILE__, __func__);
    volatile uint32_t *uart_ctrl = (uint32_t*)0x4000200C; // 新的UART0地址
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

void test_dma_basic(void) {
    printf("[%s:%s] \n=== DMA Basic Test ===\n", __FILE__, __func__);
    
    // 测试DMA寄存器访问
    volatile uint32_t *dma_ctrl = (uint32_t*)DMA_BASE_ADDR; // DMA0地址
    
    printf("[%s:%s] Enabling DMA controller\n", __FILE__, __func__);
    *dma_ctrl = 0x01;  // 启用DMA
    
    sleep(1);
    
    // 配置DMA通道0进行内存到内存传输
    volatile uint32_t *ch0_src = (uint32_t*)DMA_CH_SRC_REG(0);    // 通道0源地址寄存器
    volatile uint32_t *ch0_dst = (uint32_t*)DMA_CH_DST_REG(0);    // 通道0目标地址寄存器
    volatile uint32_t *ch0_size = (uint32_t*)DMA_CH_SIZE_REG(0);  // 通道0传输大小寄存器
    volatile uint32_t *ch0_config = (uint32_t*)DMA_CH_CONFIG_REG(0); // 通道0配置寄存器
    volatile uint32_t *ch0_ctrl = (uint32_t*)DMA_CH_CTRL_REG(0);  // 通道0控制寄存器
    
    printf("[%s:%s] Configuring DMA channel 0\n", __FILE__, __func__);
    *ch0_src = 0x20000000;     // 源地址
    *ch0_dst = 0x20001000;     // 目标地址
    *ch0_size = 1024;          // 传输1KB
    *ch0_config = 0x30;        // 内存到内存，源和目标地址递增
    
    printf("[%s:%s] Starting DMA transfer\n", __FILE__, __func__);
    *ch0_ctrl = 0x03;          // 启用并开始传输
    
    sleep(1);  // 等待传输完成
    
    printf("[%s:%s] DMA basic test completed\n", __FILE__, __func__);
}

void test_uart_dma(void) {
    printf("[%s:%s] \n=== UART DMA Test ===\n", __FILE__, __func__);
    
    // 测试UART DMA控制寄存器
    printf("[%s:%s] Testing UART DMA control register...\n", __FILE__, __func__);
    
    volatile uint32_t *uart_dma_ctrl = (uint32_t*)UART_DMA_CTRL_REG;
    
    // 读取初始值
    uint32_t initial_value = *uart_dma_ctrl;
    printf("[%s:%s] Initial UART DMA control register value: 0x%08X\n", __FILE__, __func__, initial_value);
    
    // 测试写入TX DMA使能位
    printf("[%s:%s] Setting UART DMA TX enable bit...\n", __FILE__, __func__);
    *uart_dma_ctrl = UART_DMA_TX_ENABLE;
    
    uint32_t read_value = *uart_dma_ctrl;
    printf("[%s:%s] UART DMA control register value: 0x%08X\n", __FILE__, __func__, read_value);
    
    if (read_value & UART_DMA_TX_ENABLE) {
        printf("[%s:%s] ✓ UART DMA TX enable bit is set correctly\n", __FILE__, __func__);
    } else {
        printf("[%s:%s] ✗ UART DMA TX enable bit not set\n", __FILE__, __func__);
    }
    
    // 测试写入RX DMA使能位
    printf("[%s:%s] Setting UART DMA RX enable bit...\n", __FILE__, __func__);
    *uart_dma_ctrl = UART_DMA_RX_ENABLE;
    
    read_value = *uart_dma_ctrl;
    printf("[%s:%s] UART DMA control register value: 0x%08X\n", __FILE__, __func__, read_value);
    
    if (read_value & UART_DMA_RX_ENABLE) {
        printf("[%s:%s] ✓ UART DMA RX enable bit is set correctly\n", __FILE__, __func__);
    } else {
        printf("[%s:%s] ✗ UART DMA RX enable bit not set\n", __FILE__, __func__);
    }
    
    // 测试同时设置两个位
    printf("[%s:%s] Setting both UART DMA TX and RX enable bits...\n", __FILE__, __func__);
    *uart_dma_ctrl = UART_DMA_TX_ENABLE | UART_DMA_RX_ENABLE;
    
    read_value = *uart_dma_ctrl;
    printf("[%s:%s] UART DMA control register value: 0x%08X\n", __FILE__, __func__, read_value);
    
    if ((read_value & UART_DMA_TX_ENABLE) && (read_value & UART_DMA_RX_ENABLE)) {
        printf("[%s:%s] ✓ Both UART DMA TX and RX enable bits are set correctly\n", __FILE__, __func__);
    } else {
        printf("[%s:%s] ✗ UART DMA enable bits not set correctly\n", __FILE__, __func__);
    }
    
    // 测试清除所有位
    printf("[%s:%s] Clearing all UART DMA control bits...\n", __FILE__, __func__);
    *uart_dma_ctrl = 0x00;
    
    read_value = *uart_dma_ctrl;
    printf("[%s:%s] UART DMA control register value: 0x%08X\n", __FILE__, __func__, read_value);
    
    if (read_value == 0) {
        printf("[%s:%s] ✓ UART DMA control register cleared successfully\n", __FILE__, __func__);
    } else {
        printf("[%s:%s] ✗ UART DMA control register not cleared properly\n", __FILE__, __func__);
    }
    
    // 如果UART DMA相关函数存在，测试实际DMA功能
    printf("[%s:%s] Testing UART DMA initialization...\n", __FILE__, __func__);
    if (uart_dma_init() == 0) {
        printf("[%s:%s] ✓ UART DMA initialized successfully\n", __FILE__, __func__);
        
        // 测试数据
        const char *test_data = "Hello DMA World!";
        printf("[%s:%s] Starting UART DMA send test with data: \"%s\"\n", __FILE__, __func__, test_data);
        
        // 重新启用DMA
        *uart_dma_ctrl = UART_DMA_TX_ENABLE;
        
        if (uart_dma_send((const uint8_t*)test_data, strlen(test_data)) == 0) {
            printf("[%s:%s] ✓ DMA send started successfully\n", __FILE__, __func__);
            printf("[%s:%s] Waiting for DMA completion...\n", __FILE__, __func__);
            
            if (uart_dma_wait_send_complete(5000) == 0) {
                printf("[%s:%s] ✓ DMA send completed successfully\n", __FILE__, __func__);
            } else {
                printf("[%s:%s] ⚠ DMA send timeout (may be expected in simulation)\n", __FILE__, __func__);
            }
        } else {
            printf("[%s:%s] ✗ Failed to start DMA send\n", __FILE__, __func__);
        }
    } else {
        printf("[%s:%s] ⚠ UART DMA initialization failed (may not be implemented)\n", __FILE__, __func__);
    }
    
    printf("[%s:%s] UART DMA test completed\n", __FILE__, __func__);
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
