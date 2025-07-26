# IC Simulator Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0
LDFLAGS = -ldl -lpthread

# 目录定义
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# 源文件
COMMON_SRCS = $(SRC_DIR)/common/protocol.h
DRIVER_SRCS = $(SRC_DIR)/driver/uart_driver.c $(SRC_DIR)/driver/dma_driver.c
SIM_INTERFACE_SRCS = $(SRC_DIR)/sim_interface/sim_interface.c $(SRC_DIR)/sim_interface/interrupt_manager.c
SIMULATOR_SRCS = $(SRC_DIR)/simulator/plugin_manager.c $(SRC_DIR)/simulator/plugins/uart_plugin.c $(SRC_DIR)/simulator/plugins/dma_plugin.c
MAIN_SRC = $(SRC_DIR)/main.c

# 目标文件
OBJS = $(BUILD_DIR)/uart_driver.o $(BUILD_DIR)/interrupt_manager.o $(BUILD_DIR)/dma_driver.o $(BUILD_DIR)/sim_interface.o $(BUILD_DIR)/plugin_manager.o $(BUILD_DIR)/uart_plugin.o $(BUILD_DIR)/dma_plugin.o $(BUILD_DIR)/main.o

# 可执行文件
TARGET = $(BIN_DIR)/ic_simulator

# 默认目标
all: $(TARGET)

# 创建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# 编译目标文件
$(BUILD_DIR)/uart_driver.o: $(SRC_DIR)/driver/uart_driver.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/interrupt_manager.o: $(SRC_DIR)/sim_interface/interrupt_manager.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/dma_driver.o: $(SRC_DIR)/driver/dma_driver.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/sim_interface.o: $(SRC_DIR)/sim_interface/sim_interface.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/plugin_manager.o: $(SRC_DIR)/simulator/plugin_manager.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/uart_plugin.o: $(SRC_DIR)/simulator/plugins/uart_plugin.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/dma_plugin.o: $(SRC_DIR)/simulator/plugins/dma_plugin.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

# 链接可执行文件
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# 清理
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# 运行
run: $(TARGET)
	./$(TARGET)

# 调试运行
debug: $(TARGET)
	gdb ./$(TARGET)

# UART DMA测试程序
test_uart_dma: $(BIN_DIR)/test_uart_dma

$(BIN_DIR)/test_uart_dma: test_uart_dma.c $(BUILD_DIR)/uart_driver.o $(BUILD_DIR)/interrupt_manager.o $(BUILD_DIR)/dma_driver.o $(BUILD_DIR)/sim_interface.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) test_uart_dma.c $(BUILD_DIR)/uart_driver.o $(BUILD_DIR)/interrupt_manager.o $(BUILD_DIR)/dma_driver.o $(BUILD_DIR)/sim_interface.o $(LDFLAGS) -o $@

.PHONY: all clean run debug test_uart_dma
