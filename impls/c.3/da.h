#pragma once

#if __has_include("da_config.h")
#include "da_config.h"
#endif

/// Change the assert function, or remove it completally.
#ifndef da_assert
#define da_assert(expr) assert(expr)
#endif  // !da_assert

/// Change the allocation function.
#ifndef da_realloc_fn
#define da_realloc_fn(ptr, size) realloc((ptr), (size))
#endif  // !da_realloc

/// Change the deallocation function.
#ifndef da_free_fn
#define da_free_fn(ptr) free((ptr))
#endif  // !da_realloc

/// Initialize a dynamic array with some already existing buffer. If the buffer
/// was statically allocated, make shure to never append to it.
///
/// Example:
/// ```c
/// string_t s;
/// da_fixed(&s, buf, buflen);
/// ```
#define da_fixed(da, data, length) \
    do {                           \
        (da)->items = data;        \
        (da)->size = length;       \
        (da)->capacity = length;   \
    } while (0)

/// Initialize a dynamic array with some already existing buffer. If the buffer
/// was statically allocated, make shure to never append to it.
///
/// Example:
/// ```c
/// string_t s = da_init_fixed(buf, buflen);
/// ```
#define da_init_fixed(data, length) \
    { .items = (data), .size = (length), .capacity = (length) }

/// Append a value `v` to the dynamic array, but do not reallocate. In case the
/// array is full, nothing is done. View `da_append` for more info.
#define da_append_fixed_impl(da, v)              \
    do {                                         \
        if ((da)->size >= (da)->capacity) break; \
        (da)->items[(da)->size++] = (v);         \
    } while (0);

/// Append a value `v` to the dynamic array. The struct at `da` **must** be a
/// pointer with _at least_ the following fields:
/// - `size`: any integer
/// - `capacity`: any integer
/// - `items`: array.
///
/// In order to use this struct, the struct `va` just needs to have been
/// zero-initialized.
///
/// Example:
/// ```c
/// typedef struct {
///     char*  items;
///     size_t size;
///     size_t capacity;
/// } dynamic_string_t;
///
/// void test(void) {
///     dynamic_string_t ds = {0};
///     da_append(&ds, 'a');
///     da_append(&ds, 'b');
///     da_append(&ds, 'c');
///
///     printf("%.*s\n", ds.size, ds.items); // abc
///     da_free(&ds);
/// }
/// ```
#define da_append_impl(da, v)                                          \
    do {                                                               \
        if ((da)->size >= (da)->capacity) {                            \
            (da)->capacity = (da)->capacity ? (da)->capacity * 2 : 8;  \
            (da)->items = da_realloc_fn(                               \
                (da)->items, (da)->capacity * sizeof((da)->items[0])); \
            da_assert((da)->items != NULL);                            \
        }                                                              \
        (da)->items[(da)->size++] = (v);                               \
    } while (0);

/// Forwards to `da_append_impl`.
///
/// This trick makes so that we can have commas in the value (like a struct
/// literal)
#define da_append(da, ...) da_append_impl(da, (__VA_ARGS__))

/// Forwards to `da_append_fixed_impl`.
///
/// This trick makes so that we can have commas in the value (like a struct
/// literal)
#define da_append_fixed(da, ...) da_append_fixed_impl(da, (__VA_ARGS__))

/// Pop the last item in the dynamic array. In case the array is empty, does
/// nothing.
#define da_pop(da)                        \
    do {                                  \
        if ((da)->size > 0) (da)->size--; \
    } while (0)

/// Clear the dynamic array, setting the size to `0`. Does **not** deallocate.
#define da_clear(da)    \
    do {                \
        (da)->size = 0; \
    } while (0);

/// Free the dynamic array. After this call the fields used are reset back to
/// the initial state.
#define da_free(da)              \
    do {                         \
        da_free_fn((da)->items); \
        (da)->items = NULL;      \
        (da)->size = 0;          \
        (da)->capacity = 0;      \
    } while (0);
