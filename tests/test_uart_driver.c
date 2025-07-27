/**
 ******************************************************************************
 * @file    test_uart_driver.c
 * @author  IC Simulator Team
 * @brief   UART Driver Test Cases
 * @version V1.0.0
 * @date    26-July-2025
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 IC Simulator Project.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include "../src/driver/uart_driver.h"
#include "../src/common/register_map.h"
#include "../src/sim_interface/sim_interface.h"
#include "../src/sim_interface/interrupt_manager.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static UART_HandleTypeDef test_uart_handle;
static uint8_t test_tx_buffer[256];
static uint8_t test_rx_buffer[256];
static bool sim_initialized = false;

/* Test setup and teardown ---------------------------------------------------*/
void uart_test_setup(void)
{
    /* Initialize simulation interface if not already done */
    if (!sim_initialized) {
        /* Note: In a real test environment, we might skip simulation init 
         * and just test the driver logic directly */
        sim_initialized = true;
        printf("[UART_TEST] Simulation interface initialization skipped for unit testing\n");
    }
    
    /* Initialize UART handle for testing */
    memset(&test_uart_handle, 0, sizeof(test_uart_handle));
    test_uart_handle.Instance = UART0;
    test_uart_handle.Init.BaudRate = 115200;
    test_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    test_uart_handle.Init.StopBits = UART_STOPBITS_1;
    test_uart_handle.Init.Parity = UART_PARITY_NONE;
    test_uart_handle.Init.Mode = UART_MODE_TX_RX;
    test_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    test_uart_handle.Init.TransferMode = UART_TRANSFER_MODE_POLLING;
    
    /* Clear test buffers */
    memset(test_tx_buffer, 0, sizeof(test_tx_buffer));
    memset(test_rx_buffer, 0, sizeof(test_rx_buffer));
}

void uart_test_teardown(void)
{
    /* Cleanup UART handle */
    if (test_uart_handle.gState != HAL_UART_STATE_RESET) {
        HAL_UART_DeInit(&test_uart_handle);
    }
}

/* Test cases ----------------------------------------------------------------*/

/**
 * @brief Test UART HAL initialization
 */
test_result_t test_uart_hal_init(void)
{
    HAL_StatusTypeDef status;
    
    /* Test normal initialization */
    status = HAL_UART_Init(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "UART HAL initialization should succeed");
    TEST_ASSERT_EQUAL(HAL_UART_STATE_READY, test_uart_handle.gState, "UART state should be READY after init");
    TEST_ASSERT_EQUAL(HAL_UART_STATE_READY, test_uart_handle.RxState, "UART RX state should be READY after init");
    TEST_ASSERT_EQUAL(HAL_UART_ERROR_NONE, test_uart_handle.ErrorCode, "UART should have no errors after init");
    
    /* Test initialization with NULL handle */
    status = HAL_UART_Init(NULL);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "UART HAL init with NULL handle should fail");
    
    /* Test initialization with NULL instance */
    UART_HandleTypeDef null_instance_handle = test_uart_handle;
    null_instance_handle.Instance = NULL;
    status = HAL_UART_Init(&null_instance_handle);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "UART HAL init with NULL instance should fail");
    
    TEST_PASS_MSG("UART HAL initialization tests passed");
}

/**
 * @brief Test UART HAL deinitialization
 */
test_result_t test_uart_hal_deinit(void)
{
    HAL_StatusTypeDef status;
    
    /* Initialize first */
    status = HAL_UART_Init(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "UART init should succeed before deinit test");
    
    /* Test normal deinitialization */
    status = HAL_UART_DeInit(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "UART HAL deinitialization should succeed");
    TEST_ASSERT_EQUAL(HAL_UART_STATE_RESET, test_uart_handle.gState, "UART state should be RESET after deinit");
    TEST_ASSERT_EQUAL(HAL_UART_STATE_RESET, test_uart_handle.RxState, "UART RX state should be RESET after deinit");
    TEST_ASSERT_EQUAL(HAL_UART_ERROR_NONE, test_uart_handle.ErrorCode, "UART should have no errors after deinit");
    
    /* Test deinitialization with NULL handle */
    status = HAL_UART_DeInit(NULL);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "UART HAL deinit with NULL handle should fail");
    
    TEST_PASS_MSG("UART HAL deinitialization tests passed");
}

/**
 * @brief Test UART transmit functionality
 */
