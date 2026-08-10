#ifndef TEST_DEPENDENCIES_H
#define TEST_DEPENDENCIES_H

#include "dependencies.h"

typedef struct TestFail TestFail;
typedef struct TestEnv  TestEnv ;

struct TestFail
{
    const char* test_name;
    const char* file     ;
    int line;
    const char* msg      ;
};

struct TestEnv
{
    const char* test_name;
    DYNAMIC_ARRAY(TestFail*) failures;
};

void test_fail_print (FILE* file, TestFail fail);

void test_env_init(TestEnv* test_env);
void test_env_free(TestEnv* test_env);

void test_env_set_test_name(TestEnv* test_env, const char* test_name); 
int  test_env_get_number_of_fails(TestEnv* test_env);

void test_env_print_msgs(FILE* file, TestEnv* test_env);
void test_env_push_fail (TestEnv* test_env, TestFail fail);

#define TEST(test_name) void (test_name)(TestEnv* test_env)
#define TEST_RUN(test_name) \
{ \
    test_env_set_test_name(&test_env, #test_name); \
    test_name(&test_env); \
    test_env_set_test_name(&test_env, NULL); \
}

#define TEST_FAILURE(message) test_env_push_fail(test_env, \
    (TestFail) { \
        .test_name = test_env->test_name, \
        .file = __FILE__, \
        .line = __LINE__, \
        .msg  = message , \
    })

#endif
