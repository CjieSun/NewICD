#include "plugin_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #define dlopen(path, flags) LoadLibrary(path)
    #define dlsym(handle, symbol) GetProcAddress((HMODULE)handle, symbol)
    #define dlclose(handle) FreeLibrary((HMODULE)handle)
    #define dlerror() "Windows LoadLibrary error"
    #define RTLD_LAZY 0
#else
    #include <dlfcn.h>
#endif

#define MAX_PLUGINS 32

typedef struct {
    simulator_plugin_t *plugins[MAX_PLUGINS];
    int plugin_count;
} plugin_manager_t;

static plugin_manager_t g_plugin_manager = {0};

// 注册插件
int register_plugin(simulator_plugin_t *plugin) {
    if (g_plugin_manager.plugin_count >= MAX_PLUGINS) {
        printf("[%s:%s] Error: Maximum plugins reached\n", __FILE__, __func__);
        return -1;
    }
    
    g_plugin_manager.plugins[g_plugin_manager.plugin_count] = plugin;
    g_plugin_manager.plugin_count++;
    
    if (plugin->init) {
        return plugin->init(plugin);
    }
    
    printf("[%s:%s] Plugin '%s' registered successfully\n", __FILE__, __func__, plugin->name);
    return 0;
}

// 查找插件
simulator_plugin_t* find_plugin(const char *name) {
    for (int i = 0; i < g_plugin_manager.plugin_count; i++) {
        if (strcmp(g_plugin_manager.plugins[i]->name, name) == 0) {
            return g_plugin_manager.plugins[i];
        }
    }
    return NULL;
}

// 加载动态库插件
int load_plugin_from_lib(const char *lib_path, const char *create_func_name) {
    void *handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        printf("[%s:%s] Error loading plugin: %s\n", __FILE__, __func__, dlerror());
        return -1;
    }
    
    plugin_create_func_t create_func = (plugin_create_func_t)dlsym(handle, create_func_name);
    if (!create_func) {
        printf("[%s:%s] Error finding create function: %s\n", __FILE__, __func__, dlerror());
        dlclose(handle);
        return -1;
    }
    
    simulator_plugin_t *plugin = create_func();
    if (!plugin) {
        printf("[%s:%s] Error creating plugin instance\n", __FILE__, __func__);
        dlclose(handle);
        return -1;
    }
    
    return register_plugin(plugin);
}

// 处理仿真消息
int handle_sim_message(const sim_message_t *msg, sim_message_t *response) {
    simulator_plugin_t *plugin = find_plugin(msg->module);
    if (!plugin) {
        printf("[%s:%s] Plugin not found: %s\n", __FILE__, __func__, msg->module);
        if (response) {
            response->type = MSG_RESPONSE;
            response->id = msg->id;
            response->data.response.error = -1;
        }
        return -1;
    }
    
    int result = 0;
    uint32_t reg_value = 0;
    
    switch (msg->type) {
        case MSG_CLOCK:
            if (plugin->clock) {
                result = plugin->clock(plugin, msg->data.clock.action, msg->data.clock.cycles);
            }
            break;
            
        case MSG_RESET:
            if (plugin->reset) {
                result = plugin->reset(plugin, msg->data.reset.action);
            }
            break;
            
        case MSG_REG_READ:
            if (plugin->reg_read) {
                reg_value = plugin->reg_read(plugin, msg->address);
                result = 0;
            }
            break;
            
        case MSG_REG_WRITE:
            if (plugin->reg_write) {
                result = plugin->reg_write(plugin, msg->address, msg->value);
            }
            break;
            
        case MSG_INTERRUPT:
            if (plugin->interrupt) {
                result = plugin->interrupt(plugin, msg->data.interrupt.irq_num);
            }
            break;
            
        default:
            result = -1;
            break;
    }
    
    // 构造响应消息
    if (response) {
        response->type = MSG_RESPONSE;
        response->id = msg->id;
        response->data.response.result = (msg->type == MSG_REG_READ) ? reg_value : result;
        response->data.response.error = (result < 0) ? -1 : 0;
    }
    
    return result;
}

// 清理所有插件
void cleanup_plugins(void) {
    for (int i = 0; i < g_plugin_manager.plugin_count; i++) {
        simulator_plugin_t *plugin = g_plugin_manager.plugins[i];
        if (plugin && plugin->cleanup) {
            plugin->cleanup(plugin);
        }
    }
    g_plugin_manager.plugin_count = 0;
}
