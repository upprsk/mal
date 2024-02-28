#include "core.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "da.h"
#include "env.h"
#include "printer.h"
#include "types.h"

#define buintin_fn_min_binary(args, op)                                      \
    mal_value_list_da_t da = {0};                                            \
    list_to_da((args).as.list, &da);                                         \
    if (da.size < 3) {                                                       \
        fprintf(stderr,                                                      \
                "ERROR: Invalid size for '" #op                              \
                "', expected sequence of 3, found "                          \
                "%zu\n",                                                     \
                da.size);                                                    \
        return (mal_value_t){.tag = MAL_ERR};                                \
    }                                                                        \
    if (da.items[1].tag != MAL_NUM) {                                        \
        fprintf(stderr,                                                      \
                "ERROR: invalid input to '" #op "', expected number\n");     \
        return (mal_value_t){.tag = MAL_ERR};                                \
    }                                                                        \
    double acc = da.items[1].as.num;                                         \
    for (size_t i = 2; i < da.size; i++) {                                   \
        if (da.items[i].tag != MAL_NUM) {                                    \
            fprintf(stderr,                                                  \
                    "ERROR: invalid input to '" #op "', expected number\n"); \
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

mal_value_t builtin_fn_add(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_min_binary(args, +);
}

mal_value_t builtin_fn_sub(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_min_binary(args, -);
}

mal_value_t builtin_fn_div(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_min_binary(args, /);
}

mal_value_t builtin_fn_mul(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_min_binary(args, *);
}

mal_value_t builtin_fn_prn(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'prn'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    string_t v = pr_str(arg->value, true);

    fwrite(v.items, sizeof(char), v.size, stdout);
    fprintf(stdout, "\n");

    da_free(&v);

    return (mal_value_t){.tag = MAL_NIL};
}

mal_value_t builtin_fn_list(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* out = NULL;
    for (mal_value_list_t* l = args.as.list->next; l != NULL; l = l->next) {
        out = list_append(out, l->value);
    }

    return (mal_value_t){.tag = MAL_LIST, .as.list = out};
}

mal_value_t builtin_fn_list_question(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'list?'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    return (mal_value_t){.tag =
                             arg->value.tag == MAL_LIST ? MAL_TRUE : MAL_FALSE};
}

mal_value_t builtin_fn_empty(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'empty'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (arg->value.tag != MAL_LIST) {
        fprintf(stderr, "ERROR: invalid input to 'empty', expected list\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    return (mal_value_t){.tag =
                             arg->value.as.list == NULL ? MAL_TRUE : MAL_FALSE};
}

mal_value_t builtin_fn_size(UNUSED env_t* env, mal_value_t args) {
    mal_value_list_t* arg = args.as.list->next;
    if (arg == NULL) {
        fprintf(stderr, "ERROR: Missing parameter for 'size'\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (arg->value.tag != MAL_LIST) {
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

mal_value_t builtin_fn_equals(UNUSED env_t* env, mal_value_t args) {
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

mal_value_t builtin_fn_lt(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_binary(args, <);
}

mal_value_t builtin_fn_lte(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_binary(args, <=);
}

mal_value_t builtin_fn_gt(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_binary(args, >);
}

mal_value_t builtin_fn_gte(UNUSED env_t* env, mal_value_t args) {
    buintin_fn_binary(args, >=);
}

void core_env_populate(env_t* env) {
    mal_value_t keys[] = {
        {.tag = MAL_SYMBOL,     .as.string = mal_string_new_from_cstr("+")},
        {.tag = MAL_SYMBOL,     .as.string = mal_string_new_from_cstr("-")},
        {.tag = MAL_SYMBOL,     .as.string = mal_string_new_from_cstr("*")},
        {.tag = MAL_SYMBOL,     .as.string = mal_string_new_from_cstr("/")},
        {.tag = MAL_SYMBOL,   .as.string = mal_string_new_from_cstr("prn")},
        {.tag = MAL_SYMBOL,  .as.string = mal_string_new_from_cstr("list")},
        {.tag = MAL_SYMBOL, .as.string = mal_string_new_from_cstr("list?")},
        {.tag = MAL_SYMBOL, .as.string = mal_string_new_from_cstr("empty")},
        {.tag = MAL_SYMBOL,  .as.string = mal_string_new_from_cstr("size")},
        {.tag = MAL_SYMBOL,     .as.string = mal_string_new_from_cstr("=")},
        {.tag = MAL_SYMBOL,     .as.string = mal_string_new_from_cstr("<")},
        {.tag = MAL_SYMBOL,    .as.string = mal_string_new_from_cstr("<=")},
        {.tag = MAL_SYMBOL,     .as.string = mal_string_new_from_cstr(">")},
        {.tag = MAL_SYMBOL,    .as.string = mal_string_new_from_cstr(">=")},
    };

    mal_value_t values[] = {
        {.tag = MAL_BUILTIN,           .as.builtin = {.impl = builtin_fn_add}},
        {.tag = MAL_BUILTIN,           .as.builtin = {.impl = builtin_fn_sub}},
        {.tag = MAL_BUILTIN,           .as.builtin = {.impl = builtin_fn_mul}},
        {.tag = MAL_BUILTIN,           .as.builtin = {.impl = builtin_fn_div}},
        {.tag = MAL_BUILTIN,           .as.builtin = {.impl = builtin_fn_prn}},
        {.tag = MAL_BUILTIN,          .as.builtin = {.impl = builtin_fn_list}},
        {.tag = MAL_BUILTIN, .as.builtin = {.impl = builtin_fn_list_question}},
        {.tag = MAL_BUILTIN,         .as.builtin = {.impl = builtin_fn_empty}},
        {.tag = MAL_BUILTIN,          .as.builtin = {.impl = builtin_fn_size}},
        {.tag = MAL_BUILTIN,        .as.builtin = {.impl = builtin_fn_equals}},
        {.tag = MAL_BUILTIN,            .as.builtin = {.impl = builtin_fn_lt}},
        {.tag = MAL_BUILTIN,           .as.builtin = {.impl = builtin_fn_lte}},
        {.tag = MAL_BUILTIN,            .as.builtin = {.impl = builtin_fn_gt}},
        {.tag = MAL_BUILTIN,           .as.builtin = {.impl = builtin_fn_gte}},
    };

    static_assert(sizeof(keys) == sizeof(values),
                  "keys and values should have same size");

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        env_set(env, keys[i], values[i]);
    }
}
