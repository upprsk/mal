#pragma once

#include <stdbool.h>

#include "common.h"
#include "tgc.h"

// fwd
typedef struct mal_value_string  mal_value_string_t;
typedef struct mal_value_list    mal_value_list_t;
typedef struct mal_value_hashmap mal_value_hashmap_t;

// =============================================================================

/// This is the tag we use to identify which of the field of our union is in
/// use. Some values do not map to any value in the union (like `nil`, `true`
/// and `false`).
typedef enum __attribute__((packed)) mal_value_tag {
    MAL_ERR,      // no storage
    MAL_NIL,      // no storage
    MAL_TRUE,     // no storage
    MAL_FALSE,    // no storage
    MAL_NUM,      // uses `num`
    MAL_KEYWORD,  // uses `string`
    MAL_SYMBOL,   // uses `string`
    MAL_STRING,   // uses `string`
    MAL_VEC,      // uses `list`
    MAL_LIST,     // uses `list`
    MAL_HASMAP,   // uses `hashmap`
} mal_value_tag_t;

/// This is our value struct. It is just a 2 word (16 bytes on 64bit) containing
/// our tag and the value.
typedef struct mal_value {
    mal_value_tag_t tag;
    union {
        double               num;
        mal_value_string_t*  string;
        mal_value_list_t*    list;
        mal_value_hashmap_t* hashmap;
    } as;
} mal_value_t;

static inline bool is_valid_hashmap_key(mal_value_t value) {
    return value.tag == MAL_KEYWORD || value.tag == MAL_SYMBOL ||
           value.tag == MAL_STRING;
}

// =============================================================================

struct mal_value_string {
    size_t size;
    size_t hash;
    char   chars[];
};

mal_value_string_t* mal_string_new(char const* chars, size_t size);

mal_value_string_t* mal_string_new_from_cstr(char const* chars);

/// Given a string, process escape codes into their values.
string_t mal_string_escape_direct(mal_value_string_t* s);

/// Given a string, process escapable characters into their escaped values.
string_t mal_string_unescape_direct(mal_value_string_t* s);

/// Given a string, process escape codes into their values.
mal_value_string_t* mal_string_escape(mal_value_string_t* s);

/// Given a string, process escapable characters into their escaped values.
mal_value_string_t* mal_string_unescape(mal_value_string_t* s);

// =============================================================================

/// A singly linked list.
struct mal_value_list {
    mal_value_t            value;
    struct mal_value_list* next;
};

/// Prepend an item to the start of the list. Returns the new list with the
/// added element.
mal_value_list_t* list_prepend(mal_value_list_t* l, mal_value_t value);

/// Append an item to the end of the list. This method modifies the given list.
mal_value_list_t* list_append(mal_value_list_t* l, mal_value_t value);

/// Get the last element of the list.
mal_value_list_t* list_end(mal_value_list_t* l);

// =============================================================================

#define HASHMAP_MAX_LOAD 0.75

typedef struct mal_hashmap_entry {
    mal_value_t key;
    mal_value_t value;
} mal_hashmap_entry_t;

struct mal_value_hashmap {
    mal_hashmap_entry_t* entries;
    size_t               size;
    size_t               capacity;
};

bool mal_hashmap_put(mal_value_hashmap_t* hm, mal_value_t key,
                     mal_value_t value);
bool mal_hashmap_get(mal_value_hashmap_t const* hm, mal_value_t key,
                     mal_value_t* value);

// =============================================================================

/// hehe. This is our global garbage collector allocator.
extern tgc_t gc;
