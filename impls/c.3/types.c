#include "types.h"

#include <assert.h>

#include "common.h"
#include "da.h"
#include "tgc.h"

string_t string_escape(string_t s) {
    string_t out = {0};

    for (size_t i = 0; i < s.size; i++) {
        if (s.items[i] == '\\') {
            i++;

            switch (s.items[i]) {
                case 'n': da_append(&out, '\n'); break;
                case 't': da_append(&out, '\t'); break;
                case 'r': da_append(&out, '\r'); break;
                case '"': da_append(&out, '"'); break;
                default: da_append(&out, s.items[i]); break;
            }
        } else {
            da_append(&out, s.items[i]);
        }
    }

    return out;
}

string_t string_unescape(string_t s) {
    string_t out = {0};
    da_append(&out, '"');

    for (size_t i = 0; i < s.size; i++) {
        switch (s.items[i]) {
            case '\n':
                da_append(&out, '\\');
                da_append(&out, 'n');
                break;
            case '\t':
                da_append(&out, '\\');
                da_append(&out, 't');
                break;
            case '\r':
                da_append(&out, '\\');
                da_append(&out, 'r');
                break;
            case '"':
                da_append(&out, '\\');
                da_append(&out, '"');
                break;
            case '\\':
                da_append(&out, '\\');
                da_append(&out, '\\');
                break;
            default: da_append(&out, s.items[i]);
        }
    }

    da_append(&out, '"');

    return out;
}

// =============================================================================

mal_value_list_t* list_prepend(mal_value_list_t* l, mal_value_t value) {
    mal_value_list_t* new_node = tgc_alloc(&gc, sizeof(mal_value_list_t));

    // Put the value in, and make the rest of the list the next element.
    *new_node = (mal_value_list_t){.value = value, .next = l};

    return new_node;
}

mal_value_list_t* list_append(mal_value_list_t* l, mal_value_t value) {
    mal_value_list_t* end = list_end(l);

    mal_value_list_t* new_node = tgc_alloc(&gc, sizeof(mal_value_list_t));
    *new_node = (mal_value_list_t){.value = value};

    if (end == NULL) {
        l = new_node;
    } else {
        end->next = new_node;
    }

    return l;
}

mal_value_list_t* list_end(mal_value_list_t* l) {
    if (l == NULL) return NULL;
    for (; l->next != NULL; l = l->next) {
    }

    return l;
}

tgc_t gc;
