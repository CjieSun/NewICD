#ifndef DMA_DRIVER_H
#define DMA_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../common/register_map.h"

// DMA寄存器地址定义已在register_map.h中统一

// DMA控制位定义
#define DMA_CTRL_ENABLE         (1 << 0)
#define DMA_CTRL_START          (1 << 1)
#define DMA_CTRL_ABORT          (1 << 2)

// DMA状态位定义
#define DMA_STATUS_BUSY         (1 << 0)
#define DMA_STATUS_DONE         (1 << 1)
#define DMA_STATUS_ERROR        (1 << 2)

// DMA配置位定义
#define DMA_CONFIG_MEM_TO_MEM   (0 << 0)
#define DMA_CONFIG_MEM_TO_PER   (1 << 0)
#define DMA_CONFIG_PER_TO_MEM   (2 << 0)
#define DMA_CONFIG_PER_TO_PER   (3 << 0)
#define DMA_CONFIG_INC_SRC      (1 << 4)
#define DMA_CONFIG_INC_DST      (1 << 5)
#define DMA_CONFIG_INT_ENABLE   (1 << 8)

// DMA传输类型
typedef enum {
    DMA_TRANSFER_MEM_TO_MEM = 0,
    DMA_TRANSFER_MEM_TO_PER = 1,
    DMA_TRANSFER_PER_TO_MEM = 2,
    DMA_TRANSFER_PER_TO_PER = 3
} dma_transfer_type_t;

// DMA通道配置结构
typedef struct {
    uint32_t src_addr;          // 源地址
    uint32_t dst_addr;          // 目标地址
    uint32_t size;              // 传输大小(字节)
    dma_transfer_type_t type;   // 传输类型
    bool inc_src;               // 源地址是否递增
    bool inc_dst;               // 目标地址是否递增
    bool interrupt_enable;      // 是否启用中断
} dma_config_t;

// DMA通道状态
typedef enum {
    DMA_CH_IDLE = 0,
    DMA_CH_BUSY = 1,
    DMA_CH_DONE = 2,
    DMA_CH_ERROR = 3
} dma_channel_status_t;

// DMA回调函数类型
typedef void (*dma_callback_t)(uint8_t channel, dma_channel_status_t status);

// DMA驱动函数声明
int dma_init(void);
void dma_cleanup(void);

// DMA通道管理
int dma_allocate_channel(void);
int dma_free_channel(uint8_t channel);
bool dma_is_channel_available(uint8_t channel);

// DMA配置和控制
int dma_configure_channel(uint8_t channel, const dma_config_t *config);
int dma_start_transfer(uint8_t channel);
int dma_stop_transfer(uint8_t channel);
dma_channel_status_t dma_get_channel_status(uint8_t channel);

// DMA传输函数
int dma_transfer_async(uint8_t channel, uint32_t src, uint32_t dst, uint32_t size, 
                      dma_transfer_type_t type, dma_callback_t callback);
int dma_transfer_sync(uint8_t channel, uint32_t src, uint32_t dst, uint32_t size, 
                     dma_transfer_type_t type);

// DMA中断处理
void dma_interrupt_handler(void);

// DMA回调注册
int dma_register_callback(uint8_t channel, dma_callback_t callback);

#endif // DMA_DRIVER_H
