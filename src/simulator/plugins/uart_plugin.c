#include "../plugin_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// 声明外部函数
extern int trigger_interrupt(const char *module, uint32_t irq_num);

// UART寄存器定义
#define UART_TX_REG     0x40001000
#define UART_RX_REG     0x40001004
#define UART_STATUS_REG 0x40001008
#define UART_CTRL_REG   0x4000100C

// UART状态位
#define UART_TX_READY   (1 << 0)
#define UART_RX_READY   (1 << 1)

// UART私有数据
typedef struct {
    uint32_t tx_reg;
    uint32_t rx_reg;
    uint32_t status_reg;
    uint32_t ctrl_reg;
    bool tx_ready;
    bool rx_ready;
    uint8_t rx_buffer[256];
    int rx_head, rx_tail;
    bool interrupt_enabled;
    bool simulation_running;
    pthread_t monitor_thread;
} uart_private_t;

// UART状态监控线程
static void* uart_monitor_thread(void *arg) {
    simulator_plugin_t *plugin = (simulator_plugin_t*)arg;
    uart_private_t *priv = (uart_private_t*)plugin->private_data;
    
    printf("[uart_plugin.c:%s] UART monitor thread started\n", __func__);
    
    int cycle_count = 0;
    while (priv->simulation_running) {
        sleep(1);  // 每1秒检查一次
        cycle_count++;
        
        if (priv->interrupt_enabled && priv->ctrl_reg & 0x01) {
            // 每5秒模拟一次接收数据
            if (cycle_count % 5 == 0 && priv->rx_head == priv->rx_tail) {
                printf("[uart_plugin.c:%s] Simulating RX data available (cycle %d)\n", __func__, cycle_count);
                priv->rx_buffer[priv->rx_head] = 0x41 + (cycle_count / 5 - 1) % 26;  // 模拟接收字符A-Z循环
                priv->rx_head = (priv->rx_head + 1) % 256;
                priv->status_reg |= UART_RX_READY;
                
                // 触发接收中断
                trigger_interrupt("uart", 6);
            }
        }
    }
    
    printf("[uart_plugin.c:%s] UART monitor thread stopped\n", __func__);
    return NULL;
}

// UART时钟处理
static int uart_clock(simulator_plugin_t *plugin, clock_action_t action, uint32_t cycles) {
    uart_private_t *priv = (uart_private_t*)plugin->private_data;
    
    switch (action) {
        case CLOCK_TICK:
            // 模拟时钟周期，可以在这里更新UART状态
            priv->tx_ready = true;  // 简化处理，假设发送总是就绪
            priv->status_reg |= UART_TX_READY;
            break;
        case CLOCK_ENABLE:
            printf("[uart_plugin.c:%s] UART clock enabled\n", __func__);
            break;
        case CLOCK_DISABLE:
            printf("[uart_plugin.c:%s] UART clock disabled\n", __func__);
            break;
    }
    return 0;
}

// UART复位处理
static int uart_reset(simulator_plugin_t *plugin, reset_action_t action) {
    uart_private_t *priv = (uart_private_t*)plugin->private_data;
    
    if (action == RESET_ASSERT) {
        printf("[uart_plugin.c:%s] UART reset asserted\n", __func__);
        priv->tx_reg = 0;
        priv->rx_reg = 0;
        priv->status_reg = UART_TX_READY;  // 复位后发送就绪
        priv->ctrl_reg = 0;
        priv->tx_ready = true;
        priv->rx_ready = false;
        priv->rx_head = priv->rx_tail = 0;
    } else {
        printf("[uart_plugin.c:%s] UART reset deasserted\n", __func__);
    }
    return 0;
}

// UART寄存器读
static uint32_t uart_reg_read(simulator_plugin_t *plugin, uint32_t address) {
    uart_private_t *priv = (uart_private_t*)plugin->private_data;
    
    switch (address) {
        case UART_TX_REG:
            return priv->tx_reg;
        case UART_RX_REG:
            if (priv->rx_head != priv->rx_tail) {
                uint8_t data = priv->rx_buffer[priv->rx_tail];
                priv->rx_tail = (priv->rx_tail + 1) % 256;
                if (priv->rx_head == priv->rx_tail) {
                    priv->status_reg &= ~UART_RX_READY;
                }
                printf("[uart_plugin.c:%s] UART read: 0x%02X\n", __func__, data);
                return data;
            }
            return 0;
        case UART_STATUS_REG:
            return priv->status_reg;
        case UART_CTRL_REG:
            return priv->ctrl_reg;
        default:
            printf("[uart_plugin.c:%s] UART: Invalid read address 0x%08X\n", __func__, address);
            return 0;
    }
}

