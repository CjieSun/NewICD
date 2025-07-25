# IC模拟器项目 - 通用硬件仿真平台

## 项目概述

这是一个采用**分层解耦**设计的通用IC模拟器，实现了驱动透明性和插件化硬件仿真。通过内存保护机制和信号处理技术，驱动程序可以像访问真实硬件一样工作，无需任何修改。

## 🏗️ 架构设计

### 分层架构
```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (main.c)                      │
│              系统初始化 + 测试用例执行                    │
└─────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────┐
│                   驱动层 (Driver Layer)                  │
│      UART Driver + Interrupt Manager + 静态配置          │
│           (透明的寄存器访问 + 中断处理)                    │
└─────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────┐
│              接口层 (Sim Interface Layer)                │
│     内存保护 + 信号处理 + 寄存器映射 + 中断路由            │
└─────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────┐
│               模拟器层 (Simulator Layer)                 │
│            插件管理 + 硬件行为模拟                        │
└─────────────────────────────────────────────────────────┘
```

### 🎯 核心设计原则

#### 1. **驱动透明性**
- 驱动程序可以像访问真实硬件一样工作，无需修改
- 通过内存保护(mmap + PROT_NONE)拦截寄存器访问
- 驱动代码与模拟器完全解耦

#### 2. **中断架构分离** 
```c
// 驱动层：只负责注册中断处理函数
register_interrupt_handler(5, uart_tx_interrupt_handler);

// 主程序：负责信号到中断的映射  
{34, "uart", 5},  // 信号34 -> IRQ 5

// 接口层：负责信号处理和中断路由
handle_interrupt(mapping->irq_num);
```

#### 3. **静态配置表**
```c
// 寄存器映射表
static const struct {
    uint32_t start_addr, end_addr;
    const char *module;
} register_mappings[] = {
    {0x40001000, 0x40001050, "uart"},
};

// 信号映射表
static const struct {
    int signal_num;
    const char *module; 
    uint32_t irq_num;
} signal_mappings[] = {
    {34, "uart", 5},  // TX中断
    {35, "uart", 6},  // RX中断
};
```

### 🧩 核心组件

1. **驱动层** (`src/driver/`)
   - **UART Driver**: 标准UART驱动实现，直接访问寄存器地址
   - **Interrupt Manager**: 中断管理器，统一管理IRQ到处理函数的映射
   - 无需修改即可用于仿真和真实硬件

2. **接口层** (`src/sim_interface/`)
   - **内存保护**: 使用mmap设置PROT_NONE拦截寄存器访问
   - **信号处理**: 捕获SIGSEGV并解析访问意图
   - **中断路由**: 将信号映射到IRQ号并调用interrupt_manager
   - **模块解耦**: 通过字符串模块名实现松耦合

3. **模拟器层** (`src/simulator/`)
   - **插件架构**: 每个硬件模块作为独立插件
   - **消息驱动**: 通过标准消息协议通信
   - **行为模拟**: 实现真实硬件的功能逻辑

4. **应用层** (`src/main.c`)
   - **静态配置**: 编译时配置寄存器和中断映射
   - **系统初始化**: 一键初始化所有组件
   - **测试框架**: 模块化的测试用例

5. **通用协议** (`src/common/`)
   - 标准化的消息协议定义
   - 跨层通信的数据结构

## 📋 通信协议

支持以下消息类型：
- `MSG_CLOCK`: 时钟控制 (启用/禁用/时钟周期)
- `MSG_RESET`: 复位控制 (拉高/拉低复位信号)
- `MSG_REG_READ`: 寄存器读取
- `MSG_REG_WRITE`: 寄存器写入  
- `MSG_INTERRUPT`: 中断处理
- `MSG_RESPONSE`: 响应消息

## 🚀 快速开始

### 编译环境要求
- GCC 编译器
- Linux/WSL环境 (支持POSIX信号)
- Make 构建工具

### 编译
```bash
make clean && make
```

### 运行测试
```bash
make run
# 或者直接运行
./bin/ic_simulator
```

### 清理
```bash
make clean
```

