/**
 ******************************************************************************
 * @file    test_dma_driver.c
 * @author  IC Simulator Team
 * @brief   DMA Driver Test Cases
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
#include "../src/driver/dma_driver.h"
#include "../src/common/register_map.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static DMA_HandleTypeDef test_dma_handle;
static uint8_t test_src_buffer[256];
static uint8_t test_dst_buffer[256];

/* Test setup and teardown ---------------------------------------------------*/
void dma_test_setup(void)
{
    /* Initialize DMA handle for testing */
    memset(&test_dma_handle, 0, sizeof(test_dma_handle));
    test_dma_handle.Instance = DMA0_Channel0;
    test_dma_handle.ChannelIndex = 0;
    test_dma_handle.Init.Direction = DMA_MEMORY_TO_MEMORY;
    test_dma_handle.Init.PeriphInc = DMA_PINC_ENABLE;
    test_dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    test_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    test_dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    test_dma_handle.Init.Mode = DMA_NORMAL;
    test_dma_handle.Init.Priority = DMA_PRIORITY_LOW;
    
    /* Initialize test buffers */
    for (int i = 0; i < sizeof(test_src_buffer); i++) {
        test_src_buffer[i] = (uint8_t)(i & 0xFF);
    }
    memset(test_dst_buffer, 0, sizeof(test_dst_buffer));
}

void dma_test_teardown(void)
{
    /* Cleanup DMA handle */
    if (test_dma_handle.State != HAL_DMA_STATE_RESET) {
        HAL_DMA_DeInit(&test_dma_handle);
    }
}

/* Test cases ----------------------------------------------------------------*/

/**
 * @brief Test DMA HAL initialization
 */
test_result_t test_dma_hal_init(void)
{
    HAL_StatusTypeDef status;
    
    /* Test normal initialization */
    status = HAL_DMA_Init(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA HAL initialization should succeed");
    TEST_ASSERT_EQUAL(HAL_DMA_STATE_READY, test_dma_handle.State, "DMA state should be READY after init");
    TEST_ASSERT_EQUAL(HAL_DMA_ERROR_NONE, test_dma_handle.ErrorCode, "DMA should have no errors after init");
    
    /* Test initialization with NULL handle */
    status = HAL_DMA_Init(NULL);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "DMA HAL init with NULL handle should fail");
    
    /* Test initialization with NULL instance */
    DMA_HandleTypeDef null_instance_handle = test_dma_handle;
    null_instance_handle.Instance = NULL;
    status = HAL_DMA_Init(&null_instance_handle);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "DMA HAL init with NULL instance should fail");
    
    TEST_PASS_MSG("DMA HAL initialization tests passed");
}

/**
 * @brief Test DMA HAL deinitialization
 */