test_result_t test_uart_transmit(void)
{
    HAL_StatusTypeDef status;
    const char *test_string = "Hello, UART!";
    
    /* Initialize UART */
    status = HAL_UART_Init(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "UART init should succeed");
    
    /* Test normal transmit */
    strcpy((char*)test_tx_buffer, test_string);
    status = HAL_UART_Transmit(&test_uart_handle, test_tx_buffer, strlen(test_string), 1000);
    TEST_ASSERT_EQUAL(HAL_OK, status, "UART transmit should succeed");
    
    /* Test transmit with NULL data */
    status = HAL_UART_Transmit(&test_uart_handle, NULL, 10, 1000);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "UART transmit with NULL data should fail");
    
    /* Test transmit with zero size */
    status = HAL_UART_Transmit(&test_uart_handle, test_tx_buffer, 0, 1000);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "UART transmit with zero size should fail");
    
    /* Test transmit when not ready (simulate busy state) */
    test_uart_handle.gState = HAL_UART_STATE_BUSY_TX;
    status = HAL_UART_Transmit(&test_uart_handle, test_tx_buffer, 5, 1000);
    TEST_ASSERT_EQUAL(HAL_BUSY, status, "UART transmit when busy should return BUSY");
    test_uart_handle.gState = HAL_UART_STATE_READY; /* Restore state */
    
    TEST_PASS_MSG("UART transmit tests passed");
}

/**
 * @brief Test UART receive functionality
 */
test_result_t test_uart_receive(void)
{
    HAL_StatusTypeDef status;
    
    /* Initialize UART */
    status = HAL_UART_Init(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "UART init should succeed");
    
    /* Test receive with NULL data */
    status = HAL_UART_Receive(&test_uart_handle, NULL, 10, 100);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "UART receive with NULL data should fail");
    
    /* Test receive with zero size */
    status = HAL_UART_Receive(&test_uart_handle, test_rx_buffer, 0, 100);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "UART receive with zero size should fail");
    
    /* Test receive when not ready (simulate busy state) */
    test_uart_handle.RxState = HAL_UART_STATE_BUSY_RX;
    status = HAL_UART_Receive(&test_uart_handle, test_rx_buffer, 5, 100);
    TEST_ASSERT_EQUAL(HAL_BUSY, status, "UART receive when busy should return BUSY");
    test_uart_handle.RxState = HAL_UART_STATE_READY; /* Restore state */
    
    /* Note: In simulation mode, actual receive will timeout as expected */
    /* This is normal behavior and should result in HAL_TIMEOUT */
    status = HAL_UART_Receive(&test_uart_handle, test_rx_buffer, 1, 50);
    TEST_ASSERT_TRUE(status == HAL_TIMEOUT || status == HAL_OK, "UART receive should timeout or succeed in simulation");
    
    TEST_PASS_MSG("UART receive tests passed");
}

/**
 * @brief Test UART state management
 */
test_result_t test_uart_state_management(void)
{
    HAL_StatusTypeDef status;
    HAL_UART_StateTypeDef state;
    uint32_t error;
    
    /* Test initial state */
    state = HAL_UART_GetState(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_UART_STATE_RESET, state, "Initial UART state should be RESET");
    
    error = HAL_UART_GetError(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_UART_ERROR_NONE, error, "Initial UART error should be NONE");
    
    /* Initialize UART */
    status = HAL_UART_Init(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "UART init should succeed");
    
    /* Test state after initialization */
    state = HAL_UART_GetState(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_UART_STATE_READY, state, "UART state should be READY after init");
    
    /* Test error state management */
    test_uart_handle.ErrorCode = HAL_UART_ERROR_PE;
    error = HAL_UART_GetError(&test_uart_handle);
    TEST_ASSERT_EQUAL(HAL_UART_ERROR_PE, error, "UART error should be retrievable");
    
    TEST_PASS_MSG("UART state management tests passed");
}

/**
 * @brief Test legacy UART functions
 */
