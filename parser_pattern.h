#ifndef PARSER_PATTERN_H
#define PARSER_PATTERN_H

#include "parser_definitions.h"

// bool      parser_parse_pattern  (Parser* parser, Pattern** pattern_ref);
bool parser_attempt_parse_pattern(Parser* parser, Span* span_ref, Pattern** pattern_ref);

const char* copy_literal_to_const_c_str(Arena* arena, int length, const char* str);

#endif
