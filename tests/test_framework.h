/**
 ******************************************************************************
 * @file    test_framework.h
 * @author  IC Simulator Team
 * @brief   Test Framework Header File
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

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Test result types ---------------------------------------------------------*/
typedef enum {
    TEST_PASS = 0,
    TEST_FAIL = 1,
    TEST_SKIP = 2
} test_result_t;

/* Test function pointer type */
typedef test_result_t (*test_function_t)(void);

/* Test suite structure */
typedef struct {
    const char *name;
    test_function_t test_func;
    const char *description;
} test_case_t;

/* Test statistics */
typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
} test_stats_t;

/* Test macros ---------------------------------------------------------------*/
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("ASSERTION FAILED: %s at %s:%d\n", message, __FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("ASSERTION FAILED: %s - Expected: %d, Actual: %d at %s:%d\n", \
                   message, (int)(expected), (int)(actual), __FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    do { \
        if ((ptr) == NULL) { \
            printf("ASSERTION FAILED: %s - Pointer is NULL at %s:%d\n", \
                   message, __FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr, message) \
    do { \
        if ((ptr) != NULL) { \
            printf("ASSERTION FAILED: %s - Pointer is not NULL at %s:%d\n", \
                   message, __FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition, message) \
    TEST_ASSERT((condition) == true, message)

#define TEST_ASSERT_FALSE(condition, message) \
    TEST_ASSERT((condition) == false, message)

#define TEST_PASS_MSG(message) \
    do { \
        printf("TEST PASSED: %s\n", message); \
        return TEST_PASS; \
    } while(0)

#define TEST_FAIL_MSG(message) \
    do { \
        printf("TEST FAILED: %s at %s:%d\n", message, __FILE__, __LINE__); \
        return TEST_FAIL; \
    } while(0)

#define TEST_SKIP_MSG(message) \
    do { \
        printf("TEST SKIPPED: %s\n", message); \
        return TEST_SKIP; \
    } while(0)

/* Test runner functions -----------------------------------------------------*/
test_result_t run_test_case(const test_case_t *test_case);
test_result_t run_test_suite(const test_case_t test_cases[], uint32_t num_tests, const char *suite_name);
void print_test_summary(const test_stats_t *stats, const char *suite_name);
test_result_t run_all_tests(void);

/* Test initialization and cleanup */
void test_setup(void);
void test_teardown(void);

/* Utility functions */
void test_print_header(const char *test_name);
void test_print_separator(void);
bool test_compare_memory(const void *ptr1, const void *ptr2, size_t size);
void test_fill_memory(void *ptr, uint8_t value, size_t size);

/* Global test statistics functions */
const test_stats_t* get_global_test_stats(void);
void reset_global_test_stats(void);
void print_global_test_summary(void);
bool all_tests_passed(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_FRAMEWORK_H */