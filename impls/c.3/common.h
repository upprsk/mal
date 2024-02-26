#pragma once

#include <stddef.h>

#define UNUSED __attribute__((unused))

typedef struct string {
    char*  items;
    size_t size;
    size_t capacity;
} string_t;