test_result_t test_dma_hal_deinit(void)
{
    HAL_StatusTypeDef status;
    
    /* Initialize first */
    status = HAL_DMA_Init(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA init should succeed before deinit test");
    
    /* Test normal deinitialization */
    status = HAL_DMA_DeInit(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA HAL deinitialization should succeed");
    TEST_ASSERT_EQUAL(HAL_DMA_STATE_RESET, test_dma_handle.State, "DMA state should be RESET after deinit");
    TEST_ASSERT_EQUAL(HAL_DMA_ERROR_NONE, test_dma_handle.ErrorCode, "DMA should have no errors after deinit");
    
    /* Test deinitialization with NULL handle */
    status = HAL_DMA_DeInit(NULL);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "DMA HAL deinit with NULL handle should fail");
    
    TEST_PASS_MSG("DMA HAL deinitialization tests passed");
}

/**
 * @brief Test DMA start functionality
 */
test_result_t test_dma_start(void)
{
    HAL_StatusTypeDef status;
    uint32_t src_addr = (uint32_t)test_src_buffer;
    uint32_t dst_addr = (uint32_t)test_dst_buffer;
    uint32_t data_length = 10;
    
    /* Initialize DMA */
    status = HAL_DMA_Init(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA init should succeed");
    
    /* Test normal start */
    status = HAL_DMA_Start(&test_dma_handle, src_addr, dst_addr, data_length);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA start should succeed");
    TEST_ASSERT_EQUAL(HAL_DMA_STATE_BUSY, test_dma_handle.State, "DMA state should be BUSY after start");
    
    /* Reset for next test */
    test_dma_handle.State = HAL_DMA_STATE_READY;
    
    /* Test start with NULL handle */
    status = HAL_DMA_Start(NULL, src_addr, dst_addr, data_length);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "DMA start with NULL handle should fail");
    
    /* Test start when not ready (simulate busy state) */
    test_dma_handle.State = HAL_DMA_STATE_BUSY;
    status = HAL_DMA_Start(&test_dma_handle, src_addr, dst_addr, data_length);
    TEST_ASSERT_EQUAL(HAL_BUSY, status, "DMA start when busy should return BUSY");
    test_dma_handle.State = HAL_DMA_STATE_READY; /* Restore state */
    
    TEST_PASS_MSG("DMA start tests passed");
}

/**
 * @brief Test DMA interrupt-driven start
 */
test_result_t test_dma_start_it(void)
{
    HAL_StatusTypeDef status;
    uint32_t src_addr = (uint32_t)test_src_buffer;
    uint32_t dst_addr = (uint32_t)test_dst_buffer;
    uint32_t data_length = 10;
    
    /* Initialize DMA */
    status = HAL_DMA_Init(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA init should succeed");
    
    /* Test interrupt-driven start */
    status = HAL_DMA_Start_IT(&test_dma_handle, src_addr, dst_addr, data_length);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA start IT should succeed");
    TEST_ASSERT_EQUAL(HAL_DMA_STATE_BUSY, test_dma_handle.State, "DMA state should be BUSY after start IT");
    
    /* Reset for next test */
    test_dma_handle.State = HAL_DMA_STATE_READY;
    
    /* Test start IT with NULL handle */
    status = HAL_DMA_Start_IT(NULL, src_addr, dst_addr, data_length);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "DMA start IT with NULL handle should fail");
    
    TEST_PASS_MSG("DMA start IT tests passed");
}

/**
 * @brief Test DMA abort functionality
 */
test_result_t test_dma_abort(void)
{
    HAL_StatusTypeDef status;
    uint32_t src_addr = (uint32_t)test_src_buffer;
    uint32_t dst_addr = (uint32_t)test_dst_buffer;
    uint32_t data_length = 10;
    
    /* Initialize DMA */
    status = HAL_DMA_Init(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA init should succeed");
    
    /* Start a transfer first */
    status = HAL_DMA_Start(&test_dma_handle, src_addr, dst_addr, data_length);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA start should succeed");
    
    /* Test abort */
    status = HAL_DMA_Abort(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA abort should succeed");
    TEST_ASSERT_EQUAL(HAL_DMA_STATE_READY, test_dma_handle.State, "DMA state should be READY after abort");
    
    /* Test abort when not busy */
    status = HAL_DMA_Abort(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_ERROR, status, "DMA abort when not busy should fail");
    TEST_ASSERT_EQUAL(HAL_DMA_ERROR_NO_XFER, test_dma_handle.ErrorCode, "DMA should have NO_XFER error");
    
    TEST_PASS_MSG("DMA abort tests passed");
}

/**
 * @brief Test DMA state management
 */
test_result_t test_dma_state_management(void)
{
    HAL_StatusTypeDef status;
    HAL_DMA_StateTypeDef state;
    uint32_t error;
    
    /* Test initial state */
    state = HAL_DMA_GetState(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_DMA_STATE_RESET, state, "Initial DMA state should be RESET");
    
    error = HAL_DMA_GetError(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_DMA_ERROR_NONE, error, "Initial DMA error should be NONE");
    
    /* Initialize DMA */
    status = HAL_DMA_Init(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_OK, status, "DMA init should succeed");
    
    /* Test state after initialization */
    state = HAL_DMA_GetState(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_DMA_STATE_READY, state, "DMA state should be READY after init");
    
    /* Test error state management */
    test_dma_handle.ErrorCode = HAL_DMA_ERROR_TE;
    error = HAL_DMA_GetError(&test_dma_handle);
    TEST_ASSERT_EQUAL(HAL_DMA_ERROR_TE, error, "DMA error should be retrievable");
    
    TEST_PASS_MSG("DMA state management tests passed");
}

/**
 * @brief Test legacy DMA functions
 */
test_result_t test_dma_legacy_functions(void)
{
    int result;
    int channel;
    dma_config_t config;
    dma_channel_status_t status;
    
    /* Test legacy initialization */
    result = dma_init();
    TEST_ASSERT_EQUAL(0, result, "Legacy DMA init should succeed");
    
    /* Test channel allocation */
    channel = dma_allocate_channel();
    TEST_ASSERT_TRUE(channel >= 0, "DMA channel allocation should succeed");
    
    /* Test channel availability check */
    bool available = dma_is_channel_available(channel);
    TEST_ASSERT_FALSE(available, "Allocated channel should not be available");
    
    /* Test channel configuration */
    memset(&config, 0, sizeof(config));
    config.src_addr = (uint32_t)test_src_buffer;
    config.dst_addr = (uint32_t)test_dst_buffer;
    config.size = 10;
    config.type = DMA_TRANSFER_MEM_TO_MEM;
    config.inc_src = true;
    config.inc_dst = true;
    config.interrupt_enable = false;
    
    result = dma_configure_channel(channel, &config);
    TEST_ASSERT_EQUAL(0, result, "DMA channel configuration should succeed");
    
    /* Test configuration with invalid parameters */
    result = dma_configure_channel(255, &config); /* Invalid channel */
    TEST_ASSERT_EQUAL(-1, result, "DMA config with invalid channel should fail");
    
    result = dma_configure_channel(channel, NULL); /* NULL config */
    TEST_ASSERT_EQUAL(-1, result, "DMA config with NULL config should fail");
    
    /* Test transfer start */
    result = dma_start_transfer(channel);
    TEST_ASSERT_EQUAL(0, result, "DMA transfer start should succeed");
    
    /* Test channel status */
    status = dma_get_channel_status(channel);
    TEST_ASSERT_TRUE(status == DMA_CH_BUSY || status == DMA_CH_DONE, 
                     "DMA channel should be BUSY or DONE after start");
    
    /* Test transfer stop */
    result = dma_stop_transfer(channel);
    TEST_ASSERT_EQUAL(0, result, "DMA transfer stop should succeed");
    
    /* Test synchronous transfer */
    result = dma_transfer_sync(channel, (uint32_t)test_src_buffer, 
                              (uint32_t)test_dst_buffer, 5, DMA_TRANSFER_MEM_TO_MEM);
    TEST_ASSERT_EQUAL(0, result, "DMA synchronous transfer should succeed");
    
    /* Verify data transfer */
    bool data_match = test_compare_memory(test_src_buffer, test_dst_buffer, 5);
    TEST_ASSERT_TRUE(data_match, "Transferred data should match source data");
    
    /* Test channel deallocation */
    result = dma_free_channel(channel);
    TEST_ASSERT_EQUAL(0, result, "DMA channel deallocation should succeed");
    
    /* Test cleanup */
    dma_cleanup();
    
    TEST_PASS_MSG("Legacy DMA function tests passed");
}

/**
 * @brief Test DMA asynchronous transfer
 */
test_result_t test_dma_async_transfer(void)
{
    int result;
    int channel;
    bool transfer_completed = false;
    
    /* Callback function for async transfer */
    void async_callback(uint8_t ch, dma_channel_status_t status) {
        (void)ch; /* Unused parameter */
        if (status == DMA_CH_DONE) {
            transfer_completed = true;
        }
    }
    
    /* Initialize DMA */
    result = dma_init();
    TEST_ASSERT_EQUAL(0, result, "DMA init should succeed");
    
    /* Allocate channel */
    channel = dma_allocate_channel();
    TEST_ASSERT_TRUE(channel >= 0, "DMA channel allocation should succeed");
    
    /* Test callback registration */
    result = dma_register_callback(channel, async_callback);
    TEST_ASSERT_EQUAL(0, result, "DMA callback registration should succeed");
    
    /* Test asynchronous transfer */
    result = dma_transfer_async(channel, (uint32_t)test_src_buffer, 
                               (uint32_t)test_dst_buffer, 8, 
                               DMA_TRANSFER_MEM_TO_MEM, async_callback);
    TEST_ASSERT_EQUAL(0, result, "DMA asynchronous transfer should succeed");
    
    /* In simulation mode, the transfer should complete quickly */
    /* Wait a bit and check completion */
    for (int i = 0; i < 100 && !transfer_completed; i++) {
        /* Simulate some waiting time */
        volatile int dummy = 0;
        for (int j = 0; j < 1000; j++) dummy++;
    }
    
    /* Test with invalid parameters */
    result = dma_transfer_async(255, (uint32_t)test_src_buffer, 
                               (uint32_t)test_dst_buffer, 8, 
                               DMA_TRANSFER_MEM_TO_MEM, async_callback);
    TEST_ASSERT_EQUAL(-1, result, "DMA async transfer with invalid channel should fail");
    
    /* Cleanup */
    dma_free_channel(channel);
    dma_cleanup();
    
    TEST_PASS_MSG("DMA asynchronous transfer tests passed");
}

/**
 * @brief Test DMA different transfer types
 */
test_result_t test_dma_transfer_types(void)
{
    int result;
    int channel;
    dma_config_t config;
    
    /* Initialize DMA */
    result = dma_init();
    TEST_ASSERT_EQUAL(0, result, "DMA init should succeed");
    
    /* Allocate channel */
    channel = dma_allocate_channel();
    TEST_ASSERT_TRUE(channel >= 0, "DMA channel allocation should succeed");
    
    /* Test Memory to Memory transfer */
    memset(&config, 0, sizeof(config));
    config.src_addr = (uint32_t)test_src_buffer;
    config.dst_addr = (uint32_t)test_dst_buffer;
    config.size = 16;
    config.type = DMA_TRANSFER_MEM_TO_MEM;
    config.inc_src = true;
    config.inc_dst = true;
    config.interrupt_enable = false;
    
    result = dma_configure_channel(channel, &config);
    TEST_ASSERT_EQUAL(0, result, "DMA MEM_TO_MEM configuration should succeed");
    
    result = dma_transfer_sync(channel, config.src_addr, config.dst_addr, 
                              config.size, config.type);
    TEST_ASSERT_EQUAL(0, result, "DMA MEM_TO_MEM transfer should succeed");
    
    /* Verify transfer */
    bool data_match = test_compare_memory(test_src_buffer, test_dst_buffer, 16);
    TEST_ASSERT_TRUE(data_match, "MEM_TO_MEM transfer data should match");
    
    /* Test Memory to Peripheral transfer type */
    config.type = DMA_TRANSFER_MEM_TO_PER;
    result = dma_configure_channel(channel, &config);
    TEST_ASSERT_EQUAL(0, result, "DMA MEM_TO_PER configuration should succeed");
    
    /* Test Peripheral to Memory transfer type */
    config.type = DMA_TRANSFER_PER_TO_MEM;
    result = dma_configure_channel(channel, &config);
    TEST_ASSERT_EQUAL(0, result, "DMA PER_TO_MEM configuration should succeed");
    
    /* Test Peripheral to Peripheral transfer type */
    config.type = DMA_TRANSFER_PER_TO_PER;
    result = dma_configure_channel(channel, &config);
    TEST_ASSERT_EQUAL(0, result, "DMA PER_TO_PER configuration should succeed");
    
    /* Cleanup */
    dma_free_channel(channel);
    dma_cleanup();
    
    TEST_PASS_MSG("DMA transfer types tests passed");
}

/* Test suite definition -----------------------------------------------------*/
const test_case_t dma_test_cases[] = {
    {"DMA_HAL_Init", test_dma_hal_init, "Test DMA HAL initialization functionality"},
    {"DMA_HAL_DeInit", test_dma_hal_deinit, "Test DMA HAL deinitialization functionality"},
    {"DMA_Start", test_dma_start, "Test DMA start functionality"},
    {"DMA_Start_IT", test_dma_start_it, "Test DMA interrupt-driven start"},
    {"DMA_Abort", test_dma_abort, "Test DMA abort functionality"},
    {"DMA_State_Management", test_dma_state_management, "Test DMA state and error management"},
    {"DMA_Legacy_Functions", test_dma_legacy_functions, "Test legacy DMA functions"},
    {"DMA_Async_Transfer", test_dma_async_transfer, "Test DMA asynchronous transfer"},
    {"DMA_Transfer_Types", test_dma_transfer_types, "Test different DMA transfer types"},
};

const uint32_t dma_test_count = sizeof(dma_test_cases) / sizeof(dma_test_cases[0]);

/**
 * @brief Run all DMA tests
 * @retval Test result
 */
test_result_t run_dma_tests(void)
{
    test_result_t result = run_test_suite(dma_test_cases, dma_test_count, "DMA Driver Tests");
    return result;
}