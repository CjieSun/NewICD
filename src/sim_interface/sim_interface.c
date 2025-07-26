#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "sim_interface.h"
#include "../simulator/plugin_interface.h"
#include "interrupt_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <ucontext.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

// 声明外部函数
extern int register_plugin(simulator_plugin_t *plugin);
extern simulator_plugin_t* find_plugin(const char *name);
extern int handle_sim_message(const sim_message_t *msg, sim_message_t *response);
extern void cleanup_plugins(void);

#define MAX_REG_MAPPINGS 32
#define MAX_SIGNAL_MAPPINGS 16

static reg_mapping_t g_reg_mappings[MAX_REG_MAPPINGS];
static signal_mapping_t g_signal_mappings[MAX_SIGNAL_MAPPINGS];
static int g_reg_mapping_count = 0;
static int g_signal_mapping_count = 0;
static uint32_t g_msg_id_counter = 1;

// 查找寄存器映射
static reg_mapping_t* find_register_mapping(void *addr) {
    for (int i = 0; i < g_reg_mapping_count; i++) {
        reg_mapping_t *mapping = &g_reg_mappings[i];
        if (addr >= mapping->start_addr && 
            addr < (char*)mapping->start_addr + (mapping->end_addr - mapping->start_addr)) {
            return mapping;
        }
    }
    return NULL;
}

// 段错误信号处理器
static void segfault_handler(int sig, siginfo_t *si, void *ctx) {
    (void)sig;

    void *fault_addr = si->si_addr;
    reg_mapping_t *mapping = find_register_mapping(fault_addr);
    
    if (!mapping) {
        printf("[%s:%s] Segfault at unknown address: %p\n", __FILE__, __func__, fault_addr);
        exit(1);
    }
    else {
        //printf("[%s:%s] Segfault at address: %p in module %s\n", __FILE__, __func__, fault_addr, mapping->module);
    }

    sim_message_t msg = {0};
    sim_message_t response = {0};
    
    strcpy(msg.module, mapping->module);
    msg.address = (uintptr_t)fault_addr;
    msg.id = g_msg_id_counter++;

    // 获取当前指令指针和指令内容
    ucontext_t *uc = (ucontext_t *)ctx;
    uintptr_t rip = uc->uc_mcontext.gregs[REG_RIP];
    uint8_t *insn = (uint8_t *)rip;
    if (insn[0] == 0x8B) {               // 8b 00  mov    (%rax),%eax - 读操作
        msg.type = MSG_REG_READ;

        //printf("[%s:%s] Register read start: addr=0x%08X\n", __FILE__, __func__, msg.address);
        if (handle_sim_message(&msg, &response) == 0) {
            printf("[%s:%s] Register read completed: addr=0x%08X, 0x%08X\n", __FILE__, __func__, msg.address, response.data.response.result);
            
            // 将读取的值设置到目标寄存器(这里假设是EAX)
            uc->uc_mcontext.gregs[REG_RAX] = response.data.response.result;
        } else {
            printf("[%s:%s] Failed to handle register read\n", __FILE__, __func__);
            exit(1);
        }

        uc->uc_mcontext.gregs[REG_RIP] += 2; // next instruction
    } else if (insn[0] == 0x89) {        // 89 02  mov    %eax,(%rdx) - 写寄存器操作
        msg.value = uc->uc_mcontext.gregs[REG_RAX];
        msg.type = MSG_REG_WRITE;

        //printf("[%s:%s] Register write start: addr=0x%08X, value=0x%08X\n", __FILE__, __func__, msg.address, msg.value);
        if (handle_sim_message(&msg, &response) == 0) {
            printf("[%s:%s] Register write completed: addr=0x%08X, 0x%08X\n", __FILE__, __func__, msg.address, response.data.response.result);
        } else {
            printf("[%s:%s] Failed to handle register write\n", __FILE__, __func__);
            exit(1);
        }

        uc->uc_mcontext.gregs[REG_RIP] += 2; // next instruction 
    } else if (insn[0] == 0xC7) {        // c7 00 01 00 00 00       movl   $0x1,(%rax) - 写立即数操作
        msg.value = *(uint32_t *)(rip + 2);
        msg.type = MSG_REG_WRITE;

        //printf("[%s:%s] Register write start: addr=0x%08X, value=0x%08X\n", __FILE__, __func__, msg.address, msg.value);
        if (handle_sim_message(&msg, &response) == 0) {
            printf("[%s:%s] Register write completed: addr=0x%08X, 0x%08X\n", __FILE__, __func__, msg.address, response.data.response.result);
        } else {
            printf("[%s:%s] Failed to handle register write\n", __FILE__, __func__);
            exit(1);
        }

        uc->uc_mcontext.gregs[REG_RIP] += 6; // next instruction
    } else {                            // 对于其他不支持的指令，我们简化处理：
        printf("[%s:%s] Unsupported instruction: 0x%02X at RIP=0x%lx, treating as read\n", __FILE__, __func__, insn[0], rip);
    }
}

