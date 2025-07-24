# IC模拟器项目

## 项目概述

这是一个通用的、易扩展的IC模拟器，采用插件化架构设计，支持通过内存页保护和信号机制实现寄存器访问拦截和中断模拟。

## 架构设计

### 分层架构
```
Test Application
       ↓
Driver Layer (uart_driver)
       ↓
Sim Interface Layer 
       ↓
Simulator Layer (Plugin Framework)
```

### 核心组件

1. **Driver Layer** (`src/driver/`)
   - UART驱动实现
   - 直接进行寄存器访问和信号处理
   - 无需修改即可用于仿真和真实硬件

2. **Sim Interface Layer** (`src/sim_interface/`)
   - 拦截寄存器访问（通过SIGSEGV信号）
   - 处理中断信号
   - 与Simulator Layer通信

3. **Simulator Layer** (`src/simulator/`)
   - 插件管理框架
   - UART等外设仿真插件
   - 消息处理和分发

4. **Common** (`src/common/`)
   - 通信协议定义
   - 公共数据结构

## 通信协议

支持以下消息类型：
- `MSG_CLOCK`: 时钟控制
- `MSG_RESET`: 复位控制  
- `MSG_REG_READ`: 寄存器读取
- `MSG_REG_WRITE`: 寄存器写入
- `MSG_INTERRUPT`: 中断处理
- `MSG_RESPONSE`: 响应消息

## 编译和运行

### 编译
```bash
make
```

### 运行
```bash
make run
```

### 清理
```bash
make clean
```

## 扩展新外设

1. 在 `src/simulator/plugins/` 目录创建新的插件文件
2. 实现 `simulator_plugin_t` 接口
3. 提供 `create_xxx_plugin()` 函数
4. 在主程序中注册插件

## 目录结构

```
├── src/
│   ├── common/
│   │   └── protocol.h          # 通信协议定义
│   ├── driver/
│   │   ├── uart_driver.h       # UART驱动头文件
│   │   └── uart_driver.c       # UART驱动实现
│   ├── sim_interface/
│   │   ├── sim_interface.h     # 仿真接口头文件
│   │   └── sim_interface.c     # 仿真接口实现
│   ├── simulator/
│   │   ├── plugin_interface.h  # 插件接口定义
│   │   ├── plugin_manager.c    # 插件管理器
│   │   └── plugins/
│   │       └── uart_plugin.c   # UART仿真插件
│   └── main.c                  # 主程序和测试用例
├── build/                      # 编译输出目录
├── bin/                        # 可执行文件目录
├── Makefile                    # 编译脚本
└── README.md                   # 项目说明
```

## 特性

- **通用性**: Driver层无需修改，支持多种外设
- **易扩展**: 插件化架构，方便添加新外设仿真
- **高仿真度**: 通过信号机制模拟真实的寄存器访问和中断处理
- **跨平台**: 基于标准C语言实现，支持Linux/Unix系统

## 注意事项

- 当前实现主要适用于Linux/Unix系统
- 需要支持POSIX信号和内存映射功能
- Windows环境需要使用相应的API替代（如VirtualAlloc、SEH等）
