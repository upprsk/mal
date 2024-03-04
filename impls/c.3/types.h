#pragma once

#include <stdbool.h>

#include "common.h"

// fwd
typedef struct mal_value         mal_value_t;
typedef struct mal_value_builtin mal_value_builtin_t;
typedef struct mal_value_string  mal_value_string_t;
typedef struct mal_value_list    mal_value_list_t;
typedef struct mal_value_hashmap mal_value_hashmap_t;
typedef struct mal_value_fn      mal_value_fn_t;
typedef struct mal_value_atom    mal_value_atom_t;

typedef struct env env_t;

typedef mal_value_t (*mal_builtin_fn)(env_t* env, mal_value_t args);

struct mal_value_builtin {
    mal_builtin_fn impl;
};

// =============================================================================

/// This is the tag we use to identify which of the field of our union is in
/// use. Some values do not map to any value in the union (like `nil`, `true`
/// and `false`).
typedef enum __attribute__((packed)) mal_value_tag {
    MAL_ERR,      // no storage
    MAL_BUILTIN,  // uses `builtin`
    MAL_NIL,      // no storage
    MAL_TRUE,     // no storage
    MAL_FALSE,    // no storage
    MAL_NUM,      // uses `num`
    MAL_KEYWORD,  // uses `string`
    MAL_SYMBOL,   // uses `string`
    MAL_STRING,   // uses `string`
    MAL_VEC,      // uses `list`
    MAL_LIST,     // uses `list`
    MAL_HASHMAP,  // uses `hashmap`
    MAL_ENV,      // IS NOT A `mal_value`! But used by GC
    MAL_FN,       // uses `fn`
    MAL_ATOM,     // uses `atom`
} mal_value_tag_t;

/// This is our value struct. It is just a 2 word (16 bytes on 64bit) containing
/// our tag and the value.
struct mal_value {
    mal_value_tag_t tag;
    union {
        double               num;
        mal_value_builtin_t  builtin;
        mal_value_string_t*  string;
        mal_value_list_t*    list;
        mal_value_hashmap_t* hashmap;
        mal_value_fn_t*      fn;
        mal_value_atom_t*    atom;
    } as;
};

static inline bool is_valid_sequence(mal_value_t value) {
    return value.tag == MAL_LIST || value.tag == MAL_VEC;
}

static inline bool is_valid_hashmap_key(mal_value_t value) {
    return value.tag == MAL_KEYWORD || value.tag == MAL_SYMBOL ||
           value.tag == MAL_STRING;
}

// =============================================================================

typedef enum __attribute__((packed)) gc_mark {
    GC_MARK_NONE,
    GC_MARK_REACHED,
} gc_mark_t;

typedef struct gc_obj {
    struct gc_obj*  next;
    gc_mark_t       mark;
    mal_value_tag_t tag;
} gc_obj_t;

#define gc_alloc(...) malloc(__VA_ARGS__)
#define gc_free(...)  free(__VA_ARGS__)

/// Initialize the garbage collector.
void gc_init();

/// Free all of the garbage collected objects.
void gc_deinit();

/// Add an object to the garbage collector.
void gc_add_obj(mal_value_tag_t tag, gc_obj_t* obj);

// =============================================================================

struct mal_value_string {
    gc_obj_t obj;

    size_t size;
    size_t hash;
    char   chars[];
};

bool mal_string_equal_cstr(mal_value_string_t* s, char const* cstr);

static inline bool mal_value_equal_cstr(mal_value_t v, char const* cstr) {
    return (v.tag == MAL_STRING || v.tag == MAL_SYMBOL) &&
           mal_string_equal_cstr(v.as.string, cstr);
}

mal_value_string_t* mal_string_new(char const* chars, size_t size);

mal_value_string_t* mal_string_new_sized(size_t size);

mal_value_string_t* mal_string_new_from_cstr(char const* chars);

/// Given a string, process escape codes into their values.
mal_value_string_t* mal_string_escape(mal_value_string_t* s);

/// Given a string, process escapable characters into their escaped values.
mal_value_string_t* mal_string_unescape(mal_value_string_t* s);

// =============================================================================

/// A singly linked list.
struct mal_value_list {
    gc_obj_t obj;

    struct mal_value_list* next;
    mal_value_t            value;
};

mal_value_list_t* list_new(mal_value_t value, mal_value_list_t* next);

/// Prepend an item to the start of the list. Returns the new list with the
/// added element.
mal_value_list_t* list_prepend(mal_value_list_t* l, mal_value_t value);

/// Append an item to the end of the list. This method modifies the given list.
mal_value_list_t* list_append(mal_value_list_t* l, mal_value_t value);

/// Get the last element of the list.
mal_value_list_t* list_end(mal_value_list_t* l);

mal_value_list_t* list_reverse(mal_value_list_t* l);

typedef struct mal_value_list_da {
    mal_value_t* items;
    size_t       size;
    size_t       capacity;
} mal_value_list_da_t;

void list_to_da(mal_value_list_t* list, mal_value_list_da_t* out);

// =============================================================================

#define HASHMAP_MAX_LOAD 0.75

typedef struct mal_hashmap_entry {
    mal_value_t key;
    mal_value_t value;
} mal_hashmap_entry_t;

struct mal_value_hashmap {
    gc_obj_t obj;

    size_t               size;
    size_t               capacity;
    mal_hashmap_entry_t* entries;
};

mal_value_hashmap_t* mal_hashmap_new();

// TODO: Add ways to remove from the hasmap map

bool mal_hashmap_put(mal_value_hashmap_t* hm, mal_value_t key,
                     mal_value_t value);
bool mal_hashmap_get(mal_value_hashmap_t const* hm, mal_value_t key,
                     mal_value_t* value);

// =============================================================================

struct mal_value_fn {
    gc_obj_t obj;

    bool is_variadic;

    mal_value_list_da_t binds;
    mal_value_t         body;
    env_t*              outer_env;
};

mal_value_fn_t* mal_fn_new(bool is_variadic, mal_value_list_da_t binds,
                           mal_value_t body, env_t* outer_env);
// =============================================================================

struct mal_value_atom {
    gc_obj_t    obj;
    mal_value_t value;
};

mal_value_atom_t* mal_atom_new(mal_value_t value);

// =============================================================================

typedef struct mal_eval_result {
    mal_value_t value;
    env_t*      env;
} mal_eval_result_t;
