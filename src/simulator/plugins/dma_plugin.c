#include "../plugin_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include "../../common/register_map.h"

// 解决usleep在一些系统上的声明问题
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// 声明外部函数
extern int trigger_interrupt(const char *module, uint32_t irq_num);

// 前向声明
static simulator_plugin_t* create_dma_plugin_instance(const char *instance_name, int instance_id);

// DMA实例私有数据
typedef struct {
    dma_channel_regs_t channels[16];  // 16个DMA通道
    bool enabled;
    uint32_t transfer_count;
    bool simulation_running;
    pthread_t monitor_thread;
    
    // 实例化的寄存器状态（之前是全局的）
    uint32_t dma_global_ctrl;
    uint32_t dma_global_status;
    uint32_t dma_int_status;
    
    // 实例标识和地址配置
    int instance_id;
    char instance_name[32];
    uint32_t base_addr;           // DMA控制器基地址
    uint32_t channel_base_addr;   // DMA通道寄存器基地址
} dma_private_t;

// DMA监控线程
static void* dma_monitor_thread(void *arg) {
    simulator_plugin_t *plugin = (simulator_plugin_t*)arg;
    dma_private_t *priv = (dma_private_t*)plugin->private_data;
    
    printf("[dma_plugin.c:%s] DMA monitor thread started for instance %s\n", __func__, priv->instance_name);
    
    int cycle_count = 0;
    while (priv->simulation_running) {
        sleep(1);  // 每1秒检查一次（简化）
        cycle_count++;
        
        // 检查所有通道的传输状态
        for (int i = 0; i < 16; i++) {
            if (priv->channels[i].ctrl & 0x01) {  // 通道启用
                printf("[%s:%s] %s DMA channel %d active, size=%d, cycle=%d\n", 
                       __FILE__, __func__, priv->instance_name, i, priv->channels[i].size, cycle_count);
                
                // 简化的DMA传输仿真 - 模拟传输需要一定时间
                if (priv->channels[i].size > 0) {
                    // 每次检查减少一部分传输大小，加快传输速度
                    uint32_t transfer_amount = (priv->channels[i].size > 512) ? 512 : priv->channels[i].size;
                    priv->channels[i].size -= transfer_amount;
                    
                    printf("[%s:%s] %s DMA channel %d transferring %d bytes, remaining=%d\n", 
                           __FILE__, __func__, priv->instance_name, i, transfer_amount, priv->channels[i].size);
                    
                    if (priv->channels[i].size == 0) {
                        // 传输完成
                        priv->channels[i].ctrl &= ~0x01;  // 清除启用位
                        priv->channels[i].status |= 0x02; // 设置完成位
                        
                        printf("[%s:%s] %s DMA channel %d transfer completed!\n", 
                               __FILE__, __func__, priv->instance_name, i);
                        
                        // 触发DMA完成中断
                        priv->dma_int_status |= (1 << i);  // 设置中断状态位
                        printf("[%s:%s] %s triggering DMA interrupt for channel %d\n", 
                               __FILE__, __func__, priv->instance_name, i);
                        trigger_interrupt(priv->instance_name, 10 + i);
                    }
                }
            }
        }
        
        // 每10秒打印一次状态信息
        if (cycle_count % 10 == 0) {
            printf("[%s:%s] %s DMA monitor heartbeat - cycle %d\n", 
                   __FILE__, __func__, priv->instance_name, cycle_count);
        }
    }
    
    printf("[dma_plugin.c:%s] %s DMA monitor thread stopped\n", __func__, priv->instance_name);
    return NULL;
}

