#include "interrupt_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 中断绑定表
static interrupt_binding_t g_interrupt_bindings[MAX_INTERRUPTS];
static int g_interrupt_count = 0;

// 中断管理器初始化
int interrupt_manager_init(void) {
    memset(g_interrupt_bindings, 0, sizeof(g_interrupt_bindings));
    g_interrupt_count = 0;
    
    printf("[%s:%s] Interrupt manager initialized\n", __FILE__, __func__);
    return 0;
}

// 注册中断处理函数
int register_interrupt_handler(uint32_t irq_num, interrupt_handler_t handler) {
    if (g_interrupt_count >= MAX_INTERRUPTS) {
        printf("[%s:%s] Error: Maximum interrupt handlers reached\n", __FILE__, __func__);
        return -1;
    }
    
    if (!handler) {
        printf("[%s:%s] Error: Invalid handler\n", __FILE__, __func__);
        return -1;
    }
    
    // 检查是否已存在相同的中断绑定
    for (int i = 0; i < g_interrupt_count; i++) {
        if (g_interrupt_bindings[i].irq_num == irq_num) {
            printf("[%s:%s] Warning: Interrupt %d already registered, updating handler\n", 
                   __FILE__, __func__, irq_num);
            g_interrupt_bindings[i].handler = handler;
            g_interrupt_bindings[i].enabled = 1;
            return 0;
        }
    }
    
    // 添加新的中断绑定
    interrupt_binding_t *binding = &g_interrupt_bindings[g_interrupt_count];
    binding->irq_num = irq_num;
    binding->handler = handler;
    binding->enabled = 1;
    
    g_interrupt_count++;
    
    printf("[%s:%s] Registered interrupt handler: IRQ %d\n", 
           __FILE__, __func__, irq_num);
    return 0;
}

// 启用中断
int enable_interrupt(uint32_t irq_num) {
    for (int i = 0; i < g_interrupt_count; i++) {
        if (g_interrupt_bindings[i].irq_num == irq_num) {
            g_interrupt_bindings[i].enabled = 1;
            printf("[%s:%s] Enabled interrupt: IRQ %d\n", 
                   __FILE__, __func__, irq_num);
            return 0;
        }
    }
    
    printf("[%s:%s] Warning: Interrupt IRQ %d not found\n", 
           __FILE__, __func__, irq_num);
    return -1;
}

// 禁用中断
int disable_interrupt(uint32_t irq_num) {
    for (int i = 0; i < g_interrupt_count; i++) {
        if (g_interrupt_bindings[i].irq_num == irq_num) {
            g_interrupt_bindings[i].enabled = 0;
            printf("[%s:%s] Disabled interrupt: IRQ %d\n", 
                   __FILE__, __func__, irq_num);
            return 0;
        }
    }
    
    printf("[%s:%s] Warning: Interrupt IRQ %d not found\n", 
           __FILE__, __func__, irq_num);
    return -1;
}

// 触发中断处理（由信号处理器调用）
int handle_interrupt(uint32_t irq_num) {
    for (int i = 0; i < g_interrupt_count; i++) {
        if (g_interrupt_bindings[i].irq_num == irq_num) {
            
            if (!g_interrupt_bindings[i].enabled) {
                printf("[%s:%s] Interrupt IRQ %d is disabled\n", 
                       __FILE__, __func__, irq_num);
                return 0;
            }
            
            if (g_interrupt_bindings[i].handler) {
                printf("[%s:%s] Handling interrupt: IRQ %d\n", 
                       __FILE__, __func__, irq_num);
                g_interrupt_bindings[i].handler();
                return 0;
            } else {
                printf("[%s:%s] Error: No handler for interrupt IRQ %d\n", 
                       __FILE__, __func__, irq_num);
                return -1;
            }
        }
    }
    
    printf("[%s:%s] Warning: Interrupt IRQ %d not found\n", 
           __FILE__, __func__, irq_num);
    return -1;
}

// 获取中断处理函数
interrupt_handler_t get_interrupt_handler(uint32_t irq_num) {
    for (int i = 0; i < g_interrupt_count; i++) {
        if (g_interrupt_bindings[i].irq_num == irq_num) {
            return g_interrupt_bindings[i].handler;
        }
    }
    return NULL;
}

// 清理中断管理器
void interrupt_manager_cleanup(void) {
    memset(g_interrupt_bindings, 0, sizeof(g_interrupt_bindings));
    g_interrupt_count = 0;
    
    printf("[%s:%s] Interrupt manager cleaned up\n", __FILE__, __func__);
}
