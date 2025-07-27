# IC Simulator Makefile with Automated Testing

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0
LDFLAGS = -ldl -lpthread

# 目录定义
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests
TEST_BUILD_DIR = build/tests

# 源文件
COMMON_SRCS = $(SRC_DIR)/common/protocol.h
DRIVER_SRCS = $(SRC_DIR)/driver/uart_driver.c $(SRC_DIR)/driver/dma_driver.c
SIM_INTERFACE_SRCS = $(SRC_DIR)/sim_interface/sim_interface.c $(SRC_DIR)/sim_interface/interrupt_manager.c
SIMULATOR_SRCS = $(SRC_DIR)/simulator/plugin_manager.c $(SRC_DIR)/simulator/plugins/uart_plugin.c $(SRC_DIR)/simulator/plugins/dma_plugin.c
MAIN_SRC = $(SRC_DIR)/main.c

# 测试源文件
TEST_FRAMEWORK_SRCS = $(TEST_DIR)/test_framework.c
TEST_SRCS = $(TEST_DIR)/test_uart_driver.c $(TEST_DIR)/test_dma_driver.c $(TEST_DIR)/test_main.c

# 目标文件
OBJS = $(BUILD_DIR)/uart_driver.o $(BUILD_DIR)/interrupt_manager.o $(BUILD_DIR)/dma_driver.o $(BUILD_DIR)/sim_interface.o $(BUILD_DIR)/plugin_manager.o $(BUILD_DIR)/uart_plugin.o $(BUILD_DIR)/dma_plugin.o $(BUILD_DIR)/main.o

# 测试目标文件  
TEST_OBJS = $(TEST_BUILD_DIR)/test_framework.o $(TEST_BUILD_DIR)/test_uart_driver.o $(TEST_BUILD_DIR)/test_dma_driver.o $(TEST_BUILD_DIR)/test_main.o
DRIVER_TEST_OBJS = $(BUILD_DIR)/uart_driver.o $(BUILD_DIR)/interrupt_manager.o $(BUILD_DIR)/dma_driver.o

# 可执行文件
TARGET = $(BIN_DIR)/ic_simulator
TEST_TARGET = $(BIN_DIR)/test_runner

# 默认目标
all: $(TARGET)

# 构建和测试
build-and-test: $(TARGET) $(TEST_TARGET) test

# 创建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_BUILD_DIR):
	mkdir -p $(TEST_BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# 编译主程序目标文件
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

# 编译测试目标文件
$(TEST_BUILD_DIR)/test_framework.o: $(TEST_DIR)/test_framework.c | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -c $< -o $@

$(TEST_BUILD_DIR)/test_uart_driver.o: $(TEST_DIR)/test_uart_driver.c | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -c $< -o $@

$(TEST_BUILD_DIR)/test_dma_driver.o: $(TEST_DIR)/test_dma_driver.c | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -c $< -o $@

$(TEST_BUILD_DIR)/test_main.o: $(TEST_DIR)/test_main.c | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -c $< -o $@

# 链接主程序可执行文件
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# 链接测试可执行文件
$(TEST_TARGET): $(TEST_OBJS) $(DRIVER_TEST_OBJS) | $(BIN_DIR)
	$(CC) $(TEST_OBJS) $(DRIVER_TEST_OBJS) $(LDFLAGS) -o $@

# 清理
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# 运行主程序
run: $(TARGET)
	./$(TARGET)

# 构建测试
build-tests: $(TEST_TARGET)

# 运行所有测试
test: $(TEST_TARGET)
	@echo "=================================================================================="
	@echo "                           RUNNING AUTOMATED TESTS"
	@echo "=================================================================================="
	./$(TEST_TARGET)
	@echo "=================================================================================="
	@echo "                           AUTOMATED TESTS COMPLETED"
	@echo "=================================================================================="

# 运行特定测试套件
test-uart: $(TEST_TARGET)
	@echo "Running UART Driver Tests..."
	./$(TEST_TARGET) --uart

test-dma: $(TEST_TARGET)
	@echo "Running DMA Driver Tests..."
	./$(TEST_TARGET) --dma

# 详细测试输出
test-verbose: $(TEST_TARGET)
	@echo "Running tests with verbose output..."
	./$(TEST_TARGET) --verbose

# 持续集成测试 (返回非零退出码如果测试失败)
ci-test: clean build-tests test

# 调试运行
debug: $(TARGET)
	gdb ./$(TARGET)

debug-tests: $(TEST_TARGET)
	gdb ./$(TEST_TARGET)

# 代码检查 (使用编译器警告作为基本检查)
lint:
	@echo "Running basic code quality checks..."
	$(CC) $(CFLAGS) -I$(SRC_DIR) -fsyntax-only $(SRC_DIR)/driver/*.c $(SRC_DIR)/sim_interface/*.c $(SRC_DIR)/simulator/*.c $(SRC_DIR)/simulator/plugins/*.c $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -fsyntax-only $(TEST_SRCS) $(TEST_FRAMEWORK_SRCS)
	@echo "Code quality check completed."

# 生成测试报告
test-report: $(TEST_TARGET)
	@echo "Generating test report..."
	./$(TEST_TARGET) --verbose > test_report.txt 2>&1
	@echo "Test report generated: test_report.txt"

# 帮助信息
help:
	@echo "Available targets:"
	@echo "  all              - Build main program (default)"
	@echo "  build-and-test   - Build main program and tests, then run tests"
	@echo "  build-tests      - Build test suite only"
	@echo "  test             - Run all automated tests"
	@echo "  test-uart        - Run UART driver tests only"
	@echo "  test-dma         - Run DMA driver tests only"
	@echo "  test-verbose     - Run tests with verbose output"
	@echo "  test-report      - Generate test report file"
	@echo "  ci-test          - Clean build and test (for CI/CD)"
	@echo "  lint             - Run basic code quality checks"
	@echo "  run              - Run main program"
	@echo "  debug            - Debug main program with gdb"
	@echo "  debug-tests      - Debug test suite with gdb"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this help message"

.PHONY: all build-and-test build-tests test test-uart test-dma test-verbose test-report ci-test lint run debug debug-tests clean help
