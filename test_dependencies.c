#include "test_dependencies.h"

void test_env_init(TestEnv* test_env)
{
    *test_env = (TestEnv)
    {
        .test_name = NULL,
        .failures  = NULL,
    };
}

void test_env_free(TestEnv* test_env)
{
    if (test_env->failures == NULL)
    {
        arrfree(test_env->failures);
    }

    *test_env = (TestEnv)
    {
        .test_name = NULL,
        .failures  = NULL,
    };
}

void test_env_set_test_name(TestEnv* test_env, const char* test_name)
{
    test_env->test_name = test_name;
}

int  test_env_get_number_of_fails(TestEnv* test_env)
{
    return arrlen(test_env->failures);
}

void test_fail_print(FILE* file, TestFail fail)
{
    fprintf(file, "[%s:%d] %s: %s\n", fail.file, fail.line, fail.test_name, fail.msg);
}

void test_env_print_msgs(FILE* file, TestEnv* test_env)
{
    int length = arrlen(test_env->failures);
    for (int i = 0; i < length; ++i)
    {
        test_fail_print(file, test_env->failures[i]);
    }
}

void test_env_push_fail(TestEnv* test_env, TestFail fail)
{
    arrput(test_env->failures, fail);
}