// UART寄存器写
static int uart_reg_write(simulator_plugin_t *plugin, uint32_t address, uint32_t value) {
    uart_private_t *priv = (uart_private_t*)plugin->private_data;
    
    switch (address) {
        case UART_TX_REG:
            priv->tx_reg = value;
            printf("[uart_plugin.c:%s] UART transmit: 0x%02X ('%c')\n", __func__, value & 0xFF, 
                   (value >= 32 && value < 127) ? (char)value : '.');
            // 模拟发送完成，触发TX完成中断
            if (priv->interrupt_enabled && priv->ctrl_reg & 0x01) {
                printf("[uart_plugin.c:%s] UART: TX complete interrupt triggered\n", __func__);
                trigger_interrupt("uart", 5);
            }
            break;
        case UART_RX_REG:
            // RX寄存器通常是只读的
            printf("[%s:%s] UART: Warning - write to RX register\n", __FILE__, __func__);
            break;
        case UART_STATUS_REG:
            // 状态寄存器可能部分可写（清除标志位）
            priv->status_reg = value;
            break;
        case UART_CTRL_REG:
            priv->ctrl_reg = value;
            printf("[%s:%s] UART control register set: 0x%08X\n", __FILE__, __func__, value);
            
            // 如果UART被启用，启动监控线程
            if ((value & 0x01) && !priv->interrupt_enabled) {
                priv->interrupt_enabled = true;
                priv->simulation_running = true;
                
                if (pthread_create(&priv->monitor_thread, NULL, uart_monitor_thread, plugin) == 0) {
                    printf("[%s:%s] UART monitor thread started\n", __FILE__, __func__);
                } else {
                    printf("[%s:%s] Failed to create UART monitor thread\n", __FILE__, __func__);
                    priv->interrupt_enabled = false;
                    priv->simulation_running = false;
                }
            } else if (!(value & 0x01) && priv->interrupt_enabled) {
                // 如果UART被禁用，停止监控线程
                priv->simulation_running = false;
                priv->interrupt_enabled = false;
                pthread_join(priv->monitor_thread, NULL);
                printf("[%s:%s] UART monitor thread stopped\n", __FILE__, __func__);
            }
            break;
        default:
            printf("[%s:%s] UART: Invalid write address 0x%08X\n", __FILE__, __func__, address);
            return -1;
    }
    return 0;
}

// UART中断处理
static int uart_interrupt(simulator_plugin_t *plugin, uint32_t irq_num) {
    printf("[%s:%s] UART interrupt %d triggered\n", __FILE__, __func__, irq_num);
    return 0;
}

// UART初始化
static int uart_init(simulator_plugin_t *plugin) {
    uart_private_t *priv = malloc(sizeof(uart_private_t));
    if (!priv) {
        return -1;
    }
    
    memset(priv, 0, sizeof(uart_private_t));
    priv->status_reg = UART_TX_READY;  // 初始状态发送就绪
    priv->tx_ready = true;
    priv->interrupt_enabled = false;
    priv->simulation_running = false;
    
    plugin->private_data = priv;
    printf("[%s:%s] UART plugin initialized\n", __FILE__, __func__);
    return 0;
}

// UART清理
static void uart_cleanup(simulator_plugin_t *plugin) {
    if (plugin->private_data) {
        uart_private_t *priv = (uart_private_t*)plugin->private_data;
        
        // 停止监控线程
        if (priv->simulation_running) {
            priv->simulation_running = false;
            pthread_join(priv->monitor_thread, NULL);
            printf("[%s:%s] UART monitor thread joined\n", __FILE__, __func__);
        }
        
        free(plugin->private_data);
        plugin->private_data = NULL;
    }
    printf("[%s:%s] UART plugin cleaned up\n", __FILE__, __func__);
}

// 创建UART插件实例
simulator_plugin_t* create_uart_plugin(void) {
    simulator_plugin_t *plugin = malloc(sizeof(simulator_plugin_t));
    if (!plugin) {
        return NULL;
    }
    
    strcpy(plugin->name, "uart");
    plugin->clock = uart_clock;
    plugin->reset = uart_reset;
    plugin->reg_read = uart_reg_read;
    plugin->reg_write = uart_reg_write;
    plugin->interrupt = uart_interrupt;
    plugin->init = uart_init;
    plugin->cleanup = uart_cleanup;
    plugin->private_data = NULL;
    
    return plugin;
}
