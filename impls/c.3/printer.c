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

string_t pr_str(mal_value_t value) {
    switch (value.tag) {
        case MAL_NIL:
            return (string_t){.items = "nil", .size = 3, .capacity = 3};
        case MAL_NUM: {
            int   c = snprintf(NULL, 0, "%g", value.as.num);
            char* str = tgc_calloc(&gc, c + 1, sizeof(char));
            snprintf(str, c + 1, "%g", value.as.num);

            return (string_t){.items = str, .size = c, .capacity = c};
        } break;
        case MAL_SYMBOL: {
            return value.as.symbol;
        } break;
        case MAL_LIST: {
            string_t str = {0};

            da_append(&str, '(');

            size_t i = 0;
            for (mal_value_list_t* l = value.as.list; l != NULL;
                 l = l->next, i++) {
                if (i != 0) {
                    da_append(&str, ' ');
                }

                string_t s = pr_str(l->value);
                for (size_t i = 0; i < s.size; i++) {
                    da_append(&str, s.items[i]);
                }
            }

            da_append(&str, ')');

            return str;
        } break;
    }

    return (string_t){0};
}
