#include "common.h"

#include <string.h>

#include "types.h"

uint32_t fnv_1a_hash(char const* str, size_t length) {
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619;
    }
    return hash;
}

string_t string_init_with(char* str, size_t length) {
    return (string_t){
        .items = str, .size = length, .capacity = length,
        // .hash = fnv_1a_hash(str, length),
    };
}

// string_t string_copy_with(char* str, size_t length) {
//     char* s = tgc_calloc(&gc, length, sizeof(char));
//     memcpy(s, str, length);
//
//     return string_init_with(s, length);
// }

string_t string_init_with_cstr(char* str) {
    size_t length = strlen(str);

    return (string_t){
        .items = str, .size = length, .capacity = length,
        // .hash = fnv_1a_hash(str, length),
    };
}
