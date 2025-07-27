/**
 ******************************************************************************
 * @file    test_framework.c
 * @author  IC Simulator Team
 * @brief   Test Framework Implementation
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
#include <time.h>

/* Private variables ---------------------------------------------------------*/
static test_stats_t global_stats = {0};

/* Test runner functions -----------------------------------------------------*/

/**
 * @brief Run a single test case
 * @param test_case Pointer to test case structure
 * @retval Test result
 */
test_result_t run_test_case(const test_case_t *test_case)
{
    if (test_case == NULL || test_case->test_func == NULL) {
        printf("ERROR: Invalid test case\n");
        return TEST_FAIL;
    }

    printf("Running test: %s - %s\n", test_case->name, test_case->description);
    
    /* Setup for each test */
    test_setup();
    
    /* Run the test */
    test_result_t result = test_case->test_func();
    
    /* Cleanup after each test */
    test_teardown();
    
    /* Print result */
    switch (result) {
        case TEST_PASS:
            printf("✓ PASS: %s\n", test_case->name);
            break;
        case TEST_FAIL:
            printf("✗ FAIL: %s\n", test_case->name);
            break;
        case TEST_SKIP:
            printf("○ SKIP: %s\n", test_case->name);
            break;
        default:
            printf("? UNKNOWN: %s (result=%d)\n", test_case->name, result);
            result = TEST_FAIL;
            break;
    }
    
    return result;
}

/**
 * @brief Run a test suite
 * @param test_cases Array of test cases
 * @param num_tests Number of test cases
 * @param suite_name Name of the test suite
 * @retval Overall test result (PASS if all pass, FAIL if any fail)
 */
test_result_t run_test_suite(const test_case_t test_cases[], uint32_t num_tests, const char *suite_name)
{
    test_stats_t stats = {0};
    stats.total_tests = num_tests;
    
    test_print_separator();
    printf("Starting Test Suite: %s (%u tests)\n", suite_name ? suite_name : "Unknown", num_tests);
    test_print_separator();
    
    clock_t start_time = clock();
    
    for (uint32_t i = 0; i < num_tests; i++) {
        test_result_t result = run_test_case(&test_cases[i]);
        
        switch (result) {
            case TEST_PASS:
                stats.passed_tests++;
                global_stats.passed_tests++;
                break;
            case TEST_FAIL:
                stats.failed_tests++;
                global_stats.failed_tests++;
                break;
            case TEST_SKIP:
                stats.skipped_tests++;
                global_stats.skipped_tests++;
                break;
        }
        
        global_stats.total_tests++;
    }
    
    clock_t end_time = clock();
    double execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    test_print_separator();
    print_test_summary(&stats, suite_name);
    printf("Execution time: %.3f seconds\n", execution_time);
    test_print_separator();
    
    return (stats.failed_tests == 0) ? TEST_PASS : TEST_FAIL;
}

/**
 * @brief Print test summary
 * @param stats Pointer to test statistics
 * @param suite_name Name of the test suite
 */
void print_test_summary(const test_stats_t *stats, const char *suite_name)
{
    if (stats == NULL) {
        printf("ERROR: Invalid test statistics\n");
        return;
    }
    
    printf("\n=== Test Summary: %s ===\n", suite_name ? suite_name : "Unknown");
    printf("Total Tests:  %u\n", stats->total_tests);
    printf("Passed:       %u (%.1f%%)\n", stats->passed_tests, 
           stats->total_tests > 0 ? (100.0 * stats->passed_tests / stats->total_tests) : 0.0);
    printf("Failed:       %u (%.1f%%)\n", stats->failed_tests,
           stats->total_tests > 0 ? (100.0 * stats->failed_tests / stats->total_tests) : 0.0);
    printf("Skipped:      %u (%.1f%%)\n", stats->skipped_tests,
           stats->total_tests > 0 ? (100.0 * stats->skipped_tests / stats->total_tests) : 0.0);
    
    if (stats->failed_tests == 0 && stats->total_tests > 0) {
        printf("Result:       ✓ ALL TESTS PASSED\n");
    } else if (stats->failed_tests > 0) {
        printf("Result:       ✗ SOME TESTS FAILED\n");
    } else {
        printf("Result:       ? NO TESTS RUN\n");
    }
}

/**
 * @brief Test setup function (called before each test)
 */
__attribute__((weak)) void test_setup(void)
{
    /* Default implementation - can be overridden by user */
}

/**
 * @brief Test teardown function (called after each test)
 */
__attribute__((weak)) void test_teardown(void)
{
    /* Default implementation - can be overridden by user */
}

/* Utility functions ---------------------------------------------------------*/

/**
 * @brief Print test header
 * @param test_name Name of the test
 */
void test_print_header(const char *test_name)
{
    printf("\n");
    printf("================================================================================\n");
    printf("  %s\n", test_name ? test_name : "Test");
    printf("================================================================================\n");
}

/**
 * @brief Print separator line
 */
void test_print_separator(void)
{
    printf("--------------------------------------------------------------------------------\n");
}

/**
 * @brief Compare two memory regions
 * @param ptr1 Pointer to first memory region
 * @param ptr2 Pointer to second memory region
 * @param size Number of bytes to compare
 * @retval true if memory regions are identical, false otherwise
 */
bool test_compare_memory(const void *ptr1, const void *ptr2, size_t size)
{
    if (ptr1 == NULL || ptr2 == NULL) {
        return false;
    }
    
    return memcmp(ptr1, ptr2, size) == 0;
}

/**
 * @brief Fill memory with a specific value
 * @param ptr Pointer to memory region
 * @param value Value to fill with
 * @param size Number of bytes to fill
 */
void test_fill_memory(void *ptr, uint8_t value, size_t size)
{
    if (ptr != NULL) {
        memset(ptr, value, size);
    }
}

/**
 * @brief Get global test statistics
 * @retval Pointer to global test statistics
 */
const test_stats_t* get_global_test_stats(void)
{
    return &global_stats;
}

/**
 * @brief Reset global test statistics
 */
void reset_global_test_stats(void)
{
    memset(&global_stats, 0, sizeof(global_stats));
}

/**
 * @brief Print global test summary
 */
void print_global_test_summary(void)
{
    test_print_header("GLOBAL TEST SUMMARY");
    print_test_summary(&global_stats, "All Test Suites");
    test_print_separator();
    
    if (global_stats.failed_tests == 0 && global_stats.total_tests > 0) {
        printf("\n🎉 ALL TESTS PASSED! 🎉\n\n");
    } else if (global_stats.failed_tests > 0) {
        printf("\n❌ SOME TESTS FAILED ❌\n\n");
    } else {
        printf("\n⚠️  NO TESTS WERE RUN ⚠️\n\n");
    }
}

/**
 * @brief Check if all tests passed
 * @retval true if all tests passed, false otherwise
 */
bool all_tests_passed(void)
{
    return (global_stats.failed_tests == 0 && global_stats.total_tests > 0);
}