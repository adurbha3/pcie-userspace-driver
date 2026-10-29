#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>

static int tf_tests_run = 0;
static int tf_tests_failed = 0;

#define TEST_ASSERT(cond)                                                    \
    do {                                                                     \
        tf_tests_run++;                                                      \
        if (!(cond)) {                                                       \
            tf_tests_failed++;                                               \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_EQ(actual, expected)                                     \
    do {                                                                     \
        tf_tests_run++;                                                      \
        long _a = (long)(actual);                                            \
        long _e = (long)(expected);                                          \
        if (_a != _e) {                                                      \
            tf_tests_failed++;                                               \
            printf("FAIL %s:%d: %s == %s (got %ld, expected %ld)\n",         \
                   __FILE__, __LINE__, #actual, #expected, _a, _e);           \
        }                                                                     \
    } while (0)

#define TEST_RUN(fn)                                                         \
    do {                                                                     \
        printf("-- %s\n", #fn);                                              \
        fn();                                                                \
    } while (0)

#define TEST_SUMMARY_AND_RETURN()                                            \
    do {                                                                     \
        printf("%d/%d assertions passed\n", tf_tests_run - tf_tests_failed,  \
               tf_tests_run);                                                \
        return tf_tests_failed == 0 ? 0 : 1;                                 \
    } while (0)

#endif /* TEST_FRAMEWORK_H */
