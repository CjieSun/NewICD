/**
 ******************************************************************************
 * @file    test_main.c
 * @author  IC Simulator Team
 * @brief   Main Test Runner
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External test functions ---------------------------------------------------*/
extern test_result_t run_uart_tests(void);
extern test_result_t run_dma_tests(void);

/* Private function prototypes -----------------------------------------------*/
static void print_test_banner(void);
static void print_usage(void);
static int run_specific_test_suite(const char* suite_name);

/**
 * @brief Print test banner
 */
static void print_test_banner(void)
{
    printf("\n");
    printf("================================================================================\n");
    printf("                        IC SIMULATOR TEST SUITE\n");
    printf("                         CMSIS Driver Testing\n");
    printf("================================================================================\n");
    printf("Version: 1.0.0\n");
    printf("Date: 26-July-2025\n");
    printf("Description: Comprehensive test suite for UART and DMA drivers\n");
    printf("================================================================================\n");
    printf("\n");
}

/**
 * @brief Print usage information
 */
static void print_usage(void)
{
    printf("Usage: test_runner [options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  --help, -h         Show this help message\n");
    printf("  --uart             Run only UART driver tests\n");
    printf("  --dma              Run only DMA driver tests\n");
    printf("  --all              Run all test suites (default)\n");
    printf("  --verbose, -v      Enable verbose output\n");
    printf("\n");
    printf("Examples:\n");
    printf("  test_runner              # Run all tests\n");
    printf("  test_runner --uart       # Run only UART tests\n");
    printf("  test_runner --dma        # Run only DMA tests\n");
    printf("  test_runner --verbose    # Run all tests with verbose output\n");
    printf("\n");
}

/**
 * @brief Run specific test suite
 * @param suite_name Name of the test suite to run
 * @retval 0 on success, -1 on error
 */
static int run_specific_test_suite(const char* suite_name)
{
    test_result_t result = TEST_FAIL;
    
    if (strcmp(suite_name, "uart") == 0) {
        printf("Running UART Driver Test Suite...\n");
        result = run_uart_tests();
    } else if (strcmp(suite_name, "dma") == 0) {
        printf("Running DMA Driver Test Suite...\n");
        result = run_dma_tests();
    } else {
        printf("Error: Unknown test suite '%s'\n", suite_name);
        return -1;
    }
    
    return (result == TEST_PASS) ? 0 : -1;
}

/**
 * @brief Main test runner function
 * @param argc Number of command line arguments
 * @param argv Command line arguments
 * @retval 0 on success, non-zero on failure
 */
int main(int argc, char* argv[])
{
    bool run_uart = false;
    bool run_dma = false;
    bool run_all = true;
    bool verbose = false;
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(argv[i], "--uart") == 0) {
            run_uart = true;
            run_all = false;
        } else if (strcmp(argv[i], "--dma") == 0) {
            run_dma = true;
            run_all = false;
        } else if (strcmp(argv[i], "--all") == 0) {
            run_all = true;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else {
            printf("Error: Unknown option '%s'\n", argv[i]);
            print_usage();
            return 1;
        }
    }
    
    /* Print banner */
    print_test_banner();
    
    /* Reset global test statistics */
    reset_global_test_stats();
    
    int exit_code = 0;
    
    /* Run requested test suites */
    if (run_all) {
        printf("Running all test suites...\n\n");
        
        /* Run UART tests */
        test_result_t uart_result = run_uart_tests();
        if (uart_result != TEST_PASS) {
            exit_code = 1;
        }
        
        printf("\n");
        
        /* Run DMA tests */
        test_result_t dma_result = run_dma_tests();
        if (dma_result != TEST_PASS) {
            exit_code = 1;
        }
        
    } else {
        /* Run specific test suites */
        if (run_uart) {
            if (run_specific_test_suite("uart") != 0) {
                exit_code = 1;
            }
        }
        
        if (run_dma) {
            if (run_specific_test_suite("dma") != 0) {
                exit_code = 1;
            }
        }
    }
    
    /* Print global summary */
    printf("\n");
    print_global_test_summary();
    
    /* Print final result */
    if (exit_code == 0) {
        printf("🎉 ALL TESTS COMPLETED SUCCESSFULLY! 🎉\n");
        printf("Exit Code: 0 (SUCCESS)\n");
    } else {
        printf("❌ SOME TESTS FAILED ❌\n");
        printf("Exit Code: %d (FAILURE)\n", exit_code);
    }
    
    if (verbose) {
        const test_stats_t* stats = get_global_test_stats();
        printf("\nDetailed Statistics:\n");
        printf("- Total test cases executed: %u\n", stats->total_tests);
        printf("- Successful test cases: %u\n", stats->passed_tests);
        printf("- Failed test cases: %u\n", stats->failed_tests);
        printf("- Skipped test cases: %u\n", stats->skipped_tests);
        printf("- Success rate: %.1f%%\n", 
               stats->total_tests > 0 ? (100.0 * stats->passed_tests / stats->total_tests) : 0.0);
    }
    
    printf("\n");
    return exit_code;
}

/**
 * @brief Override test setup for global setup
 */
void test_setup(void)
{
    /* Global test setup - can be customized */
    /* This function is called before each individual test */
}

/**
 * @brief Override test teardown for global cleanup
 */
void test_teardown(void)
{
    /* Global test teardown - can be customized */
    /* This function is called after each individual test */
}