#include "dma_driver.h"
#include "../common/register_map.h"
#include "../sim_interface/interrupt_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
    #define usleep(us) Sleep((us)/1000)
#else
    #include <unistd.h>
#endif

// DMA寄存器访问宏
#define DMA_GLOBAL_CTRL_PTR     ((volatile uint32_t*)DMA_GLOBAL_CTRL_REG)
#define DMA_GLOBAL_STATUS_PTR   ((volatile uint32_t*)DMA_GLOBAL_STATUS_REG)
#define DMA_INT_STATUS_PTR      ((volatile uint32_t*)DMA_INT_STATUS_REG)
#define DMA_INT_CLEAR_PTR       ((volatile uint32_t*)DMA_INT_CLEAR_REG)

#define DMA_CH_CTRL_PTR(ch)     ((volatile uint32_t*)DMA_CH_CTRL_REG(ch))
#define DMA_CH_STATUS_PTR(ch)   ((volatile uint32_t*)DMA_CH_STATUS_REG(ch))
#define DMA_CH_SRC_PTR(ch)      ((volatile uint32_t*)DMA_CH_SRC_REG(ch))
#define DMA_CH_DST_PTR(ch)      ((volatile uint32_t*)DMA_CH_DST_REG(ch))
#define DMA_CH_SIZE_PTR(ch)     ((volatile uint32_t*)DMA_CH_SIZE_REG(ch))
#define DMA_CH_CONFIG_PTR(ch)   ((volatile uint32_t*)DMA_CH_CONFIG_REG(ch))

// DMA通道管理结构
typedef struct {
    bool allocated;             // 是否已分配
    bool busy;                  // 是否忙碌
    dma_callback_t callback;    // 完成回调函数
} dma_channel_info_t;

// DMA全局状态
static dma_channel_info_t g_dma_channels[DMA_MAX_CHANNELS];
static bool g_dma_initialized = false;

// DMA中断处理函数
void dma_interrupt_handler(void) {
    printf("[%s:%s] DMA interrupt received\n", __FILE__, __func__);
    
    uint32_t int_status = *DMA_INT_STATUS_PTR;
    
    for (uint8_t ch = 0; ch < DMA_MAX_CHANNELS; ch++) {
        if (int_status & (1 << ch)) {
            printf("[%s:%s] DMA channel %d interrupt\n", __FILE__, __func__, ch);
            
            // 获取通道状态
            uint32_t ch_status = *DMA_CH_STATUS_PTR(ch);
            dma_channel_status_t status;
            
            if (ch_status & DMA_STATUS_ERROR) {
                status = DMA_CH_ERROR;
                printf("[%s:%s] DMA channel %d error\n", __FILE__, __func__, ch);
            } else if (ch_status & DMA_STATUS_DONE) {
                status = DMA_CH_DONE;
                printf("[%s:%s] DMA channel %d transfer complete\n", __FILE__, __func__, ch);
            } else {
                status = DMA_CH_BUSY;
            }
            
            // 更新通道状态
            g_dma_channels[ch].busy = (status == DMA_CH_BUSY);
            
            // 调用回调函数
            if (g_dma_channels[ch].callback) {
                g_dma_channels[ch].callback(ch, status);
            }
            
            // 清除中断标志
            *DMA_INT_CLEAR_PTR |= (1 << ch);
        }
    }
}

// DMA初始化
int dma_init(void) {
    printf("[%s:%s] DMA driver initializing...\n", __FILE__, __func__);
    
    if (g_dma_initialized) {
        printf("[%s:%s] DMA already initialized\n", __FILE__, __func__);
        return 0;
    }
    
    // 初始化通道管理结构
    memset(g_dma_channels, 0, sizeof(g_dma_channels));
    
    // 注册DMA中断处理函数 (IRQ 8)
    if (register_interrupt_handler(8, dma_interrupt_handler) != 0) {
        printf("[%s:%s] Failed to register DMA interrupt handler\n", __FILE__, __func__);
        return -1;
    }
    
    // 启用DMA控制器
    *DMA_GLOBAL_CTRL_PTR = DMA_CTRL_ENABLE;
    
    // 清除所有中断标志
    *DMA_INT_CLEAR_PTR = 0xFFFF;
    
    g_dma_initialized = true;
    printf("[%s:%s] DMA driver initialized, %d channels available\n", 
           __FILE__, __func__, DMA_MAX_CHANNELS);
    
    return 0;
}

