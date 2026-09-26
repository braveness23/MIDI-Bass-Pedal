/*
 * test_framework.h — Lightweight test harness
 *
 * Usage:
 *   TEST(suite_name, test_name) { ... }
 *   ASSERT_EQ(a, b)
 *   ASSERT_NE(a, b)
 *   ASSERT_TRUE(expr)
 *   ASSERT_FALSE(expr)
 *
 * At the bottom of each test file add:
 *   RUN_ALL_TESTS()
 *
 * Each test file is compiled to a standalone executable.
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------
 * Counters (defined in each test translation unit via DEFINE_TEST_COUNTERS)
 * ------------------------------------------------------------------------- */
static int _tests_run    = 0;
static int _tests_passed = 0;
static int _tests_failed = 0;

/* -------------------------------------------------------------------------
 * Internal failure helper
 * ------------------------------------------------------------------------- */
static void _test_fail(const char *file, int line, const char *expr)
{
    printf("  FAIL  %s:%d  %s\n", file, line, expr);
    _tests_failed++;
}

/* -------------------------------------------------------------------------
 * Assertion macros
 * ------------------------------------------------------------------------- */
#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            _test_fail(__FILE__, __LINE__, "ASSERT_TRUE(" #expr ")"); \
            return; \
        } \
    } while (0)

#define ASSERT_FALSE(expr) \
    do { \
        if ((expr)) { \
            _test_fail(__FILE__, __LINE__, "ASSERT_FALSE(" #expr ")"); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            char _buf[256]; \
            snprintf(_buf, sizeof(_buf), \
                     "ASSERT_EQ(%s, %s)  left=%lld  right=%lld", \
                     #a, #b, (long long)(a), (long long)(b)); \
            _test_fail(__FILE__, __LINE__, _buf); \
            return; \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            char _buf[256]; \
            snprintf(_buf, sizeof(_buf), \
                     "ASSERT_NE(%s, %s)  both=%lld", \
                     #a, #b, (long long)(a)); \
            _test_fail(__FILE__, __LINE__, _buf); \
            return; \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            char _buf[512]; \
            snprintf(_buf, sizeof(_buf), \
                     "ASSERT_STR_EQ(%s, %s)  left=\"%s\"  right=\"%s\"", \
                     #a, #b, (a), (b)); \
            _test_fail(__FILE__, __LINE__, _buf); \
            return; \
        } \
    } while (0)

/* -------------------------------------------------------------------------
 * TEST macro — declares a static void function and registers it
 * ------------------------------------------------------------------------- */
typedef void (*_TestFn)(void);

#define _TEST_MAX 256
static _TestFn  _test_fns[_TEST_MAX];
static const char *_test_names[_TEST_MAX];
static int _test_count = 0;

/* Register at file-scope using a constructor function */
#define TEST(suite, name) \
    static void _test_##suite##_##name(void); \
    __attribute__((constructor)) \
    static void _register_##suite##_##name(void) { \
        if (_test_count < _TEST_MAX) { \
            _test_fns[_test_count]   = _test_##suite##_##name; \
            _test_names[_test_count] = #suite "/" #name; \
            _test_count++; \
        } \
    } \
    static void _test_##suite##_##name(void)

/* -------------------------------------------------------------------------
 * RUN_ALL_TESTS — call from main()
 * ------------------------------------------------------------------------- */
static int run_all_tests(void)
{
    printf("Running %d test(s)...\n", _test_count);
    for (int i = 0; i < _test_count; i++) {
        printf("  [ RUN ] %s\n", _test_names[i]);
        _tests_run++;
        int failed_before = _tests_failed;
        _test_fns[i]();
        if (_tests_failed == failed_before) {
            printf("  [  OK ] %s\n", _test_names[i]);
            _tests_passed++;
        }
    }
    printf("\n%d/%d tests passed", _tests_passed, _tests_run);
    if (_tests_failed) {
        printf("  (%d FAILED)", _tests_failed);
    }
    printf("\n");
    return _tests_failed ? 1 : 0;
}

#define RUN_ALL_TESTS() \
    int main(void) { return run_all_tests(); }
