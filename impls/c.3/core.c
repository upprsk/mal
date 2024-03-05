#include "core.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "da.h"
#include "env.h"
#include "printer.h"
#include "reader.h"
#include "types.h"

extern mal_eval_result_t mal_eval_apply(mal_value_t       fn_value,
                                        mal_value_list_t* args, env_t* env);

[[nodiscard]] string_t load_file(char const* path) {
    // open the file and check for errors
    FILE* f = fopen(path, "rbe");
    if (f == NULL) return (string_t){0};

    // get the size of the file
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return (string_t){0};
    }

    long tell_len = ftell(f);

    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return (string_t){0};
    }

    // allocate a buffer large enough load the data into
    char* buf = calloc(tell_len, sizeof(char));
    if (buf == NULL) {
        fclose(f);
        return (string_t){0};
    }

    // read the file into the buffer
    size_t len = fread(buf, 1, tell_len, f);
    assert(len == (size_t)tell_len);  // sanity check

    // can close the file
    fclose(f);

    // wrap in a string and return it
    return string_init_with(buf, len);
}

#define buintin_fn_min_binary(args, op)                                      \
    mal_value_list_da_t da = {0};                                            \
    list_to_da((args).as.list, &da);                                         \
    if (da.size < 3) {                                                       \
        fprintf(stderr,                                                      \
                "ERROR: Invalid size for '" #op                              \
                "', expected sequence of 3, found "                          \
                "%zu\n",                                                     \
                da.size);                                                    \
        da_free(&da);                                                        \
        return (mal_value_t){.tag = MAL_ERR};                                \
    }                                                                        \
    if (da.items[1].tag != MAL_NUM) {                                        \
        fprintf(stderr,                                                      \
                "ERROR: invalid input to '" #op "', expected number\n");     \
        da_free(&da);                                                        \
        return (mal_value_t){.tag = MAL_ERR};                                \
    }                                                                        \
    double acc = da.items[1].as.num;                                         \
    for (size_t i = 2; i < da.size; i++) {                                   \
        if (da.items[i].tag != MAL_NUM) {                                    \
            fprintf(stderr,                                                  \
                    "ERROR: invalid input to '" #op "', expected number\n"); \
            da_free(&da);                                                    \
            return (mal_value_t){.tag = MAL_ERR};                            \
        }                                                                    \
                                                                             \
        acc = acc op da.items[i].as.num;                                     \
    }                                                                        \
                                                                             \
    da_free(&da);                                                            \
    return (mal_value_t) { .tag = MAL_NUM, .as.num = acc }

#define buintin_fn_binary(args, op)                                      \
    mal_value_list_da_t da = {0};                                        \
    list_to_da((args).as.list, &da);                                     \
    if (da.size != 3) {                                                  \
        fprintf(stderr,                                                  \
                "ERROR: Invalid size for '" #op                          \
                "', expected sequence of 3, found "                      \
                "%zu\n",                                                 \
                da.size);                                                \
        return (mal_value_t){.tag = MAL_ERR};                            \
    }                                                                    \
    if (da.items[1].tag != MAL_NUM) {                                    \
        fprintf(stderr,                                                  \
                "ERROR: invalid input to '" #op "', expected number\n"); \
        return (mal_value_t){.tag = MAL_ERR};                            \
    }                                                                    \
    mal_value_t lhs = da.items[1];                                       \
    mal_value_t rhs = da.items[2];                                       \
    da_free(&da);                                                        \
    if (!(lhs.tag == MAL_NUM && rhs.tag == MAL_NUM)) {                   \
        fprintf(stderr,                                                  \
                "ERROR: invalid input to '" #op "', expected number\n"); \
        return (mal_value_t){.tag = MAL_ERR};                            \
    }                                                                    \
    return (mal_value_t) {                                               \
        .tag = lhs.as.num op rhs.as.num ? MAL_TRUE : MAL_FALSE           \
    }

static mal_value_t builtin_fn_add(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_min_binary(args, +);
}

static mal_value_t builtin_fn_sub(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_min_binary(args, -);
}

static mal_value_t builtin_fn_div(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_min_binary(args, /);
}

static mal_value_t builtin_fn_mul(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_min_binary(args, *);
}

static mal_value_t builtin_fn_prn(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stdout, "\n");
        return (mal_value_t){.tag = MAL_NIL};
    }

    string_t out = {0};
    for (mal_value_list_t* l = arg; l != NULL; l = l->next) {
        if (out.size > 0) {
            da_append(&out, ' ');
        }

        mal_value_string_t* v = pr_str(l->value, true);

        for (size_t i = 0; i < v->size; i++) {
            da_append(&out, v->chars[i]);
        }
    }

    fwrite(out.items, sizeof(char), out.size, stdout);
    fprintf(stdout, "\n");

    da_free(&out);

    return (mal_value_t){.tag = MAL_NIL};
}

static mal_value_t builtin_fn_println(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stdout, "\n");
        return (mal_value_t){.tag = MAL_NIL};
    }

    string_t out = {0};
    for (mal_value_list_t* l = arg; l != NULL; l = l->next) {
        if (out.size > 0) {
            da_append(&out, ' ');
        }

        mal_value_string_t* v = pr_str(l->value, false);

        for (size_t i = 0; i < v->size; i++) {
            da_append(&out, v->chars[i]);
        }
    }

    fwrite(out.items, sizeof(char), out.size, stdout);
    fprintf(stdout, "\n");

    da_free(&out);

    return (mal_value_t){.tag = MAL_NIL};
}