// 信号处理器（用于中断）
static void interrupt_signal_handler(int sig) {
    //printf("[%s:%s] Interrupt signal %d received.\n", __FILE__, __func__, sig);

    for (int i = 0; i < g_signal_mapping_count; i++) {
        if (g_signal_mappings[i].signal_num == sig) {
            signal_mapping_t *mapping = &g_signal_mappings[i];
            printf("[%s:%s] Interrupt signal %d received for module %s (IRQ %d)\n", 
                   __FILE__, __func__, sig, mapping->module, mapping->irq_num);
            
            // 使用interrupt_manager处理中断
            handle_interrupt(mapping->irq_num);
            break;
        }
    }
}

// 初始化sim interface
int sim_interface_init(void) {

    // 设置段错误信号处理器
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_handler;
    
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction SIGSEGV");
        return -1;
    }
    
    printf("[%s:%s] Sim interface initialized\n", __FILE__, __func__);
    return 0;
}

// 添加寄存器映射
int add_register_mapping(uint32_t start_addr, uint32_t end_addr, const char *module) {
    if (g_reg_mapping_count >= MAX_REG_MAPPINGS) {
        printf("[%s:%s] Error: Maximum register mappings reached\n", __FILE__, __func__);
        return -1;
    }
    
    size_t size = end_addr - start_addr;
    
    // 分配内存并设置为不可访问，触发段错误

    void *mapped_addr = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapped_addr == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    
    reg_mapping_t *mapping = &g_reg_mappings[g_reg_mapping_count];
    mapping->start_addr = start_addr;
    mapping->end_addr = end_addr;
    strcpy(mapping->module, module);
    mapping->mapped_addr = mapped_addr;
    
    g_reg_mapping_count++;
    
    printf("[%s:%s] Register mapping added: %s [0x%08X-0x%08X] -> %p\n", 
           __FILE__, __func__, module, start_addr, end_addr, mapped_addr);
    return 0;
}

// 添加信号映射
int add_signal_mapping(int signal_num, const char *module, uint32_t irq_num) {
    if (g_signal_mapping_count >= MAX_SIGNAL_MAPPINGS) {
        printf("[%s:%s] Error: Maximum signal mappings reached\n", __FILE__, __func__);
        return -1;
    }
    
    signal_mapping_t *mapping = &g_signal_mappings[g_signal_mapping_count];
    mapping->signal_num = signal_num;
    strcpy(mapping->module, module);
    mapping->irq_num = irq_num;
    
    // 注册信号处理器
    signal(signal_num, interrupt_signal_handler);
    
    g_signal_mapping_count++;
    
    printf("[%s:%s] Signal mapping added: signal %d -> %s IRQ %d\n", 
           __FILE__, __func__, signal_num, module, irq_num);
    return 0;
}

// 触发中断信号
int trigger_interrupt(const char *module, uint32_t irq_num) {
    for (int i = 0; i < g_signal_mapping_count; i++) {
        signal_mapping_t *mapping = &g_signal_mappings[i];
        if (strcmp(mapping->module, module) == 0 && mapping->irq_num == irq_num) {
            printf("[%s:%s] Triggering interrupt: signal %d for %s IRQ %d\n", 
                   __FILE__, __func__, mapping->signal_num, module, irq_num);
            kill(getpid(), mapping->signal_num);
            return 0;
        }
    }
    printf("[%s:%s] Warning: No signal mapping found for %s IRQ %d\n", __FILE__, __func__, module, irq_num);
    return -1;
}

// 清理资源
void sim_interface_cleanup(void) {
    // 释放映射的内存
    for (int i = 0; i < g_reg_mapping_count; i++) {
        reg_mapping_t *mapping = &g_reg_mappings[i];
        size_t size = mapping->end_addr - mapping->start_addr;
        munmap(mapping->mapped_addr, size);
    }
    
    g_reg_mapping_count = 0;
    g_signal_mapping_count = 0;
    
    cleanup_plugins();
    printf("[%s:%s] Sim interface cleaned up\n", __FILE__, __func__);
}
