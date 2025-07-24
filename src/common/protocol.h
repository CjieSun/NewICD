#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// 消息类型定义
typedef enum {
    MSG_CLOCK = 1,
    MSG_RESET = 2,
    MSG_REG_READ = 3,
    MSG_REG_WRITE = 4,
    MSG_INTERRUPT = 5,
    MSG_RESPONSE = 6
} msg_type_t;

// 时钟动作
typedef enum {
    CLOCK_TICK = 1,
    CLOCK_ENABLE = 2,
    CLOCK_DISABLE = 3
} clock_action_t;

// 复位动作
typedef enum {
    RESET_ASSERT = 1,
    RESET_DEASSERT = 2
} reset_action_t;

// 消息结构体
typedef struct {
    msg_type_t type;
    char module[32];        // 模块名如"uart", "spi"等
    uint32_t address;       // 寄存器地址
    uint32_t value;         // 寄存器值
    uint32_t id;           // 消息ID
    union {
        struct {
            clock_action_t action;
            uint32_t cycles;
        } clock;
        struct {
            reset_action_t action;
        } reset;
        struct {
            uint32_t irq_num;
        } interrupt;
        struct {
            int32_t result;
            int32_t error;
        } response;
    } data;
} sim_message_t;

#endif // PROTOCOL_H
