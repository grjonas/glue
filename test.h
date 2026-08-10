#ifndef TEST_H
#define TEST_H

#include "test_dependencies.h"
#include "scanner.h"
#include "inferer.h"

Scanner test_scanner_init_with_str(const char* str);

TEST(scanner_peek_char___is_not_at_end___returns_true          );
TEST(scanner_peek_char___is_at_end___returns_false             );
TEST(scanner_consume_char___is_not_at_end___doesnt_throw_error );
TEST(scanner_consume_char___is_at_end___throws_error           );
TEST(scanner_scan_token___text_double___scans_identifier_double);
TEST(scanner_scan_token___text_let___scans_token_let           );

void scanner_tests();

#endif
