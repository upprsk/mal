#pragma once

#include "common.h"
#include "tgc.h"

// fwd
typedef struct mal_value_list mal_value_list_t;

// =============================================================================

/// This is the tag we use to identify which of the field of our union is in
/// use. Some values do not map to any value in the union (like `nil`, `true`
/// and `false`).
typedef enum __attribute__((packed)) mal_value_tag {
    MAL_NIL,      // no storage
    MAL_TRUE,     // no storage
    MAL_FALSE,    // no storage
    MAL_NUM,      // uses `num`
    MAL_KEYWORD,  // uses `string`
    MAL_SYMBOL,   // uses `string`
    MAL_STRING,   // uses `string`
    MAL_VEC,      // uses `list`
    MAL_LIST,     // uses `list`
} mal_value_tag_t;

/// This is our value struct. It is just a 2 word (16 bytes on 64bit) containing
/// our tag and the value.
typedef struct mal_value {
    mal_value_tag_t tag;
    union {
        double            num;
        string_t          string;
        mal_value_list_t* list;
    } as;
} mal_value_t;

// =============================================================================

/// Given a string, process escape codes into their values.
string_t string_escape(string_t s);

/// Given a string, process escapable characters into their escaped values.
string_t string_unescape(string_t s);

// =============================================================================

/// A singly linked list.
typedef struct mal_value_list {
    mal_value_t            value;
    struct mal_value_list* next;
} mal_value_list_t;

/// Prepend an item to the start of the list. Returns the new list with the
/// added element.
mal_value_list_t* list_prepend(mal_value_list_t* l, mal_value_t value);

/// Append an item to the end of the list. This method modifies the given list.
mal_value_list_t* list_append(mal_value_list_t* l, mal_value_t value);

/// Get the last element of the list.
mal_value_list_t* list_end(mal_value_list_t* l);

// =============================================================================

/// hehe. This is our global garbage collector allocator.
extern tgc_t gc;
