#ifndef DEPENDENCIES_H
#define DEPENDENCIES_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "stb_ds.h" // Dynamic strings and hashmaps.
/*
 int main(void)
 {
   int *array = NULL;
   arrput(array, 2);
   arrput(array, 3);
   arrput(array, 5);
   for (int i=0; i < arrlen(array); ++i)
     printf("%d ", array[i]);
 }

arrput pushes the value onto the end of the dynamic array (like std::vector.push_back), so the above code will print 2 3 5.

Note that these macros write to the array variable if the array has to be resized.
This means that if you pass a dynamic array into a function which increases its length,
you need to return the updated array from the function to its caller.
In other words, the semantics are the same as any realloc()-ed pointer,
and are unlike the semantics of things like std::vector<>, where the core object is stable even if the array elements aren't.

The following functions are defined:

    arrlen - the length of the dynamic array
    arrlenu - the length of the dynamic array as an unsigned type
    arrput - copy an object into the dynamic array
    arrfree - free the entire array
    arraddn - adds n uninitialized values to the dynamic array
    arrsetlen - sets the length of the array (leaves new slots uninitialized)
    arrlast - the last item in the array as an lvalue
    arrins - inserts an item in the middle of the array
    arrdel - deletes an item from the middle of the array, moving the following items over
    arrswapdel - deletes an item from the middle of the array, replacing it with the formerly last item
    arrcap - returns the internal capacity, the maximum length the array can be without reallocating it
    arrsetcap - sets the internal capacity. it is not possible to shrink the capacity (currently) 
*/

#define append_list_to_list(ls_a, ls_b) \
    do\
    {\
        size_t rhs_len = arrlen(ls_b);\
        for (size_t i = 0; i < rhs_len; ++i)\
            arrput(ls_a, (ls_b)[i]);\
    }\
    while(0)

#include "arena.h"

#define DYNAMIC_ARRAY

#endif