// DMA清理
void dma_cleanup(void) {
    printf("[%s:%s] DMA driver cleanup...\n", __FILE__, __func__);
    
    if (!g_dma_initialized) {
        return;
    }
    
    // 停止所有通道
    for (uint8_t ch = 0; ch < DMA_MAX_CHANNELS; ch++) {
        if (g_dma_channels[ch].allocated) {
            dma_stop_transfer(ch);
            dma_free_channel(ch);
        }
    }
    
    // 禁用DMA控制器
    *DMA_GLOBAL_CTRL_PTR = 0;
    
    g_dma_initialized = false;
    printf("[%s:%s] DMA driver cleanup completed\n", __FILE__, __func__);
}

// 分配DMA通道
int dma_allocate_channel(void) {
    if (!g_dma_initialized) {
        printf("[%s:%s] DMA not initialized\n", __FILE__, __func__);
        return -1;
    }
    
    for (uint8_t ch = 0; ch < DMA_MAX_CHANNELS; ch++) {
        if (!g_dma_channels[ch].allocated) {
            g_dma_channels[ch].allocated = true;
            g_dma_channels[ch].busy = false;
            g_dma_channels[ch].callback = NULL;
            
            printf("[%s:%s] Allocated DMA channel %d\n", __FILE__, __func__, ch);
            return ch;
        }
    }
    
    printf("[%s:%s] No available DMA channels\n", __FILE__, __func__);
    return -1;
}

// 释放DMA通道
int dma_free_channel(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        printf("[%s:%s] Invalid channel %d\n", __FILE__, __func__, channel);
        return -1;
    }
    
    if (!g_dma_channels[channel].allocated) {
        printf("[%s:%s] Channel %d not allocated\n", __FILE__, __func__, channel);
        return -1;
    }
    
    // 停止传输
    dma_stop_transfer(channel);
    
    // 重置通道状态
    g_dma_channels[channel].allocated = false;
    g_dma_channels[channel].busy = false;
    g_dma_channels[channel].callback = NULL;
    
    printf("[%s:%s] Freed DMA channel %d\n", __FILE__, __func__, channel);
    return 0;
}

// 检查通道是否可用
bool dma_is_channel_available(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        return false;
    }
    return !g_dma_channels[channel].allocated;
}

// 配置DMA通道
int dma_configure_channel(uint8_t channel, const dma_config_t *config) {
    if (channel >= DMA_MAX_CHANNELS || !config) {
        printf("[%s:%s] Invalid parameters\n", __FILE__, __func__);
        return -1;
    }
    
    if (!g_dma_channels[channel].allocated) {
        printf("[%s:%s] Channel %d not allocated\n", __FILE__, __func__, channel);
        return -1;
    }
    
    if (g_dma_channels[channel].busy) {
        printf("[%s:%s] Channel %d is busy\n", __FILE__, __func__, channel);
        return -1;
    }
    
    // 配置源地址和目标地址
    *DMA_CH_SRC_PTR(channel) = config->src_addr;
    *DMA_CH_DST_PTR(channel) = config->dst_addr;
    *DMA_CH_SIZE_PTR(channel) = config->size;
    
    // 配置传输参数
    uint32_t config_reg = 0;
    config_reg |= (config->type & 0x3);
    if (config->inc_src) config_reg |= DMA_CONFIG_INC_SRC;
    if (config->inc_dst) config_reg |= DMA_CONFIG_INC_DST;
    if (config->interrupt_enable) config_reg |= DMA_CONFIG_INT_ENABLE;
    
    *DMA_CH_CONFIG_PTR(channel) = config_reg;
    
    printf("[%s:%s] Configured DMA channel %d: src=0x%08X, dst=0x%08X, size=%d\n",
           __FILE__, __func__, channel, config->src_addr, config->dst_addr, config->size);
    
    return 0;
}