// DMA时钟处理
static int dma_clock(simulator_plugin_t *plugin, clock_action_t action, uint32_t cycles) {
    dma_private_t *priv = (dma_private_t*)plugin->private_data;
    (void)cycles; // 避免警告
    
    if (action == CLOCK_TICK) {
        // 检查所有通道的传输状态
        for (int i = 0; i < 16; i++) {
            if (priv->channels[i].ctrl & 0x01) {  // 通道启用
                // 简化的DMA传输仿真
                if (priv->channels[i].size > 0) {
                    priv->channels[i].size--;
                    if (priv->channels[i].size == 0) {
                        // 传输完成
                        priv->channels[i].ctrl &= ~0x01;  // 清除启用位
                        priv->channels[i].status |= 0x02; // 设置完成位
                        
                        printf("[%s:%s] DMA channel %d transfer completed\n", __FILE__, __func__, i);
                        
                        // 触发DMA完成中断
                        if (priv->channels[i].config & 0x100) {  // 中断使能
                            trigger_interrupt("dma", 10 + i);
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}

// DMA复位
static int dma_reset(simulator_plugin_t *plugin, reset_action_t action) {
    dma_private_t *priv = (dma_private_t*)plugin->private_data;
    
    if (action == RESET_ASSERT) {
        printf("[%s:%s] DMA reset asserted\n", __FILE__, __func__);
        // 停止监控线程
        priv->simulation_running = false;
        if (priv->monitor_thread) {
            pthread_join(priv->monitor_thread, NULL);
            printf("[%s:%s] DMA monitor thread joined\n", __FILE__, __func__);
        }
        
        // 复位所有通道
        memset(priv->channels, 0, sizeof(priv->channels));
        priv->enabled = false;
        priv->transfer_count = 0;
        priv->dma_global_ctrl = 0;
        priv->dma_global_status = 0;
        priv->dma_int_status = 0;
    }
    
    return 0;
}

// DMA寄存器读取
static uint32_t dma_reg_read(simulator_plugin_t *plugin, uint32_t address) {
    dma_private_t *priv = (dma_private_t*)plugin->private_data;
    
    printf("[%s:%s] %s DMA register read: 0x%08X (base: 0x%08X)\n", 
           __FILE__, __func__, priv->instance_name, address, priv->base_addr);
    
    // 全局寄存器 - 使用相对于基地址的偏移
    uint32_t global_ctrl_addr = priv->base_addr + (DMA_GLOBAL_CTRL_REG - DMA_BASE_ADDR);
    uint32_t global_status_addr = priv->base_addr + (DMA_GLOBAL_STATUS_REG - DMA_BASE_ADDR);
    uint32_t int_status_addr = priv->base_addr + (DMA_INT_STATUS_REG - DMA_BASE_ADDR);
    
    if (address == global_ctrl_addr) {
        return priv->dma_global_ctrl;
    } else if (address == global_status_addr) {
        return priv->dma_global_status;
    } else if (address == int_status_addr) {
        return priv->dma_int_status;
    }
    
    // 通道寄存器 - 使用可配置的通道基地址
    for (int ch = 0; ch < 16; ch++) {
        uint32_t ch_base = priv->channel_base_addr + ch * DMA_CH_OFFSET;
        if (address >= ch_base && address < ch_base + DMA_CH_OFFSET) {
            uint32_t offset = address - ch_base;
            switch (offset) {
                case 0x00: return priv->channels[ch].ctrl;
                case 0x04: return priv->channels[ch].status;
                case 0x08: return priv->channels[ch].src_addr;
                case 0x0C: return priv->channels[ch].dst_addr;
                case 0x10: return priv->channels[ch].size;
                case 0x14: return priv->channels[ch].config;
                default: return 0;
            }
        }
    }
    
    return 0;
}

// DMA寄存器写入
static int dma_reg_write(simulator_plugin_t *plugin, uint32_t address, uint32_t value) {
    dma_private_t *priv = (dma_private_t*)plugin->private_data;
    
    printf("[%s:%s] %s DMA register write: 0x%08X = 0x%08X (base: 0x%08X)\n", 
           __FILE__, __func__, priv->instance_name, address, value, priv->base_addr);
    
    // 全局寄存器 - 使用相对于基地址的偏移
    uint32_t global_ctrl_addr = priv->base_addr + (DMA_GLOBAL_CTRL_REG - DMA_BASE_ADDR);
    uint32_t global_status_addr = priv->base_addr + (DMA_GLOBAL_STATUS_REG - DMA_BASE_ADDR);
    uint32_t int_clear_addr = priv->base_addr + (DMA_INT_CLEAR_REG - DMA_BASE_ADDR);
    
    if (address == global_ctrl_addr) {
        priv->dma_global_ctrl = value;
        priv->enabled = (value & 0x01) != 0;
        printf("[%s:%s] %s DMA global control: enabled=%d\n", __FILE__, __func__, priv->instance_name, priv->enabled);
        return 0;
    } else if (address == global_status_addr) {
        priv->dma_global_status = value;
        return 0;
    } else if (address == int_clear_addr) {
        priv->dma_int_status &= ~value;  // 清除中断状态
        return 0;
    }
    
    // 通道寄存器 - 使用可配置的通道基地址
    for (int ch = 0; ch < 16; ch++) {
        uint32_t ch_base = priv->channel_base_addr + ch * DMA_CH_OFFSET;
        if (address >= ch_base && address < ch_base + DMA_CH_OFFSET) {
            uint32_t offset = address - ch_base;
            switch (offset) {
                case 0x00: // 控制寄存器
                    priv->channels[ch].ctrl = value;
                    if (value & 0x01) {
                        printf("[%s:%s] %s DMA channel %d started, size=%d\n", 
                               __FILE__, __func__, priv->instance_name, ch, priv->channels[ch].size);
                        // 如果传输大小为0，设置默认值用于测试
                        if (priv->channels[ch].size == 0) {
                            priv->channels[ch].size = 1024;  // 默认1KB用于测试
                            printf("[%s:%s] %s DMA channel %d: set default size to %d bytes\n", 
                                   __FILE__, __func__, priv->instance_name, ch, priv->channels[ch].size);
                        }
                    }
                    break;
                case 0x04: // 状态寄存器
                    priv->channels[ch].status = value;
                    break;
                case 0x08: // 源地址
                    priv->channels[ch].src_addr = value;
                    break;
                case 0x0C: // 目标地址
                    priv->channels[ch].dst_addr = value;
                    break;
                case 0x10: // 传输大小
                    priv->channels[ch].size = value;
                    break;
                case 0x14: // 配置寄存器
                    priv->channels[ch].config = value;
                    break;
            }
            return 0;
        }
    }
    
    return 0;
}

// DMA中断处理
static int dma_interrupt(simulator_plugin_t *plugin, uint32_t irq_num) {
    dma_private_t *priv = (dma_private_t*)plugin->private_data;
    printf("[%s:%s] %s DMA interrupt %d handled\n", __FILE__, __func__, priv->instance_name, irq_num);
    
    // 根据中断号设置相应的中断状态位
    if (irq_num >= 10 && irq_num <= 25) {
        priv->dma_int_status |= (1 << (irq_num - 10));
    }
    
    return 0;
}

// DMA初始化
static int dma_init(simulator_plugin_t *plugin) {
    dma_private_t *priv = malloc(sizeof(dma_private_t));
    if (!priv) {
        printf("[%s:%s] Failed to allocate DMA private data\n", __FILE__, __func__);
        return -1;
    }
    
    memset(priv, 0, sizeof(dma_private_t));
    priv->enabled = false;
    priv->simulation_running = true;  // 立即启动监控线程
    
    // 从插件名称中提取实例信息
    priv->instance_id = 0;  // 默认实例ID
    strncpy(priv->instance_name, plugin->name, sizeof(priv->instance_name) - 1);
    
    // 从插件名称中提取实例ID（如果包含数字）
    const char *name_ptr = plugin->name;
    while (*name_ptr && !isdigit(*name_ptr)) name_ptr++;
    if (*name_ptr) {
        priv->instance_id = atoi(name_ptr);
    }
    
    // 根据实例ID自动计算基地址（每个实例占用0x1000字节空间）
    priv->base_addr = DMA_BASE_ADDR + (priv->instance_id * 0x1000);        
    priv->channel_base_addr = priv->base_addr + 0x100; // 通道寄存器偏移0x100
    
    printf("[%s:%s] %s configured with base addr 0x%08X, channel base 0x%08X\n", 
           __FILE__, __func__, priv->instance_name, priv->base_addr, priv->channel_base_addr);
    
    plugin->private_data = priv;
    
    // 启动监控线程
    if (pthread_create(&priv->monitor_thread, NULL, dma_monitor_thread, plugin) == 0) {
        printf("[%s:%s] %s DMA monitor thread started\n", __FILE__, __func__, priv->instance_name);
    } else {
        printf("[%s:%s] Failed to create %s DMA monitor thread\n", __FILE__, __func__, priv->instance_name);
        priv->simulation_running = false;
    }
    
    // 添加一个测试传输用于验证监控线程
    printf("[%s:%s] Setting up test DMA transfer on %s channel 0\n", __FILE__, __func__, priv->instance_name);
    priv->channels[0].src_addr = 0x20000000;
    priv->channels[0].dst_addr = 0x40001000;  // UART_TX_REG
    priv->channels[0].size = 17;  // "Hello DMA Thread!" length
    priv->channels[0].config = 0x100;  // 中断使能
    priv->channels[0].ctrl = 0x01;     // 启用通道
    priv->channels[0].status = 0x00;   // 初始状态
    printf("[%s:%s] Test DMA transfer configured and started for %s\n", __FILE__, __func__, priv->instance_name);
    
    printf("[%s:%s] %s DMA plugin initialized\n", __FILE__, __func__, priv->instance_name);
    return 0;
}

// DMA清理
static void dma_cleanup(simulator_plugin_t *plugin) {
    if (plugin && plugin->private_data) {
        dma_private_t *priv = (dma_private_t*)plugin->private_data;
        
        // 停止监控线程
        if (priv->simulation_running) {
            priv->simulation_running = false;
            if (priv->monitor_thread) {
                pthread_join(priv->monitor_thread, NULL);
                printf("[%s:%s] %s DMA monitor thread joined\n", __FILE__, __func__, priv->instance_name);
            }
        }
        
        free(plugin->private_data);
        plugin->private_data = NULL;
    }
    printf("[%s:%s] DMA plugin cleaned up\n", __FILE__, __func__);
}

// 创建DMA插件
simulator_plugin_t* create_dma_plugin(void) {
    return create_dma_plugin_instance("dma0", 0);
}

// 创建DMA插件实例 - 支持多实例
static simulator_plugin_t* create_dma_plugin_instance(const char *instance_name, int instance_id) {
    simulator_plugin_t *plugin = malloc(sizeof(simulator_plugin_t));
    if (!plugin) {
        return NULL;
    }
    
    memset(plugin, 0, sizeof(simulator_plugin_t));
    
    // 设置实例名称
    if (instance_name) {
        strncpy(plugin->name, instance_name, sizeof(plugin->name) - 1);
    } else {
        snprintf(plugin->name, sizeof(plugin->name), "dma%d", instance_id);
    }
    
    // 设置回调函数
    plugin->init = dma_init;
    plugin->cleanup = dma_cleanup;
    plugin->clock = dma_clock;
    plugin->reset = dma_reset;
    plugin->reg_read = dma_reg_read;
    plugin->reg_write = dma_reg_write;
    plugin->interrupt = dma_interrupt;
    
    printf("[%s:%s] DMA plugin '%s' created\n", __FILE__, __func__, plugin->name);
    return plugin;
}

// 创建DMA插件实例并配置基地址
simulator_plugin_t* create_dma_plugin_with_addr(const char *instance_name, int instance_id, 
                                                uint32_t base_addr, uint32_t channel_base_addr) {
    simulator_plugin_t *plugin = create_dma_plugin_instance(instance_name, instance_id);
    if (!plugin) {
        return NULL;
    }
    
    // 注意：地址配置需要在init之前设置，但这里先创建插件
    // 实际的地址设置会在dma_init中根据instance_id计算
    printf("[%s:%s] DMA plugin '%s' created with base addr 0x%08X, channel base 0x%08X\n", 
           __FILE__, __func__, plugin->name, base_addr, channel_base_addr);
    
    return plugin;
}

// 公共接口函数 - 供外部调用
simulator_plugin_t* create_dma_plugin_multi_instance(const char *instance_name, int instance_id) {
    return create_dma_plugin_instance(instance_name, instance_id);
}

simulator_plugin_t* create_dma_plugin_with_base_addr(const char *instance_name, int instance_id, 
                                                     uint32_t base_addr, uint32_t channel_base_addr) {
    return create_dma_plugin_with_addr(instance_name, instance_id, base_addr, channel_base_addr);
}
