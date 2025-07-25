#ifndef INTERRUPT_MANAGER_H
#define INTERRUPT_MANAGER_H

#include <stdint.h>

// 中断处理函数类型定义
typedef void (*interrupt_handler_t)(void);

// 最大中断数量
#define MAX_INTERRUPTS 32

// 中断绑定结构
typedef struct {
    uint32_t irq_num;           // 中断号
    interrupt_handler_t handler; // 中断处理函数
    int enabled;                // 是否启用
} interrupt_binding_t;

// 中断管理器初始化
int interrupt_manager_init(void);

// 注册中断处理函数
int register_interrupt_handler(uint32_t irq_num, interrupt_handler_t handler);

// 启用/禁用中断
int enable_interrupt(uint32_t irq_num);
int disable_interrupt(uint32_t irq_num);

// 触发中断处理（由信号处理器调用）
int handle_interrupt(uint32_t irq_num);

// 获取中断处理函数
interrupt_handler_t get_interrupt_handler(uint32_t irq_num);

// 清理中断管理器
void interrupt_manager_cleanup(void);

#endif // INTERRUPT_MANAGER_H
