#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "da.h"
#include "tgc.h"

mal_value_string_t* mal_string_new(char const* chars, size_t size) {
    mal_value_string_t* mstr =
        tgc_calloc(&gc, sizeof(mal_value_string_t) + size, sizeof(char));

    *mstr =
        (mal_value_string_t){.size = size, .hash = fnv_1a_hash(chars, size)};
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

    mal_value_string_t* mstr = mal_string_new(out.items, out.size);
    da_free(&out);

    return mstr;
}

mal_value_string_t* mal_string_unescape(mal_value_string_t* s) {
    string_t out = mal_string_unescape_direct(s);

    mal_value_string_t* mstr = mal_string_new(out.items, out.size);
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

static mal_hashmap_entry_t* hashmap_find_entry(mal_hashmap_entry_t* entries,
                                               size_t               capacity,
                                               mal_value_string_t*  key) {
    uint32_t index = key->hash % capacity;
    for (;;) {
        mal_hashmap_entry_t* entry = &entries[index];
        mal_value_string_t*  entry_string = entry->value.as.string;
        if (entry_string == NULL) return entry;
        if (entry_string->size == key->size &&
            entry_string->hash == key->hash &&
            memcmp(entry_string->chars, key->chars, key->size) == 0) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void hashmap_adjust_capacity(mal_value_hashmap_t* hm, size_t capacity) {
    mal_hashmap_entry_t* entries =
        tgc_calloc(&gc, capacity, sizeof(mal_hashmap_entry_t));
    for (size_t i = 0; i < capacity; i++) {
        entries->key = (mal_value_t){0};
        entries->value = (mal_value_t){.tag = MAL_NIL};
    }

    for (size_t i = 0; i < hm->capacity; i++) {
        mal_hashmap_entry_t* entry = &hm->entries[i];
        if (entry->key.as.string == NULL) continue;

        mal_hashmap_entry_t* dest =
            hashmap_find_entry(entries, capacity, entry->key.as.string);
        dest->key = entry->key;
        dest->value = entry->value;
    }

    tgc_free(&gc, hm->entries);
    hm->entries = entries;
    hm->capacity = capacity;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool mal_hashmap_put(mal_value_hashmap_t* hm, mal_value_t key,
                     mal_value_t value) {
    assert(is_valid_hashmap_key(key));

    if (hm->size + 1 > (size_t)((double)hm->capacity * HASHMAP_MAX_LOAD)) {
        size_t capacity = hm->capacity == 0 ? 8 : hm->capacity * 2;
        hashmap_adjust_capacity(hm, capacity);
    }

    mal_hashmap_entry_t* entry =
        hashmap_find_entry(hm->entries, hm->capacity, key.as.string);
    bool is_new_key = entry->key.as.string == NULL;
    if (is_new_key) hm->size++;

    entry->key = key;
    entry->value = value;

    return is_new_key;
}

bool mal_hashmap_get(mal_value_hashmap_t const* hm, mal_value_t key,
                     mal_value_t* value) {
    assert(is_valid_hashmap_key(key));

    if (hm->size == 0) return false;

    mal_hashmap_entry_t* entry =
        hashmap_find_entry(hm->entries, hm->capacity, key.as.string);
    if (entry->key.as.string == NULL) return false;

    *value = entry->value;
    return true;
}

tgc_t gc;
