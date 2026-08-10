#include "test.h"

Scanner test_scanner_init_with_str(const char* str)
{
    char* copied_str = NULL;

    int length = strlen(str) + 1;
    copied_str = malloc(length * sizeof(char));
    assert(copied_str != NULL);
    memset(copied_str, 0, length);

    strcpy(copied_str, str);

    Scanner scanner =
    {
        .filename   = NULL,
        .init       = (char*) copied_str ,
        .current    = copied_str ,
        .token_list = NULL,
        .diagnostic_component = diagnostic_component_init(),
    }; 

    return scanner;
}


TEST(scanner_peek_char___is_not_at_end___returns_true)
{
    Scanner scanner = test_scanner_init_with_str("l");

    if (!scanner_peek_char(&scanner, NULL))
    {
        TEST_FAILURE("No tokens left");
    }
    free_scanner(&scanner);
}

TEST(scanner_peek_char___is_at_end___returns_false)
{
    Scanner scanner = test_scanner_init_with_str("");

    if (scanner_peek_char(&scanner, NULL))
    {
        TEST_FAILURE("There are tokens left");
    }
    free_scanner(&scanner);
}

TEST(scanner_consume_char___is_not_at_end___doesnt_throw_error)
{
    Scanner scanner = test_scanner_init_with_str("l");

    if (!scanner_consume_char(&scanner, NULL))
    {
        TEST_FAILURE("No tokens to scan");
    }
    else
    {
        if (diagnostic_component_get_msg_num
                (scanner.diagnostic_component) != 0)
        {
            TEST_FAILURE("Function threw error");
        }
    }

    free_scanner(&scanner);
}

TEST(scanner_consume_char___is_at_end___throws_error)
{
    Scanner scanner = test_scanner_init_with_str("");

    if (scanner_consume_char(&scanner, NULL))
    {
        TEST_FAILURE("There are tokens left");
    }
    else
    {
        if (diagnostic_component_get_msg_num
                (scanner.diagnostic_component) == 0)
        {
            TEST_FAILURE("Function didn't throw error");
        }
    }

    free_scanner(&scanner);
}

TEST(scanner_scan_token___text_double___scans_identifier_double)
{
    Scanner scanner = test_scanner_init_with_str("double");

    Token token;

    if (scanner_scan_token(&scanner, &token) != SCANNER_RESULT_SUCCESS)
    {
        TEST_FAILURE("Failed to scan token");
    }
    else if (token.type == TOKEN_DO)
    {
        TEST_FAILURE("Scanned the string \"double\" as 'TOKEN_DO'");
    }
    else if (token.type != TOKEN_IDENTIFIER)
    {
        TEST_FAILURE("Did not scan string as either 'TOKEN_IDENTIFIER' or 'TOKEN_DO'");
    }
    else
    {
        if (!are_strs_equal(token.start, token.length, "double", 6))
        {
            TEST_FAILURE("Scanned TOKEN_IDENTIFIER identifier incorrectly");
        }
    }

    free_scanner(&scanner);
}

TEST(scanner_scan_token___text_let___scans_token_let)
{
    Scanner scanner = test_scanner_init_with_str("let");

    Token token;

    if (scanner_scan_token(&scanner, &token) != SCANNER_RESULT_SUCCESS)
    {
        TEST_FAILURE("Failed to scan token");
    }
    else if (token.type == TOKEN_IDENTIFIER)
    {
        TEST_FAILURE("Scanned keyword 'let' as an identifier");
    }
    else if (token.type != TOKEN_LET)
    {
        TEST_FAILURE("Scanned keyword 'let' as neither identifier nor let");
    }

    free_scanner(&scanner);
}

void scanner_tests()
{
    TestEnv test_env;
    test_env_init(&test_env);

    printf("Scanner test results:\n");

    TEST_RUN(scanner_peek_char___is_not_at_end___returns_true          );
    TEST_RUN(scanner_peek_char___is_at_end___returns_false             );
    TEST_RUN(scanner_consume_char___is_not_at_end___doesnt_throw_error );
    TEST_RUN(scanner_consume_char___is_at_end___throws_error           );
    TEST_RUN(scanner_scan_token___text_double___scans_identifier_double);
    TEST_RUN(scanner_scan_token___text_let___scans_token_let           );

    test_env_print_msgs(stdout, &test_env);
}

TEST(inferer_unify___assign_var_to_var___vars_share_type)
{
}

TEST(inferer_unify___assign_int_to_var___var_type_is_int)
{
}

TEST(inferer_unify___assign_int_to_var_then_var_to_var___all_var_types_are_int)
{
}

TEST(inferer_unify___assign_struct_field_to_var___var_type_is_field_type)
{
}

TEST(inferer_unify___assign_id_fn_result_to_var___var_type_equal_to_fn_arg_type)
{
}

TEST(inferer_unify___assign_type_identical_to_alias_to_var_of_type_alias___no_unification_err)
{
}

void inferer_tests()
{
    TestEnv test_env;
    test_env_init(&test_env);

    printf("Inferer test results:\n");

    ;

    test_env_print_msgs(stdout, &test_env);
}