static mal_value_t builtin_fn_pr_str(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;

    string_t out = {0};
    for (mal_value_list_t* l = arg; l != NULL; l = l->next) {
        if (out.size > 0) {
            da_append(&out, ' ');
        }

        mal_value_string_t* v = pr_str(l->value, true);

        for (size_t i = 0; i < v->size; i++) {
            da_append(&out, v->chars[i]);
        }
    }

    mal_value_string_t* str = mal_string_new(out.items, out.size);
    da_free(&out);

    return (mal_value_t){.tag = MAL_STRING, .as.string = str};
}

static mal_value_t builtin_fn_str(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;

    string_t out = {0};
    for (mal_value_list_t* l = arg; l != NULL; l = l->next) {
        mal_value_string_t* v = pr_str(l->value, false);

        for (size_t i = 0; i < v->size; i++) {
            da_append(&out, v->chars[i]);
        }
    }

    mal_value_string_t* str = mal_string_new(out.items, out.size);
    da_free(&out);

    return (mal_value_t){.tag = MAL_STRING, .as.string = str};
}

static mal_value_t builtin_fn_vec(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'vec'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (arg->value.tag != MAL_VEC && arg->value.tag != MAL_LIST) {
        fprintf(stderr,
                "ERROR: invalid input to 'vec', expected list or vector\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    return (mal_value_t){.tag = MAL_VEC, .as.list = arg->value.as.list};
}

static mal_value_t builtin_fn_assoc(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'assoc'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t hmv = args.as.list->next->value;

    if (hmv.tag != MAL_HASHMAP) {
        fprintf(stderr, "ERROR: invalid input to 'assoc', expected hash-map\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_hashmap_t* old_hm = hmv.as.hashmap;
    mal_value_hashmap_t* hm = mal_hashmap_new();

    for (size_t i = 0; i < old_hm->capacity; i++) {
        mal_hashmap_entry_t* e = &old_hm->entries[i];

        if (e->key.as.string == NULL) continue;
        mal_hashmap_put(hm, e->key, e->value);
    }

    for (mal_value_list_t* l = args.as.list->next->next; l != NULL;
         l = l->next) {
        mal_value_t k = l->value;
        if (!is_valid_hashmap_key(k)) {
            mal_value_string_t* s = pr_str(k, true);
            fprintf(stderr,
                    "ERROR: can't use given value as key in hash-map: %s\n",
                    s->chars);
            return (mal_value_t){.tag = MAL_ERR};
        }

        l = l->next;
        if (l == NULL) {
            fprintf(stderr, "ERROR: missing value for given hash-map key\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        mal_value_t v = l->value;

        mal_hashmap_put(hm, k, v);
    }

    return (mal_value_t){.tag = MAL_HASHMAP, .as.hashmap = hm};
}

static mal_value_t builtin_fn_dissoc(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'dissoc'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t hmv = args.as.list->next->value;

    if (hmv.tag != MAL_HASHMAP) {
        fprintf(stderr,
                "ERROR: invalid input to 'dissoc', expected hash-map\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_hashmap_t tmp_set = {0};

    for (mal_value_list_t* l = args.as.list->next->next; l != NULL;
         l = l->next) {
        if (!is_valid_hashmap_key(l->value)) {
            fprintf(stderr,
                    "ERROR: invalid input to 'dissoc', expected string\n");

            free(tmp_set.entries);
            return (mal_value_t){.tag = MAL_ERR};
        }

        mal_hashmap_put(&tmp_set, l->value, (mal_value_t){.tag = MAL_NIL});
    }

    mal_value_hashmap_t* old_hm = hmv.as.hashmap;
    mal_value_hashmap_t* hm = mal_hashmap_new();

    for (size_t i = 0; i < old_hm->capacity; i++) {
        mal_hashmap_entry_t* e = &old_hm->entries[i];

        if (e->key.as.string == NULL) continue;

        mal_value_t v = {0};
        if (mal_hashmap_get(&tmp_set, e->key, &v)) continue;

        mal_hashmap_put(hm, e->key, e->value);
    }

    free(tmp_set.entries);

    return (mal_value_t){.tag = MAL_HASHMAP, .as.hashmap = hm};
}

static mal_value_t builtin_fn_get(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'get'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t hmv = args.as.list->next->value;
    if (hmv.tag == MAL_NIL) return (mal_value_t){.tag = MAL_NIL};
    if (hmv.tag != MAL_HASHMAP) {
        fprintf(stderr, "ERROR: invalid input to 'get', expected hash-map\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (args.as.list->next->next == NULL) {
        fprintf(stderr, "ERROR: missing second parameter for 'get': key\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t k = args.as.list->next->next->value;

    if (!is_valid_hashmap_key(k)) {
        return (mal_value_t){.tag = MAL_EXCEPTION,
                             .as.atom = mal_exception_newfmt(
                                 "invalid key for 'get', expected string")};
    }

    mal_value_t v = {0};
    if (!mal_hashmap_get(hmv.as.hashmap, k, &v)) {
        return (mal_value_t){.tag = MAL_NIL};
    }

    return v;
}

static mal_value_t builtin_fn_contains_question(UNUSED env_t* env,
                                                mal_value_t   args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'get'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t hmv = args.as.list->next->value;
    if (hmv.tag == MAL_NIL) return (mal_value_t){.tag = MAL_NIL};
    if (hmv.tag != MAL_HASHMAP) {
        fprintf(stderr, "ERROR: invalid input to 'get', expected hash-map\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (args.as.list->next->next == NULL) {
        fprintf(stderr, "ERROR: missing second parameter for 'get': key\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t k = args.as.list->next->next->value;

    if (!is_valid_hashmap_key(k)) {
        return (mal_value_t){.tag = MAL_EXCEPTION,
                             .as.atom = mal_exception_newfmt(
                                 "invalid key for 'get', expected string")};
    }

    mal_value_t v = {0};
    if (!mal_hashmap_get(hmv.as.hashmap, k, &v)) {
        return (mal_value_t){.tag = MAL_FALSE};
    }

    return (mal_value_t){.tag = MAL_TRUE};
}

static mal_value_t builtin_fn_keys(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'keys'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t hmv = args.as.list->next->value;

    if (hmv.tag != MAL_HASHMAP) {
        fprintf(stderr, "ERROR: invalid input to 'keys', expected hash-map\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_list_t* lst = NULL;

    mal_value_hashmap_t* hm = hmv.as.hashmap;
    for (size_t i = 0; i < hm->capacity; i++) {
        mal_hashmap_entry_t* e = &hm->entries[i];

        if (e->key.as.string == NULL) continue;

        lst = list_prepend(lst, e->key);
    }

    return (mal_value_t){.tag = MAL_LIST, .as.list = list_reverse(lst)};
}

static mal_value_t builtin_fn_vals(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'vals'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t hmv = args.as.list->next->value;

    if (hmv.tag != MAL_HASHMAP) {
        fprintf(stderr, "ERROR: invalid input to 'vals', expected hash-map\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_list_t* lst = NULL;

    mal_value_hashmap_t* hm = hmv.as.hashmap;
    for (size_t i = 0; i < hm->capacity; i++) {
        mal_hashmap_entry_t* e = &hm->entries[i];

        if (e->key.as.string == NULL) continue;

        lst = list_prepend(lst, e->value);
    }

    return (mal_value_t){.tag = MAL_LIST, .as.list = list_reverse(lst)};
}

static mal_value_t builtin_fn_list(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* out = NULL;
    for (mal_value_list_t* l = args.as.list->next; l != NULL; l = l->next) {
        out = list_prepend(out, l->value);
    }

    return (mal_value_t){.tag = MAL_LIST, .as.list = list_reverse(out)};
}

static mal_value_t builtin_fn_vector(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* out = NULL;
    for (mal_value_list_t* l = args.as.list->next; l != NULL; l = l->next) {
        out = list_prepend(out, l->value);
    }

    return (mal_value_t){.tag = MAL_VEC, .as.list = list_reverse(out)};
}

static mal_value_t builtin_fn_symbol(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'symbol'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t arg = args.as.list->next->value;

    if (arg.tag == MAL_KEYWORD) {
        if (arg.as.string->size == 1) {
            fprintf(stderr, "ERROR: Invalid symbol for 'symbol': '%s'\n",
                    arg.as.string->chars);
            return (mal_value_t){.tag = MAL_ERR};
        }

        mal_value_string_t* s = mal_string_new_sized(arg.as.string->size - 1);
        memcpy(s->chars, &arg.as.string->chars[1], s->size);

        return (mal_value_t){.tag = MAL_SYMBOL, .as.string = s};
    }

    if (arg.tag != MAL_STRING && arg.tag != MAL_SYMBOL) {
        fprintf(stderr, "ERROR: invalid input to 'symbol', expected string\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    return (mal_value_t){.tag = MAL_SYMBOL, .as.string = arg.as.string};
}

static mal_value_t builtin_fn_keyword(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'keyword'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t arg = args.as.list->next->value;

    if (arg.tag == MAL_KEYWORD) return arg;
    if (arg.tag == MAL_STRING || arg.tag == MAL_SYMBOL) {
        mal_value_string_t* s = mal_string_new_sized(arg.as.string->size + 1);
        s->chars[0] = ':';
        memcpy(&s->chars[1], arg.as.string->chars, arg.as.string->size);

        return (mal_value_t){.tag = MAL_KEYWORD, .as.string = s};
    }

    fprintf(stderr, "ERROR: invalid input to 'keyword', expected string\n");
    return (mal_value_t){.tag = MAL_ERR};
}

static mal_value_t builtin_fn_hash_map(UNUSED env_t* env, mal_value_t args) {
    mal_value_hashmap_t* hm = mal_hashmap_new();

    for (mal_value_list_t* l = args.as.list->next; l != NULL; l = l->next) {
        mal_value_t k = l->value;
        if (!is_valid_hashmap_key(k)) {
            mal_value_string_t* s = pr_str(k, true);
            fprintf(stderr,
                    "ERROR: can't use given value as key in hash-map: %s\n",
                    s->chars);
            return (mal_value_t){.tag = MAL_ERR};
        }

        l = l->next;
        if (l == NULL) {
            fprintf(stderr, "ERROR: missing value for given hash-map key\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        mal_value_t v = l->value;

        mal_hashmap_put(hm, k, v);
    }

    return (mal_value_t){.tag = MAL_HASHMAP, .as.hashmap = hm};
}

#define builtin_fn_question(NAME, TAG)                                      \
    static mal_value_t builtin_fn_##NAME##_question(UNUSED env_t* env,      \
                                                    mal_value_t   args) {     \
        mal_value_list_t* arg = args.as.list->next;                         \
        if (arg == NULL) {                                                  \
            fprintf(stderr, "ERROR: Missing parameter for '" #NAME "?'\n"); \
            return (mal_value_t){.tag = MAL_ERR};                           \
        }                                                                   \
                                                                            \
        return (mal_value_t){.tag = arg->value.tag == (TAG) ? MAL_TRUE      \
                                                            : MAL_FALSE};   \
    }

builtin_fn_question(list, MAL_LIST);
builtin_fn_question(nil, MAL_NIL);
builtin_fn_question(true, MAL_TRUE);
builtin_fn_question(false, MAL_FALSE);
builtin_fn_question(symbol, MAL_SYMBOL);
builtin_fn_question(keyword, MAL_KEYWORD);
builtin_fn_question(vector, MAL_VEC);
builtin_fn_question(map, MAL_HASHMAP);

static mal_value_t builtin_fn_sequential_question(UNUSED env_t* env,
                                                  mal_value_t   args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr,
                "ERROR: Missing parameter for '"
                "sequential"
                "?'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }
    return (mal_value_t){
        .tag = (arg->value.tag == MAL_LIST || arg->value.tag == MAL_VEC)
                   ? MAL_TRUE
                   : MAL_FALSE};
};

static mal_value_t builtin_fn_empty(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'empty'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (arg->value.tag != MAL_LIST && arg->value.tag != MAL_VEC) {
        fprintf(stderr, "ERROR: invalid input to 'empty', expected list\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    return (mal_value_t){.tag =
                             arg->value.as.list == NULL ? MAL_TRUE : MAL_FALSE};
}

static mal_value_t builtin_fn_count(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'size'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (arg->value.tag == MAL_NIL) {
        return (mal_value_t){.tag = MAL_NUM, .as.num = 0};
    }

    if (arg->value.tag != MAL_LIST && arg->value.tag != MAL_VEC) {
        fprintf(stderr, "ERROR: invalid input to 'size', expected list\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    uint32_t size = 0;
    for (mal_value_list_t* l = arg->value.as.list; l != NULL; l = l->next) {
        size++;
    }

    return (mal_value_t){.tag = MAL_NUM, .as.num = size};
}

static bool mal_values_equals(mal_value_t lhs, mal_value_t rhs) {
    if (lhs.tag == MAL_LIST && rhs.tag == MAL_VEC) rhs.tag = MAL_LIST;
    if (lhs.tag == MAL_VEC && rhs.tag == MAL_LIST) lhs.tag = MAL_LIST;

    if (lhs.tag != rhs.tag) return false;

    switch (lhs.tag) {
        case MAL_ERR: assert(false && "This should never happen");
        case MAL_BUILTIN: return lhs.as.builtin.impl == rhs.as.builtin.impl;
        case MAL_NIL:
        case MAL_TRUE:
        case MAL_FALSE: return true;
        case MAL_NUM: return lhs.as.num == rhs.as.num;
        case MAL_KEYWORD:
        case MAL_SYMBOL:
        case MAL_STRING:
            return lhs.as.string->size == rhs.as.string->size &&
                   lhs.as.string->hash == rhs.as.string->hash &&
                   memcmp(lhs.as.string->chars, rhs.as.string->chars,
                          lhs.as.string->size) == 0;
        case MAL_VEC:
        case MAL_LIST: {
            mal_value_list_da_t lhs_da = {0};
            mal_value_list_da_t rhs_da = {0};

            list_to_da(lhs.as.list, &lhs_da);
            list_to_da(rhs.as.list, &rhs_da);

            if (lhs_da.size != rhs_da.size) {
                da_free(&lhs_da);
                da_free(&rhs_da);

                return false;
            }

            for (size_t i = 0; i < lhs_da.size; i++) {
                if (!mal_values_equals(lhs_da.items[i], rhs_da.items[i])) {
                    return false;
                }
            }

            return true;
        }
        case MAL_HASHMAP: assert(false && "Not implemented");
        case MAL_FN: return lhs.as.fn == rhs.as.fn;
        default: assert(false && "This should not happen");
    }
}

static mal_value_t builtin_fn_equals(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'equals', expected sequence "
                "of 3, found %zu\n",
                da.size);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t lhs = da.items[1];
    mal_value_t rhs = da.items[2];
    da_free(&da);

    bool equal = mal_values_equals(lhs, rhs);
    return (mal_value_t){.tag = equal ? MAL_TRUE : MAL_FALSE};
}

static mal_value_t builtin_fn_lt(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_binary(args, <);
}

static mal_value_t builtin_fn_lte(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_binary(args, <=);
}

static mal_value_t builtin_fn_gt(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_binary(args, >);
}

static mal_value_t builtin_fn_gte(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_binary(args, >=);
}

#undef buintin_fn_min_binary
#undef buintin_fn_binary

static mal_value_t builtin_fn_read_str(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) return (mal_value_t){.tag = MAL_NIL};

    if (arg->value.tag != MAL_STRING) {
        fprintf(stderr,
                "ERROR: Invalid type for 'read-str', expected a string as "
                "parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_string_t* s = arg->value.as.string;
    return read_str(string_init_with(s->chars, s->size));
}

static mal_value_t builtin_fn_slurp(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr,
                "ERROR: Invalid size for 'read-str', expected a 1 parameter, "
                "got 0\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (arg->value.tag != MAL_STRING) {
        fprintf(stderr,
                "ERROR: Invalid type for 'slurp', expected a string as "
                "parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    // TODO: null terminate the mal strings
    mal_value_string_t* s = arg->value.as.string;

    string_t str = load_file(s->chars);
    if (str.items == NULL) {
        fprintf(stderr, "ERROR: Failed to read file %s\n", s->chars);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t v = {.tag = MAL_STRING,
                     .as.string = mal_string_new(str.items, str.size)};

    da_free(&str);
    return v;
}

extern mal_value_t mal_eval(mal_value_t value, env_t* env);

static mal_value_t builtin_fn_eval(env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        return (mal_value_t){.tag = MAL_NIL};
    }

    // get the root environment
    while (env->outer) env = env->outer;

    return mal_eval(arg->value, env);
}

static mal_value_t builtin_fn_atom(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'atom'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_atom_t* atom = mal_atom_new(arg->value);
    return (mal_value_t){.tag = MAL_ATOM, .as.atom = atom};
}

static mal_value_t builtin_fn_atom_question(UNUSED env_t* env,
                                            mal_value_t   args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'atom?'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    return (mal_value_t){.tag =
                             arg->value.tag == MAL_ATOM ? MAL_TRUE : MAL_FALSE};
}

static mal_value_t builtin_fn_deref(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'deref'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (arg->value.tag != MAL_ATOM) {
        fprintf(stderr,
                "ERROR: Invalid type for 'deref', expected an atom as "
                "parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    return arg->value.as.atom->value;
}

static mal_value_t builtin_fn_reset(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing first parameter for 'reset!'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (arg->value.tag != MAL_ATOM) {
        fprintf(stderr,
                "ERROR: Invalid type for 'reset!', expected an atom as first "
                "parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_list_t* new_value = arg->next;
    if (new_value == NULL) {
        fprintf(stderr, "ERROR: Missing second parameter for 'reset!'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    arg->value.as.atom->value = new_value->value;

    return arg->value.as.atom->value;
}

static mal_value_t builtin_fn_swap(env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing first parameter for 'swap!'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (arg->value.tag != MAL_ATOM) {
        fprintf(stderr,
                "ERROR: Invalid type for 'swap!', expected an atom as first "
                "parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_list_t* fn = arg->next;
    if (fn == NULL) {
        fprintf(stderr, "ERROR: Missing second parameter for 'swap!'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (fn->value.tag != MAL_FN && fn->value.tag != MAL_BUILTIN) {
        fprintf(stderr,
                "ERROR: Invalid type for 'swap!', expected a function as "
                "second parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_list_t* fn_args = NULL;
    fn_args = list_append(fn_args, arg->value.as.atom->value);
    fn_args->next = fn->next;

    if (fn->value.tag == MAL_FN) {
        mal_eval_result_t r = mal_eval_apply(fn->value, fn_args, env);
        if (is_error(r.value)) return r.value;

        mal_value_t v = mal_eval(r.value, r.env);
        if (is_error(v)) return v;

        arg->value.as.atom->value = v;
    } else if (fn->value.tag == MAL_BUILTIN) {
        mal_value_t v = fn->value.as.builtin.impl(
            env, (mal_value_t){.tag = MAL_LIST, .as.list = fn_args});
        if (is_error(v)) return v;

        arg->value.as.atom->value = v;
    } else {
        arg->value.as.atom->value = fn->value.as.builtin.impl(
            env, (mal_value_t){.tag = MAL_LIST,
                               .as.list = list_prepend(
                                   fn_args, (mal_value_t){.tag = MAL_NIL})});
    }

    return arg->value.as.atom->value;
}

static mal_value_t builtin_fn_cons(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'cons', expected sequence of 3, found "
                "%zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (da.items[2].tag != MAL_LIST && da.items[2].tag != MAL_VEC) {
        fprintf(stderr,
                "ERROR: Invalid type for 'cons', expected a list as second "
                "parameter.\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_list_t* lst = list_prepend(da.items[2].as.list, da.items[1]);
    mal_value_t       v = {.tag = MAL_LIST, .as.list = lst};

    da_free(&da);
    return v;
}

static mal_value_t builtin_fn_concat(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* lst = NULL;

    for (mal_value_list_t* arg = args.as.list->next; arg != NULL;
         arg = arg->next) {
        if (arg->value.tag != MAL_LIST && arg->value.tag != MAL_VEC) {
            fprintf(stderr,
                    "ERROR: Invalid type for 'concat', expected just lists and "
                    "vectors as parameters.\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        for (mal_value_list_t* inner = arg->value.as.list; inner != NULL;
             inner = inner->next) {
            lst = list_prepend(lst, inner->value);
        }
    }

    mal_value_t v = {.tag = MAL_LIST, .as.list = list_reverse(lst)};
    return v;
}

static mal_value_t builtin_fn_nth(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'nth', expected sequence of 3, found "
                "%zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (da.items[1].tag != MAL_LIST && da.items[1].tag != MAL_VEC) {
        fprintf(stderr,
                "ERROR: Invalid type for 'nth', expected a list or vec as "
                "first parameter.\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (da.items[2].tag != MAL_NUM) {
        fprintf(stderr,
                "ERROR: Invalid type for 'nth', expected a number as second "
                "parameter.\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    size_t      nth = (size_t)da.items[2].as.num;
    mal_value_t v = {.tag = MAL_ERR};

    size_t i = 0;
    for (mal_value_list_t* l = da.items[1].as.list; l != NULL; l = l->next) {
        if (i == nth) {
            v = l->value;
            break;
        }

        i++;
    }

    if (v.tag == MAL_ERR) {
        return (mal_value_t){
            .tag = MAL_EXCEPTION,
            .as.atom = mal_exception_newfmt(
                "index %zu out of bounds for size %zu", nth, i)};
    }

    da_free(&da);
    return v;
}

static mal_value_t builtin_fn_first(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'first'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t lst = args.as.list->next->value;
    if (lst.tag == MAL_NIL) return lst;

    if (lst.tag != MAL_LIST && lst.tag != MAL_VEC) {
        fprintf(stderr,
                "ERROR: Invalid type for 'first', expected a list or vec as "
                "first parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (lst.as.list == NULL) return (mal_value_t){.tag = MAL_NIL};

    return lst.as.list->value;
}

static mal_value_t builtin_fn_rest(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'rest'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t lst = args.as.list->next->value;
    if (lst.tag == MAL_NIL) return (mal_value_t){.tag = MAL_LIST};

    if (lst.tag != MAL_LIST && lst.tag != MAL_VEC) {
        fprintf(stderr,
                "ERROR: Invalid type for 'rest', expected a list or vec as "
                "first parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (lst.as.list == NULL) return (mal_value_t){.tag = MAL_LIST};

    return (mal_value_t){.tag = MAL_LIST, .as.list = lst.as.list->next};
}

static mal_value_t builtin_fn_throw(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'throw'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    return (mal_value_t){
        .tag = MAL_EXCEPTION,
        .as.atom = mal_exception_new(args.as.list->next->value)};
}

static mal_value_t builtin_fn_apply(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size < 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'apply', expected sequence of at "
                "least 3, found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t fn = da.items[1];
    if (fn.tag != MAL_FN && fn.tag != MAL_BUILTIN) {
        fprintf(stderr,
                "ERROR: Invalid type for 'apply', expected a function as first "
                "parameter.\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t apply_args = da.items[da.size - 1];
    if (!is_valid_sequence(apply_args)) {
        fprintf(stderr,
                "ERROR: Invalid type for 'apply', expected a sequence as last "
                "parameter.\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_list_t* lst = NULL;
    for (mal_value_list_t* a = apply_args.as.list; a != NULL; a = a->next) {
        lst = list_prepend(lst, a->value);
    }

    lst = list_reverse(lst);

    for (size_t i = da.size - 2; i > 1; i--) {
        lst = list_prepend(lst, da.items[i]);
    }

    if (fn.tag == MAL_BUILTIN) {
        lst = list_prepend(lst, fn);

        return fn.as.builtin.impl(
            env, (mal_value_t){.tag = MAL_LIST, .as.list = lst});
    }

    mal_eval_result_t r = mal_eval_apply(fn, lst, env);
    if (is_error(r.value)) return r.value;

    return mal_eval(r.value, r.env);
}

static mal_value_t builtin_fn_map(UNUSED env_t* env, mal_value_t args) {
    if (args.as.list->next == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'map'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t fn = args.as.list->next->value;
    if (fn.tag != MAL_FN && fn.tag != MAL_BUILTIN) {
        fprintf(stderr,
                "ERROR: Invalid type for 'map', expected a function as first "
                "parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (args.as.list->next->next == NULL) {
        fprintf(stderr, "ERROR: Missing second parameter for 'map'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t list_to_map = args.as.list->next->next->value;
    if (list_to_map.tag != MAL_LIST && list_to_map.tag != MAL_VEC) {
        fprintf(stderr,
                "ERROR: Invalid type for 'map', expected a list or vec as "
                "second parameter.\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_list_t* out = NULL;
    for (mal_value_list_t* l = list_to_map.as.list; l != NULL; l = l->next) {
        mal_value_list_t* args = list_new(l->value, NULL);

        mal_value_t v = {0};

        if (fn.tag == MAL_BUILTIN) {
            args = list_prepend(args, fn);

            v = fn.as.builtin.impl(
                env, (mal_value_t){.tag = MAL_LIST, .as.list = args});
        } else {
            mal_eval_result_t r = mal_eval_apply(fn, args, env);
            if (is_error(r.value)) return r.value;

            v = mal_eval(r.value, r.env);
        }

        if (is_error(v)) return v;
        out = list_prepend(out, v);
    }

    return (mal_value_t){.tag = MAL_LIST, .as.list = list_reverse(out)};
}

void core_env_populate(env_t* env) {
#define SYMBOL(s) \
    { .tag = MAL_SYMBOL, .as.string = mal_string_new_from_cstr(s) }

#define BUILTIN(f)                                                  \
    {                                                               \
        .tag = MAL_BUILTIN, .as.builtin = {.impl = builtin_fn_##f } \
    }

#define PAIR(s, f) \
    { .key = SYMBOL(s), .value = BUILTIN(f) }

    typedef struct pair {
        mal_value_t key;
        mal_value_t value;
    } pair_t;

    pair_t core_pairs[] = {
        PAIR("+", add),     //
        PAIR("-", sub),     //
        PAIR("*", mul),     //
        PAIR("/", div),     //
        PAIR("=", equals),  //
        PAIR("<", lt),      //
        PAIR("<=", lte),    //
        PAIR(">", gt),      //
        PAIR(">=", gte),    //

        PAIR("count", count),    //
        PAIR("empty?", empty),   //
        PAIR("cons", cons),      //
        PAIR("concat", concat),  //
        PAIR("nth", nth),        //
        PAIR("first", first),    //
        PAIR("rest", rest),      //

        PAIR("pr-str", pr_str),    //
        PAIR("str", str),          //
        PAIR("prn", prn),          //
        PAIR("println", println),  //

        PAIR("vec", vec),                      //
        PAIR("assoc", assoc),                  //
        PAIR("dissoc", dissoc),                //
        PAIR("get", get),                      //
        PAIR("contains?", contains_question),  //
        PAIR("keys", keys),                    //
        PAIR("vals", vals),                    //

        PAIR("list", list),          //
        PAIR("vector", vector),      //
        PAIR("symbol", symbol),      //
        PAIR("keyword", keyword),    //
        PAIR("hash-map", hash_map),  //

        PAIR("list?", list_question),        //
        PAIR("nil?", nil_question),          //
        PAIR("true?", true_question),        //
        PAIR("false?", false_question),      //
        PAIR("symbol?", symbol_question),    //
        PAIR("keyword?", keyword_question),  //
        PAIR("vector?", vector_question),    //
        PAIR("map?", map_question),          //

        PAIR("sequential?", sequential_question),  //

        PAIR("read-string", read_str),  //
        PAIR("slurp", slurp),           //
        PAIR("eval", eval),             //

        PAIR("atom", atom),            //
        PAIR("atom?", atom_question),  //
        PAIR("deref", deref),          //
        PAIR("reset!", reset),         //
        PAIR("swap!", swap),           //

        PAIR("throw", throw),  //
        PAIR("apply", apply),  //
        PAIR("map", map),      //
    };

#undef SYMBOL
#undef BUILTIN
#undef PAIR

    for (size_t i = 0; i < sizeof(core_pairs) / sizeof(core_pairs[0]); i++) {
        env_set(env, core_pairs[i].key, core_pairs[i].value);
    }
}