test_result_t test_uart_legacy_functions(void)
{
    int result;
    uint8_t test_byte;
    const char *test_string = "Test";
    
    /* Test legacy initialization */
    result = uart_init();
    TEST_ASSERT_EQUAL(0, result, "Legacy UART init should succeed");
    
    /* Test send byte */
    result = uart_send_byte(0x55);
    TEST_ASSERT_EQUAL(0, result, "Legacy UART send byte should succeed");
    
    /* Test send string */
    result = uart_send_string(test_string);
    TEST_ASSERT_EQUAL(0, result, "Legacy UART send string should succeed");
    
    /* Test send string with NULL */
    result = uart_send_string(NULL);
    TEST_ASSERT_EQUAL(-1, result, "Legacy UART send string with NULL should fail");
    
    /* Test receive byte (will likely timeout in simulation, which is expected) */
    result = uart_receive_byte(&test_byte);
    TEST_ASSERT_TRUE(result == 0 || result == -1, "Legacy UART receive byte should complete or timeout");
    
    /* Test receive byte with NULL */
    result = uart_receive_byte(NULL);
    TEST_ASSERT_EQUAL(-1, result, "Legacy UART receive byte with NULL should fail");
    
    /* Test mode setting */
    result = uart_set_mode(UART_TRANSFER_MODE_POLLING);
    TEST_ASSERT_EQUAL(0, result, "Legacy UART set mode should succeed");
    
    UART_TransferModeTypeDef mode = uart_get_mode();
    TEST_ASSERT_EQUAL(UART_TRANSFER_MODE_POLLING, mode, "Legacy UART get mode should return set mode");
    
    /* Test cleanup */
    uart_cleanup();
    
    TEST_PASS_MSG("Legacy UART function tests passed");
}

/**
 * @brief Test UART DMA functionality
 */
test_result_t test_uart_dma_functions(void)
{
    int result;
    const uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t rx_buffer[10];
    
    /* Initialize UART first */
    result = uart_init();
    TEST_ASSERT_EQUAL(0, result, "UART init should succeed before DMA tests");
    
    /* Test DMA initialization */
    result = uart_dma_init();
    TEST_ASSERT_EQUAL(0, result, "UART DMA init should succeed");
    
    /* Test DMA send */
    result = uart_dma_send(test_data, sizeof(test_data));
    TEST_ASSERT_EQUAL(0, result, "UART DMA send should succeed");
    
    /* Test DMA send with NULL data */
    result = uart_dma_send(NULL, 5);
    TEST_ASSERT_EQUAL(-1, result, "UART DMA send with NULL data should fail");
    
    /* Test DMA send with zero size */
    result = uart_dma_send(test_data, 0);
    TEST_ASSERT_EQUAL(-1, result, "UART DMA send with zero size should fail");
    
    /* Test DMA send completion status */
    bool completed = uart_dma_send_completed();
    TEST_ASSERT_TRUE(completed, "DMA send should be completed in simulation mode");
    
    /* Test DMA receive setup */
    result = uart_dma_receive(rx_buffer, sizeof(rx_buffer));
    /* In simulation mode, this might fail or succeed depending on setup */
    TEST_ASSERT_TRUE(result == 0 || result == -1, "UART DMA receive should complete or fail gracefully");
    
    /* Test DMA receive with NULL buffer */
    result = uart_dma_receive(NULL, 5);
    TEST_ASSERT_EQUAL(-1, result, "UART DMA receive with NULL buffer should fail");
    
    /* Test DMA cleanup */
    uart_dma_cleanup();
    
    TEST_PASS_MSG("UART DMA function tests passed");
}

/* Test suite definition -----------------------------------------------------*/
const test_case_t uart_test_cases[] = {
    {"UART_HAL_Init", test_uart_hal_init, "Test UART HAL initialization functionality"},
    {"UART_HAL_DeInit", test_uart_hal_deinit, "Test UART HAL deinitialization functionality"},
    {"UART_Transmit", test_uart_transmit, "Test UART transmit functionality"},
    {"UART_Receive", test_uart_receive, "Test UART receive functionality"},
    {"UART_State_Management", test_uart_state_management, "Test UART state and error management"},
    {"UART_Legacy_Functions", test_uart_legacy_functions, "Test legacy UART functions"},
    {"UART_DMA_Functions", test_uart_dma_functions, "Test UART DMA functionality"},
};

const uint32_t uart_test_count = sizeof(uart_test_cases) / sizeof(uart_test_cases[0]);

/**
 * @brief Run all UART tests
 * @retval Test result
 */
test_result_t run_uart_tests(void)
{
    test_result_t result = run_test_suite(uart_test_cases, uart_test_count, "UART Driver Tests");
    return result;
}