## 📂 项目结构

```
NewICD2/
├── src/
│   ├── common/
│   │   └── protocol.h              # 通信协议定义
│   ├── driver/
│   │   ├── uart_driver.h/.c        # UART驱动实现
│   │   └── interrupt_manager.h/.c  # 中断管理器
│   ├── sim_interface/
│   │   ├── sim_interface.h/.c      # 仿真接口层
│   │   └── plugin_manager.c        # 插件管理器  
│   ├── simulator/
│   │   ├── plugin_interface.h      # 插件接口定义
│   │   └── plugins/
│   │       └── uart_plugin.c       # UART仿真插件
│   └── main.c                      # 主程序和静态配置
├── build/                          # 编译中间文件
├── bin/                            # 可执行文件
├── Makefile                        # 构建脚本
└── README.md                       # 项目文档
```

## 🔧 扩展新硬件模块

### 添加新外设的步骤 (以SPI为例)

#### 1. 更新静态配置表
```c
// main.c - 添加寄存器映射
static const struct {
    uint32_t start_addr, end_addr;
    const char *module;
} register_mappings[] = {
    {0x40001000, 0x40001050, "uart"},
    {0x40002000, 0x40002050, "spi"},    // 新增SPI
};

// main.c - 添加中断映射  
static const struct {
    int signal_num;
    const char *module;
    uint32_t irq_num; 
} signal_mappings[] = {
    {34, "uart", 5}, {35, "uart", 6},
    {36, "spi", 7},     // 新增SPI中断
};
```

#### 2. 创建SPI驱动
```c
// src/driver/spi_driver.c
#include "interrupt_manager.h"

void spi_tx_interrupt_handler(void) {
    // SPI发送中断处理逻辑
}

int spi_init(void) {
    register_interrupt_handler(7, spi_tx_interrupt_handler);
    return 0;
}
```

#### 3. 创建SPI插件
```c
// src/simulator/plugins/spi_plugin.c
static int spi_handle_register(simulator_plugin_t *plugin, 
                              const sim_message_t *msg,
                              sim_message_t *response) {
    // 实现SPI寄存器读写逻辑
}

simulator_plugin_t* create_spi_plugin(void) {
    // 返回SPI插件实例
}
```

#### 4. 注册到系统
```c
// main.c - simulator_init()中添加
simulator_plugin_t *spi_plugin = create_spi_plugin();
register_plugin(spi_plugin);
```

## ✨ 核心特性

- **🔄 驱动透明性**: 驱动代码无需修改，直接运行
- **🧩 插件化架构**: 模块化设计，易于扩展新硬件
- **🎯 高仿真度**: 精确模拟寄存器访问和中断时序  
- **⚡ 高性能**: 内存保护机制，零拷贝数据传输
- **🔧 易配置**: 静态表配置，编译时优化
- **📦 跨平台**: 基于标准C99，支持Linux/Unix系统

## 🛠️ 技术特色

1. **内存保护驱动的透明仿真**: 使用mmap + PROT_NONE实现零侵入的寄存器访问拦截
2. **信号驱动的中断系统**: 使用POSIX信号模拟硬件中断，支持异步处理
3. **配置驱动的系统架构**: 通过静态表配置系统行为，支持编译时优化
4. **插件化的硬件抽象**: 每个硬件模块独立实现，支持热插拔和动态加载

## ⚠️ 系统要求

- **操作系统**: Linux/Unix/WSL环境
- **编译器**: GCC (支持C99标准)
- **系统特性**: POSIX信号支持、内存映射功能
- **构建工具**: Make

### Windows环境
在Windows环境下，建议使用WSL (Windows Subsystem for Linux) 来运行本项目。

## 🤝 贡献指南

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🎯 未来规划

- [ ] 支持更多外设模块 (SPI, I2C, Timer等)
- [ ] 添加图形化调试界面
- [ ] 支持脚本化测试用例
- [ ] 增强跨平台兼容性
- [ ] 支持动态插件加载
- [ ] 添加性能分析工具

---

**欢迎⭐️Star本项目，如有问题请提交Issue！**
