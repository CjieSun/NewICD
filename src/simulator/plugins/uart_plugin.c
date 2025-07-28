#include "../plugin_interface.h"
#include "../../common/register_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// 声明外部函数
extern int trigger_interrupt(const char *module, uint32_t irq_num);

// 前向声明
static simulator_plugin_t* create_uart_plugin_instance(const char *instance_name, int instance_id);

// UART私有数据
typedef struct {
    uint32_t tx_reg;
    uint32_t rx_reg;
    uint32_t status_reg;
    uint32_t ctrl_reg;
    uint32_t dma_ctrl_reg;  // DMA控制寄存器
    bool tx_ready;
    bool rx_ready;
    uint8_t rx_buffer[256];
    int rx_head, rx_tail;
    bool interrupt_enabled;
    bool simulation_running;
    pthread_t monitor_thread;
    
    // 实例标识和地址配置
    int instance_id;
    char instance_name[32];
    uint32_t base_addr;        // 实例基地址
} uart_private_t;

// UART状态监控线程
static void* uart_monitor_thread(void *arg) {
    simulator_plugin_t *plugin = (simulator_plugin_t*)arg;
    uart_private_t *priv = (uart_private_t*)plugin->private_data;
    
    printf("[uart_plugin.c:%s] %s UART monitor thread started\n", __func__, priv->instance_name);
    
    int cycle_count = 0;
    while (priv->simulation_running) {
        sleep(1);  // 每1秒检查一次
        cycle_count++;
        
        if (priv->interrupt_enabled && priv->ctrl_reg & 0x01) {
            // 每5秒模拟一次接收数据
            if (cycle_count % 5 == 0 && priv->rx_head == priv->rx_tail) {
                printf("[uart_plugin.c:%s] %s simulating RX data available (cycle %d)\n", 
                       __func__, priv->instance_name, cycle_count);
                priv->rx_buffer[priv->rx_head] = 0x41 + (cycle_count / 5 - 1) % 26;  // 模拟接收字符A-Z循环
                priv->rx_head = (priv->rx_head + 1) % 256;
                priv->status_reg |= UART_RX_READY;
                
                // 触发接收中断
                trigger_interrupt(priv->instance_name, 6);
            }
        }
    }
    
    printf("[uart_plugin.c:%s] %s UART monitor thread stopped\n", __func__, priv->instance_name);
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
        priv->dma_ctrl_reg = 0;  // 复位DMA控制寄存器
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
    
    // 转换为相对地址
    uint32_t relative_addr = address - priv->base_addr;
    
    switch (relative_addr) {
        case 0x00:  // UART_DR (Data Register)
            // For reads, return received data
            if (priv->rx_head != priv->rx_tail) {
                uint8_t data = priv->rx_buffer[priv->rx_tail];
                priv->rx_tail = (priv->rx_tail + 1) % 256;
                if (priv->rx_head == priv->rx_tail) {
                    priv->status_reg &= ~UART_RX_READY;
                }
                printf("[uart_plugin.c:%s] %s UART read: 0x%02X\n", __func__, priv->instance_name, data);
                return data;
            }
            return 0;
        case 0x04:  // UART_RSR_ECR (Receive Status/Error Clear Register)
            return 0; // No errors for now
        case 0x18:  // UART_FR (Flag Register)
            return priv->status_reg; // Use status_reg as flag register
        case 0x20:  // UART_ILPR (IrDA Low Power Register)
            return 0; // Not implemented
        case 0x24:  // UART_IBRD (Integer Baud Rate Register)
            return 0x006E; // Default baud rate divisor
        case 0x28:  // UART_FBRD (Fractional Baud Rate Register)
            return 0x0000; // Default fractional baud rate
        case 0x2C:  // UART_LCR_H (Line Control Register)
            return 0x0070; // Default line control
        case 0x30:  // UART_CR (Control Register)
            return priv->ctrl_reg;
        case 0x34:  // UART_IFLS (Interrupt FIFO Level Select Register)
            return 0x0000; // Default FIFO levels
        case 0x38:  // UART_IMSC (Interrupt Mask Set/Clear Register)
            return 0x0000; // No interrupts masked by default
        case 0x3C:  // UART_RIS (Raw Interrupt Status Register)
            return 0x0000; // No raw interrupts
        case 0x40:  // UART_MIS (Masked Interrupt Status Register)
            return 0x0000; // No masked interrupts
        case 0x48:  // UART_DMACR (DMA Control Register)
            return priv->dma_ctrl_reg;
        // Legacy compatibility offsets
        case 0x08:  // Legacy UART_STATUS_REG offset
            return priv->status_reg;
        case 0x0C:  // Legacy UART_CTRL_REG offset
            return priv->ctrl_reg;
        case 0x10:  // Legacy UART_DMA_CTRL_REG offset
            return priv->dma_ctrl_reg;
        default:
            printf("[uart_plugin.c:%s] %s UART: Invalid read address 0x%08X (relative: 0x%08X)\n", 
                   __func__, priv->instance_name, address, relative_addr);
            return 0;
    }
}

