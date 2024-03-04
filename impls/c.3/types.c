#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "da.h"

typedef struct gc {
    gc_obj_t* objs;
} gc_t;

static gc_t gc = {0};

void gc_init() { gc = (gc_t){0}; }

void gc_deinit() {
    for (gc_obj_t* obj = gc.objs; obj != NULL;) {
        gc_obj_t* o = obj;
        obj = obj->next;

        free(o);
    }
}

void gc_add_obj(gc_obj_t* obj) {
    obj->next = gc.objs;
    obj->mark = GC_MARK_NONE;
    gc.objs = obj;
}

// =============================================================================

mal_value_string_t* mal_string_new(char const* chars, size_t size) {
    mal_value_string_t* mstr = gc_alloc(sizeof(mal_value_string_t) + size + 1);

    *mstr = (mal_value_string_t){
        .size = size,
        .hash = fnv_1a_hash(chars, size),
    };
    memcpy(mstr->chars, chars, size);
    mstr->chars[size] = 0;  // null terminate

    gc_add_obj(&mstr->obj);

    return mstr;
}

mal_value_string_t* mal_string_new_sized(size_t size) {
    mal_value_string_t* mstr = gc_alloc(sizeof(mal_value_string_t) + size + 1);

    *mstr = (mal_value_string_t){.size = size};
    mstr->chars[size] = 0;  // null terminate

    gc_add_obj(&mstr->obj);

    return mstr;
}

mal_value_string_t* mal_string_new_from_cstr(char const* chars) {
    return mal_string_new(chars, strlen(chars));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
mal_value_string_t* mal_string_escape(mal_value_string_t* s) {
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

    mal_value_string_t* out_str = mal_string_new(out.items, out.size);
    da_free(&out);

    return out_str;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
mal_value_string_t* mal_string_unescape(mal_value_string_t* s) {
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

    mal_value_string_t* out_str = mal_string_new(out.items, out.size);
    da_free(&out);

    return out_str;
}

// =============================================================================

mal_value_list_t* list_new(mal_value_t value, mal_value_list_t* next) {
    mal_value_list_t* new_node = gc_alloc(sizeof(mal_value_list_t));

    *new_node = (mal_value_list_t){.value = value, .next = next};

    gc_add_obj(&new_node->obj);

    return new_node;
}

mal_value_list_t* list_prepend(mal_value_list_t* l, mal_value_t value) {
    return list_new(value, l);
}

mal_value_list_t* list_append(mal_value_list_t* l, mal_value_t value) {
    mal_value_list_t* end = list_end(l);

    mal_value_list_t* new_node = list_new(value, NULL);

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

void list_to_da(mal_value_list_t* list, mal_value_list_da_t* out) {
    for (mal_value_list_t* l = list; l != NULL; l = l->next) {
        da_append(out, l->value);
    }
}

static mal_hashmap_entry_t* hashmap_find_entry(mal_hashmap_entry_t* entries,
                                               size_t               capacity,
                                               mal_value_string_t*  key) {
    uint32_t index = key->hash % capacity;
    for (;;) {
        mal_hashmap_entry_t* entry = &entries[index];
        mal_value_string_t*  entry_key = entry->key.as.string;
        if (entry_key == NULL) return entry;
        if (entry_key->size == key->size && entry_key->hash == key->hash &&
            memcmp(entry_key->chars, key->chars, key->size) == 0) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void hashmap_adjust_capacity(mal_value_hashmap_t* hm, size_t capacity) {
    mal_hashmap_entry_t* entries =
        calloc(capacity, sizeof(mal_hashmap_entry_t));
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

    free(hm->entries);
    hm->entries = entries;
    hm->capacity = capacity;
}

mal_value_hashmap_t* mal_hashmap_new() {
    mal_value_hashmap_t* hm = gc_alloc(sizeof(mal_value_hashmap_t));
    *hm = (mal_value_hashmap_t){0};

    gc_add_obj(&hm->obj);

    return hm;
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
    if (!is_valid_hashmap_key(key)) return false;

    if (hm->size == 0) return false;

    mal_hashmap_entry_t* entry =
        hashmap_find_entry(hm->entries, hm->capacity, key.as.string);
    if (entry->key.as.string == NULL) return false;

    *value = entry->value;
    return true;
}
