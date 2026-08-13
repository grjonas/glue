#ifndef DEPENDENCIES_H
#define DEPENDENCIES_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "stb_ds.h"
#include "arena.h"

#define append_list_to_list(ls_a, ls_b) \
    do\
    {\
        size_t rhs_len = arrlen(ls_b);\
        for (size_t i = 0; i < rhs_len; ++i)\
            arrput(ls_a, (ls_b)[i]);\
    }\
    while(0)

#define UNREACHABLE do { assert(false && "This code should be unreachable"); } while(false)
#define DYNAMIC_ARRAY(x) x
#define HASHMAP(x) x
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

char* find_string_in_string_list(DYNAMIC_ARRAY(char** strs), char* str);
bool are_strs_equal(const char* str_a, int length_a, const char* str_b, int length_b);

// Logging
// Not all of these are used
typedef enum
{
    LOG_DEBUG,
    LOG_INFO ,
    LOG_WARN ,
    LOG_ERROR,
    LOG_FATAL,
}
LogKind;

const char* log_kind_show(LogKind kind);
#define LOG(kind, ...) \
	do \
	{ \
    	fprintf(stderr, "[%s:%s:%d]: ", log_kind_show(kind), __FILE__, __LINE__); \
    	fprintf(stderr, __VA_ARGS__ ); \
    	fprintf(stderr, "\n"); \
	} \
	while (false);

#endif
