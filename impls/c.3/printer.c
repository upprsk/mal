#include "printer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "da.h"
#include "tgc.h"
#include "types.h"

/// Dynamic array for tokens
typedef struct vector_str {
    string_t* items;
    size_t    size;
    size_t    capacity;
} vector_str_t;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static string_t pr_str_sequence(mal_value_t value, char open, char close,
                                bool print_readably) {
    string_t str = {0};

    da_append(&str, open);

    size_t i = 0;
    for (mal_value_list_t* l = value.as.list; l != NULL; l = l->next, i++) {
        if (i != 0) {
            da_append(&str, ' ');
        }

        string_t s = pr_str(l->value, print_readably);
        for (size_t i = 0; i < s.size; i++) {
            da_append(&str, s.items[i]);
        }
    }

    da_append(&str, close);

    return str;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static string_t pr_str_hashmap(mal_value_t value, bool print_readably) {
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

        string_t s = pr_str(e->key, print_readably);
        for (size_t i = 0; i < s.size; i++) {
            da_append(&str, s.items[i]);
        }

        da_append(&str, ' ');

        s = pr_str(e->value, print_readably);
        for (size_t i = 0; i < s.size; i++) {
            da_append(&str, s.items[i]);
        }
    }

    da_append(&str, '}');

    return str;
}

string_t pr_str(mal_value_t value, bool print_readably) {
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
        case MAL_BUILTIN: return string_init_with_cstr("builtin");
        case MAL_NIL: return string_init_with_cstr("nil");
        case MAL_TRUE: return string_init_with_cstr("true");
        case MAL_FALSE: return string_init_with_cstr("false");
        case MAL_NUM: {
            int   c = snprintf(NULL, 0, "%g", value.as.num);
            char* str = tgc_calloc(&gc, c + 1, sizeof(char));
            snprintf(str, c + 1, "%g", value.as.num);

            return string_init_with(str, c);
        } break;
        case MAL_KEYWORD:
        case MAL_SYMBOL:
            return string_copy_with(value.as.string->chars,
                                    value.as.string->size);
        case MAL_STRING:
            if (print_readably) {
                return mal_string_unescape_direct(value.as.string);
            }

            return string_copy_with(value.as.string->chars,
                                    value.as.string->size);
        case MAL_VEC: return pr_str_sequence(value, '[', ']', print_readably);
        case MAL_LIST: return pr_str_sequence(value, '(', ')', print_readably);
        case MAL_HASHMAP: return pr_str_hashmap(value, print_readably);
        case MAL_ERR: return string_init_with_cstr("ERROR"); break;
    }

    return (string_t){0};
}
