#pragma once

#include <stddef.h>
#include <stdint.h>

#define UNUSED __attribute__((unused))

typedef struct string {
    char*  items;
    size_t size;
    size_t capacity;

    /// FNV-1a hash of the string.
    // uint32_t hash;
} string_t;

uint32_t fnv_1a_hash(char const* str, size_t length);

/// Create a new string with the given characters and length. If the `str` was
/// not allocated, then do **NOT** append to it.
string_t string_init_with(char* str, size_t length);

/// Create a new string with the given characters and length. Duplicates the
/// string characters.
// string_t string_copy_with(char* str, size_t length);

string_t string_init_with_cstr(char* str);