// UART寄存器写
static int uart_reg_write(simulator_plugin_t *plugin, uint32_t address, uint32_t value) {
    uart_private_t *priv = (uart_private_t*)plugin->private_data;
    
    // 转换为相对地址
    uint32_t relative_addr = address - priv->base_addr;
    
    switch (relative_addr) {
        case 0x00:  // UART_DR_REG (Data Register)
            priv->tx_reg = value;
            printf("[uart_plugin.c:%s] %s UART transmit: 0x%02X ('%c')\n", 
                   __func__, priv->instance_name, value & 0xFF, 
                   (value >= 32 && value < 127) ? (char)value : '.');
            // 模拟发送完成，触发TX完成中断
            if (priv->interrupt_enabled && priv->ctrl_reg & 0x01) {
                printf("[uart_plugin.c:%s] %s UART: TX complete interrupt triggered\n", 
                       __func__, priv->instance_name);
                trigger_interrupt(priv->instance_name, 5);
            }
            break;
        case 0x04:  // UART_RSR_ECR (Receive Status/Error Clear Register)
            // Status/Error clear register
            printf("[uart_plugin.c:%s] %s UART: RSR/ECR register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x18:  // UART_FR (Flag Register) - read only
            printf("[uart_plugin.c:%s] %s UART: Warning - write to read-only FR register\n", 
                   __func__, priv->instance_name);
            break;
        case 0x20:  // UART_ILPR (IrDA Low Power Register)
            printf("[uart_plugin.c:%s] %s UART: ILPR register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x24:  // UART_IBRD (Integer Baud Rate Register)
            printf("[uart_plugin.c:%s] %s UART: IBRD register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x28:  // UART_FBRD (Fractional Baud Rate Register)
            printf("[uart_plugin.c:%s] %s UART: FBRD register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x2C:  // UART_LCR_H (Line Control Register)
            printf("[uart_plugin.c:%s] %s UART: LCR_H register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x30:  // UART_CR (Control Register)
            priv->ctrl_reg = value;
            printf("[uart_plugin.c:%s] %s UART control register set: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            
            // 如果UART被启用，启动监控线程
            if ((value & 0x01) && !priv->interrupt_enabled) {
                priv->interrupt_enabled = true;
                priv->simulation_running = true;
                
                if (pthread_create(&priv->monitor_thread, NULL, uart_monitor_thread, plugin) == 0) {
                    printf("[uart_plugin.c:%s] %s UART monitor thread started\n", 
                           __func__, priv->instance_name);
                } else {
                    printf("[uart_plugin.c:%s] %s Failed to create UART monitor thread\n", 
                           __func__, priv->instance_name);
                    priv->interrupt_enabled = false;
                    priv->simulation_running = false;
                }
            } else if (!(value & 0x01) && priv->interrupt_enabled) {
                // 如果UART被禁用，停止监控线程
                priv->simulation_running = false;
                priv->interrupt_enabled = false;
                pthread_join(priv->monitor_thread, NULL);
                printf("[uart_plugin.c:%s] %s UART monitor thread stopped\n", 
                       __func__, priv->instance_name);
            }
            break;
        case 0x34:  // UART_IFLS (Interrupt FIFO Level Select Register)
            printf("[uart_plugin.c:%s] %s UART: IFLS register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x38:  // UART_IMSC (Interrupt Mask Set/Clear Register)
            printf("[uart_plugin.c:%s] %s UART: IMSC register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x44:  // UART_ICR (Interrupt Clear Register)
            printf("[uart_plugin.c:%s] %s UART: ICR register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x48:  // UART_DMACR (DMA Control Register)
            priv->dma_ctrl_reg = value;
            printf("[uart_plugin.c:%s] %s UART DMA control register set: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            
            // 处理DMA控制逻辑
            if (value & UART_DMA_TX_ENABLE) {
                printf("[uart_plugin.c:%s] %s UART DMA TX enabled\n", 
                       __func__, priv->instance_name);
            }
            if (value & UART_DMA_RX_ENABLE) {
                printf("[uart_plugin.c:%s] %s UART DMA RX enabled\n", 
                       __func__, priv->instance_name);
            }
            break;
        // Legacy compatibility offsets (for simplified register access)
        case 0x08:  // Legacy UART_STATUS_REG offset
            priv->status_reg = value;
            printf("[uart_plugin.c:%s] %s UART: Legacy status register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x0C:  // Legacy UART_CTRL_REG offset
            priv->ctrl_reg = value;
            printf("[uart_plugin.c:%s] %s UART: Legacy control register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        case 0x10:  // Legacy UART_DMA_CTRL_REG offset
            priv->dma_ctrl_reg = value;
            printf("[uart_plugin.c:%s] %s UART: Legacy DMA control register write: 0x%08X\n", 
                   __func__, priv->instance_name, value);
            break;
        default:
            printf("[uart_plugin.c:%s] %s UART: Invalid write address 0x%08X (relative: 0x%08X)\n", 
                   __func__, priv->instance_name, address, relative_addr);
            return -1;
    }
    return 0;
}

// UART中断处理
static int uart_interrupt(simulator_plugin_t *plugin, uint32_t irq_num) {
    uart_private_t *priv = (uart_private_t*)plugin->private_data;
    printf("[uart_plugin.c:%s] %s UART interrupt %d triggered\n", 
           __func__, priv->instance_name, irq_num);
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
    priv->dma_ctrl_reg = 0;  // 初始化DMA控制寄存器
    priv->interrupt_enabled = false;
    priv->simulation_running = false;
    
    // 设置实例信息
    strncpy(priv->instance_name, plugin->name, sizeof(priv->instance_name) - 1);
    
    // 从插件名解析实例ID，或使用默认ID
    if (sscanf(plugin->name, "uart%d", &priv->instance_id) != 1) {
        priv->instance_id = 0;  // 默认ID
    }
    
    // 计算基地址：UART基地址 = UART_BASE + (instance_id * 0x1000)
    priv->base_addr = UART_BASE + (priv->instance_id * 0x1000);
    
    printf("[uart_plugin.c:%s] %s configured with base addr 0x%08X\n", 
           __func__, priv->instance_name, priv->base_addr);
    
    plugin->private_data = priv;
    printf("[uart_plugin.c:%s] %s UART plugin initialized\n", 
           __func__, priv->instance_name);
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
            printf("[uart_plugin.c:%s] %s UART monitor thread joined\n", 
                   __func__, priv->instance_name);
        }
        
        free(plugin->private_data);
        plugin->private_data = NULL;
    }
    printf("[uart_plugin.c:%s] UART plugin cleaned up\n", __func__);
}

// 创建UART插件实例
simulator_plugin_t* create_uart_plugin(void) {
    return create_uart_plugin_instance("uart0", 0);
}

// 创建UART插件实例 - 支持多实例
static simulator_plugin_t* create_uart_plugin_instance(const char *instance_name, int instance_id) {
    simulator_plugin_t *plugin = malloc(sizeof(simulator_plugin_t));
    if (!plugin) {
        return NULL;
    }
    
    memset(plugin, 0, sizeof(simulator_plugin_t));
    
    // 设置实例名称
    if (instance_name) {
        strncpy(plugin->name, instance_name, sizeof(plugin->name) - 1);
    } else {
        snprintf(plugin->name, sizeof(plugin->name), "uart%d", instance_id);
    }
    
    plugin->clock = uart_clock;
    plugin->reset = uart_reset;
    plugin->reg_read = uart_reg_read;
    plugin->reg_write = uart_reg_write;
    plugin->interrupt = uart_interrupt;
    plugin->init = uart_init;
    plugin->cleanup = uart_cleanup;
    plugin->private_data = NULL;
    
    printf("[uart_plugin.c:%s] UART plugin '%s' created\n", __func__, plugin->name);
    return plugin;
}

// 创建具有指定基地址的UART插件实例
simulator_plugin_t* create_uart_plugin_with_base_addr(const char *instance_name, int instance_id, uint32_t base_addr) {
    simulator_plugin_t *plugin = create_uart_plugin_instance(instance_name, instance_id);
    if (!plugin) {
        return NULL;
    }
    
    // 初始化插件以设置私有数据
    if (plugin->init && plugin->init(plugin) != 0) {
        free(plugin);
        return NULL;
    }
    
    // 覆盖自动计算的基地址
    uart_private_t *priv = (uart_private_t*)plugin->private_data;
    priv->base_addr = base_addr;
    priv->instance_id = instance_id;
    
    printf("[uart_plugin.c:%s] %s UART configured with custom base addr 0x%08X\n", 
           __func__, instance_name, base_addr);
    
    return plugin;
}

// 创建多实例UART插件
simulator_plugin_t* create_uart_plugin_multi_instance(const char *instance_name, int instance_id) {
    return create_uart_plugin_instance(instance_name, instance_id);
}
