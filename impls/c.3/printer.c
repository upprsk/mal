#include "printer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "da.h"
#include "types.h"

/// Dynamic array for tokens
typedef struct vector_str {
    string_t* items;
    size_t    size;
    size_t    capacity;
} vector_str_t;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static mal_value_string_t* pr_str_sequence(mal_value_t value, char open,
                                           char close, bool print_readably) {
    string_t str = {0};

    da_append(&str, open);

    size_t i = 0;
    for (mal_value_list_t* l = value.as.list; l != NULL; l = l->next, i++) {
        if (i != 0) {
            da_append(&str, ' ');
        }

        mal_value_string_t* s = pr_str(l->value, print_readably);
        for (size_t i = 0; i < s->size; i++) {
            da_append(&str, s->chars[i]);
        }
    }

    da_append(&str, close);

    mal_value_string_t* s = mal_string_new(str.items, str.size);
    da_free(&str);

    return s;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static mal_value_string_t* pr_str_hashmap(mal_value_t value,
                                          bool        print_readably) {
    mal_value_hashmap_t* hm = value.as.hashmap;
    string_t             str = {0};

    da_append(&str, '{');

    size_t actual_i = 0;
    for (size_t i = 0; i < hm->capacity; i++) {
        mal_hashmap_entry_t* e = &hm->entries[i];

        if (e->key.as.string == NULL) continue;
        if (actual_i != 0) {
            da_append(&str, ' ');
        }

        actual_i++;

        mal_value_string_t* s = pr_str(e->key, print_readably);
        for (size_t i = 0; i < s->size; i++) {
            da_append(&str, s->chars[i]);
        }

        da_append(&str, ' ');

        s = pr_str(e->value, print_readably);
        for (size_t i = 0; i < s->size; i++) {
            da_append(&str, s->chars[i]);
        }
    }

    da_append(&str, '}');

    mal_value_string_t* s = mal_string_new(str.items, str.size);
    da_free(&str);

    return s;
}

mal_value_string_t* pr_str(mal_value_t value, bool print_readably) {
// #define DEBUG_PRINT_TYPES
#ifdef DEBUG_PRINT_TYPES
    switch (value.tag) {
        case MAL_ERR: fprintf(stderr, "<ERR> "); break;
        case MAL_NIL: fprintf(stderr, "<NIL> "); break;
        case MAL_TRUE: fprintf(stderr, "<TRUE> "); break;
        case MAL_FALSE: fprintf(stderr, "<FALSE> "); break;
        case MAL_NUM: fprintf(stderr, "<NUM> "); break;
        case MAL_KEYWORD: fprintf(stderr, "<KEYWORD> "); break;
        case MAL_SYMBOL: fprintf(stderr, "<SYMBOL> "); break;
        case MAL_STRING: fprintf(stderr, "<STRING> "); break;
        case MAL_VEC: fprintf(stderr, "<VEC> "); break;
        case MAL_LIST: fprintf(stderr, "<LIST> "); break;
        case MAL_HASMAP: fprintf(stderr, "<HASHMAP> "); break;
    }
#endif

    switch (value.tag) {
        case MAL_BUILTIN: return mal_string_new_from_cstr("builtin");
        case MAL_NIL: return mal_string_new_from_cstr("nil");
        case MAL_TRUE: return mal_string_new_from_cstr("true");
        case MAL_FALSE: return mal_string_new_from_cstr("false");
        case MAL_NUM: {
            if ((uint32_t)value.as.num == value.as.num) {
                int c = snprintf(NULL, 0, "%u", (uint32_t)value.as.num);
                mal_value_string_t* str = mal_string_new_sized(c);
                snprintf(str->chars, c + 1, "%u", (uint32_t)value.as.num);

                str->hash = fnv_1a_hash(str->chars, str->size);
                return str;
            }

            int                 c = snprintf(NULL, 0, "%g", value.as.num);
            mal_value_string_t* str = mal_string_new_sized(c);
            snprintf(str->chars, c + 1, "%g", value.as.num);

            str->hash = fnv_1a_hash(str->chars, str->size);
            return str;
        } break;
        case MAL_KEYWORD:
        case MAL_SYMBOL: return value.as.string;
        case MAL_STRING:
            if (print_readably) {
                return mal_string_unescape(value.as.string);
            }

            return value.as.string;
        case MAL_VEC: return pr_str_sequence(value, '[', ']', print_readably);
        case MAL_LIST: return pr_str_sequence(value, '(', ')', print_readably);
        case MAL_HASHMAP: return pr_str_hashmap(value, print_readably);
        case MAL_FN: return mal_string_new_from_cstr("#<function>"); break;
        case MAL_ERR: return mal_string_new_from_cstr("ERROR"); break;
        case MAL_ENV: assert(false && "Invalid tag in pr_str: env");
    }

    assert(false && "Invalid tag in pr_str");
}