// 启动DMA传输
int dma_start_transfer(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        printf("[%s:%s] Invalid channel %d\n", __FILE__, __func__, channel);
        return -1;
    }
    
    if (!g_dma_channels[channel].allocated) {
        printf("[%s:%s] Channel %d not allocated\n", __FILE__, __func__, channel);
        return -1;
    }
    
    // 启用通道并开始传输
    *DMA_CH_CTRL_PTR(channel) = DMA_CTRL_ENABLE | DMA_CTRL_START;
    g_dma_channels[channel].busy = true;
    
    printf("[%s:%s] Started DMA transfer on channel %d\n", __FILE__, __func__, channel);
    return 0;
}

// 停止DMA传输
int dma_stop_transfer(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        printf("[%s:%s] Invalid channel %d\n", __FILE__, __func__, channel);
        return -1;
    }
    
    // 中止传输
    *DMA_CH_CTRL_PTR(channel) = DMA_CTRL_ABORT;
    g_dma_channels[channel].busy = false;
    
    printf("[%s:%s] Stopped DMA transfer on channel %d\n", __FILE__, __func__, channel);
    return 0;
}

// 获取通道状态
dma_channel_status_t dma_get_channel_status(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        return DMA_CH_ERROR;
    }
    
    if (!g_dma_channels[channel].allocated) {
        return DMA_CH_IDLE;
    }
    
    uint32_t status = *DMA_CH_STATUS_PTR(channel);
    
    if (status & DMA_STATUS_ERROR) {
        return DMA_CH_ERROR;
    } else if (status & DMA_STATUS_DONE) {
        return DMA_CH_DONE;
    } else if (status & DMA_STATUS_BUSY) {
        return DMA_CH_BUSY;
    } else {
        return DMA_CH_IDLE;
    }
}

// 异步DMA传输
int dma_transfer_async(uint8_t channel, uint32_t src, uint32_t dst, uint32_t size,
                      dma_transfer_type_t type, dma_callback_t callback) {
    dma_config_t config = {
        .src_addr = src,
        .dst_addr = dst,
        .size = size,
        .type = type,
        .inc_src = true,
        .inc_dst = true,
        .interrupt_enable = true
    };
    
    if (dma_configure_channel(channel, &config) != 0) {
        return -1;
    }
    
    g_dma_channels[channel].callback = callback;
    
    return dma_start_transfer(channel);
}

// 同步DMA传输
int dma_transfer_sync(uint8_t channel, uint32_t src, uint32_t dst, uint32_t size,
                     dma_transfer_type_t type) {
    dma_config_t config = {
        .src_addr = src,
        .dst_addr = dst,
        .size = size,
        .type = type,
        .inc_src = true,
        .inc_dst = true,
        .interrupt_enable = false
    };
    
    if (dma_configure_channel(channel, &config) != 0) {
        return -1;
    }
    
    if (dma_start_transfer(channel) != 0) {
        return -1;
    }
    
    // 等待传输完成
    while (g_dma_channels[channel].busy) {
        dma_channel_status_t status = dma_get_channel_status(channel);
        if (status == DMA_CH_DONE) {
            g_dma_channels[channel].busy = false;
            break;
        } else if (status == DMA_CH_ERROR) {
            printf("[%s:%s] DMA transfer error on channel %d\n", __FILE__, __func__, channel);
            return -1;
        }
        usleep(1000);  // 等待1ms
    }
    
    printf("[%s:%s] DMA sync transfer completed on channel %d\n", __FILE__, __func__, channel);
    return 0;
}

// 注册回调函数
int dma_register_callback(uint8_t channel, dma_callback_t callback) {
    if (channel >= DMA_MAX_CHANNELS) {
        printf("[%s:%s] Invalid channel %d\n", __FILE__, __func__, channel);
        return -1;
    }
    
    if (!g_dma_channels[channel].allocated) {
        printf("[%s:%s] Channel %d not allocated\n", __FILE__, __func__, channel);
        return -1;
    }
    
    g_dma_channels[channel].callback = callback;
    return 0;
}
