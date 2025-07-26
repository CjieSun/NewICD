#ifndef MULTI_INSTANCE_H
#define MULTI_INSTANCE_H

#include "plugin_interface.h"

// DMA多实例创建函数
simulator_plugin_t* create_dma_plugin_multi_instance(const char *instance_name, int instance_id);
simulator_plugin_t* create_dma_plugin_with_base_addr(const char *instance_name, int instance_id, 
                                                     uint32_t base_addr, uint32_t channel_base_addr);

// UART多实例创建函数
simulator_plugin_t* create_uart_plugin_multi_instance(const char *instance_name, int instance_id);
simulator_plugin_t* create_uart_plugin_with_base_addr(const char *instance_name, int instance_id, uint32_t base_addr);

#endif // MULTI_INSTANCE_H
