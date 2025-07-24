#ifndef SIMULATOR_PLUGIN_H
#define SIMULATOR_PLUGIN_H

#include "../common/protocol.h"

// 插件接口定义
typedef struct simulator_plugin {
    char name[32];
    
    // 插件方法
    int (*clock)(struct simulator_plugin *plugin, clock_action_t action, uint32_t cycles);
    int (*reset)(struct simulator_plugin *plugin, reset_action_t action);
    uint32_t (*reg_read)(struct simulator_plugin *plugin, uint32_t address);
    int (*reg_write)(struct simulator_plugin *plugin, uint32_t address, uint32_t value);
    int (*interrupt)(struct simulator_plugin *plugin, uint32_t irq_num);
    
    // 插件初始化和清理
    int (*init)(struct simulator_plugin *plugin);
    void (*cleanup)(struct simulator_plugin *plugin);
    
    // 私有数据
    void *private_data;
} simulator_plugin_t;

// 插件注册函数类型
typedef simulator_plugin_t* (*plugin_create_func_t)(void);

#endif // SIMULATOR_PLUGIN_H
