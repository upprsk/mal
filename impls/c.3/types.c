#include "types.h"

#include <assert.h>
#include <string.h>

#include "common.h"
#include "da.h"
#include "tgc.h"

mal_value_string_t* mal_string_new(char const* chars, size_t size) {
    mal_value_string_t* mstr =
        tgc_calloc(&gc, sizeof(mal_value_string_t) + size, sizeof(char));

    *mstr = (mal_value_string_t){.size = size};
    memcpy(mstr->chars, chars, size);

    return mstr;
}

mal_value_string_t* mal_string_new_from_cstr(char const* chars) {
    return mal_string_new(chars, strlen(chars));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
string_t mal_string_escape_direct(mal_value_string_t* s) {
    string_t out = {0};

    for (size_t i = 0; i < s->size; i++) {
        if (s->chars[i] == '\\') {
            i++;

            switch (s->chars[i]) {
                case 'n': da_append(&out, '\n'); break;
                case 't': da_append(&out, '\t'); break;
                case 'r': da_append(&out, '\r'); break;
                case '"': da_append(&out, '"'); break;
                default: da_append(&out, s->chars[i]); break;
            }
        } else {
            da_append(&out, s->chars[i]);
        }
    }

    return out;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
string_t mal_string_unescape_direct(mal_value_string_t* s) {
    string_t out = {0};
    da_append(&out, '"');

    for (size_t i = 0; i < s->size; i++) {
        switch (s->chars[i]) {
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
            default: da_append(&out, s->chars[i]);
        }
    }

    da_append(&out, '"');

    return out;
}

mal_value_string_t* mal_string_escape(mal_value_string_t* s) {
    string_t out = mal_string_escape_direct(s);

    mal_value_string_t* mstr =
        tgc_calloc(&gc, sizeof(mal_value_string_t) + out.size, sizeof(char));

    *mstr = (mal_value_string_t){.size = out.size};
    memcpy(mstr->chars, out.items, out.size);

    da_free(&out);

    return mstr;
}

mal_value_string_t* mal_string_unescape(mal_value_string_t* s) {
    string_t out = mal_string_unescape_direct(s);

    mal_value_string_t* mstr =
        tgc_calloc(&gc, sizeof(mal_value_string_t) + out.size, sizeof(char));

    *mstr = (mal_value_string_t){.size = out.size};
    memcpy(mstr->chars, out.items, out.size);

    da_free(&out);

    return mstr;
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
