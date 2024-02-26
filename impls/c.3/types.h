#pragma once

#include "common.h"
#include "tgc.h"

// fwd
typedef struct mal_value_list mal_value_list_t;

// =============================================================================

/// This is the tag we use to identify which of the field of our union is in
/// use. Some values do not map to any value in the union (like `nil`).
typedef enum __attribute__((packed)) mal_value_tag {
    MAL_NIL,
    MAL_NUM,
    MAL_SYMBOL,
    MAL_LIST,
} mal_value_tag_t;

/// This is our value struct. It is just a 2 word (16 bytes on 64bit) containing
/// our tag and the value.
typedef struct mal_value {
    mal_value_tag_t tag;
    union {
        double            num;
        string_t          symbol;
        mal_value_list_t* list;
    } as;
} mal_value_t;

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
