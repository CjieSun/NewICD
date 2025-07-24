#ifndef SIM_INTERFACE_H
#define SIM_INTERFACE_H

#include "../common/protocol.h"
#include <signal.h>
#include <sys/mman.h>

// 寄存器映射条目
typedef struct {
    uint32_t start_addr;
    uint32_t end_addr;
    char module[32];
    void *mapped_addr;
} reg_mapping_t;

// 信号映射条目
typedef struct {
    int signal_num;
    char module[32];
    uint32_t irq_num;
} signal_mapping_t;

// Sim Interface初始化
int sim_interface_init(void);

// 设置寄存器映射
int add_register_mapping(uint32_t start_addr, uint32_t end_addr, const char *module);

// 设置信号映射
int add_signal_mapping(int signal_num, const char *module, uint32_t irq_num);

// 触发中断信号
int trigger_interrupt(const char *module, uint32_t irq_num);

// 获取映射的虚拟地址
void* get_mapped_address(uint32_t physical_addr);

// 清理资源
void sim_interface_cleanup(void);

#endif // SIM_INTERFACE_H
