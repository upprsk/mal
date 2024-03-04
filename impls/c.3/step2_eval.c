#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "da.h"
#include "env.h"
#include "printer.h"
#include "reader.h"
#include "types.h"

mal_value_t mal_eval(mal_value_t value, env_t* env);

mal_value_t mal_eval_ast(mal_value_t ast, env_t* env) {
    switch (ast.tag) {
        case MAL_SYMBOL: {
            mal_value_t value = {0};
            if (!mal_hashmap_get(&env->data, ast, &value)) {
                fprintf(stderr, "ERROR: Undefined symbol: `%.*s`\n",
                        (int)ast.as.string->size, ast.as.string->chars);

                return (mal_value_t){.tag = MAL_ERR};
            }

            return value;
        } break;
        case MAL_VEC:
        case MAL_LIST: {
            mal_value_list_t* out = NULL;

            for (mal_value_list_t* l = ast.as.list; l != NULL; l = l->next) {
                mal_value_t v = mal_eval(l->value, env);
                if (v.tag == MAL_ERR) return v;
                out = list_append(out, v);
            }

            return (mal_value_t){.tag = ast.tag, .as.list = out};
        }
        case MAL_HASHMAP: {
            mal_value_hashmap_t* out = mal_hashmap_new();

            mal_value_hashmap_t* hm = ast.as.hashmap;
            for (size_t i = 0; i < hm->capacity; i++) {
                mal_hashmap_entry_t* e = &hm->entries[i];
                if (e->key.as.string == NULL) continue;

                mal_value_t k = mal_eval(e->key, env);
                mal_value_t v = mal_eval(e->value, env);

                mal_hashmap_put(out, k, v);
            }

            return (mal_value_t){.tag = ast.tag, .as.hashmap = out};
        }
        default: return ast;
    }
}

mal_value_t mal_read(string_t s) { return read_str(s); }

mal_value_t mal_eval(mal_value_t value, env_t* env) {
    if (value.tag != MAL_LIST) return mal_eval_ast(value, env);

    if (value.as.list == NULL) return value;

    mal_value_t args = mal_eval_ast(value, env);
    if (args.tag == MAL_ERR) return args;

    assert(args.tag == MAL_LIST && args.as.list != NULL);

    mal_value_t fn = args.as.list->value;
    if (fn.tag != MAL_BUILTIN) {
        fprintf(stderr, "ERROR: Can't call non-function value\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    return fn.as.builtin.impl(env, args);
}

mal_value_string_t* mal_print(mal_value_t value) { return pr_str(value, true); }

mal_value_string_t* mal_rep(string_t s, env_t* env) {
    mal_value_t val = mal_eval(mal_read(s), env);
    if (val.tag == MAL_ERR) return NULL;

    return mal_print(val);
}

mal_value_t builtin_fn_add(UNUSED env_t* env, mal_value_t args) {
    assert(is_valid_sequence(args));

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size < 3) {
        fprintf(stderr,
                "ERROR: Invalid size for `builtin_fn_add`, expected sequence "
                "of 3, found %zu\n",
                da.size);
        return (mal_value_t){.tag = MAL_ERR};
    }

    double acc = da.items[1].as.num;
    for (size_t i = 2; i < da.size; i++) {
        if (da.items[i].tag != MAL_NUM) {
            fprintf(
                stderr,
                "ERROR: invalid input to `builtin_fn_add`, expected number\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        acc += da.items[i].as.num;
    }

    return (mal_value_t){.tag = MAL_NUM, .as.num = acc};
}

mal_value_t builtin_fn_sub(UNUSED env_t* env, mal_value_t args) {
    assert(is_valid_sequence(args));

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size < 3) {
        fprintf(stderr,
                "ERROR: Invalid size for `builtin_fn_add`, expected sequence "
                "of 3, found %zu\n",
                da.size);
        return (mal_value_t){.tag = MAL_ERR};
    }

    double acc = da.items[1].as.num;
    for (size_t i = 2; i < da.size; i++) {
        if (da.items[i].tag != MAL_NUM) {
            fprintf(
                stderr,
                "ERROR: invalid input to `builtin_fn_add`, expected number\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        acc -= da.items[i].as.num;
    }

    return (mal_value_t){.tag = MAL_NUM, .as.num = acc};
}

mal_value_t builtin_fn_div(UNUSED env_t* env, mal_value_t args) {
    assert(is_valid_sequence(args));

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size < 3) {
        fprintf(stderr,
                "ERROR: Invalid size for `builtin_fn_add`, expected sequence "
                "of 3, found %zu\n",
                da.size);
        return (mal_value_t){.tag = MAL_ERR};
    }

    double acc = da.items[1].as.num;
    for (size_t i = 2; i < da.size; i++) {
        if (da.items[i].tag != MAL_NUM) {
            fprintf(
                stderr,
                "ERROR: invalid input to `builtin_fn_add`, expected number\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        acc /= da.items[i].as.num;
    }

    return (mal_value_t){.tag = MAL_NUM, .as.num = acc};
}

mal_value_t builtin_fn_mul(UNUSED env_t* env, mal_value_t args) {
    assert(is_valid_sequence(args));

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size < 3) {
        fprintf(stderr,
                "ERROR: Invalid size for `builtin_fn_add`, expected sequence "
                "of 3, found %zu\n",
                da.size);
        return (mal_value_t){.tag = MAL_ERR};
    }

    double acc = da.items[1].as.num;
    for (size_t i = 2; i < da.size; i++) {
        if (da.items[i].tag != MAL_NUM) {
            fprintf(
                stderr,
                "ERROR: invalid input to `builtin_fn_add`, expected number\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        acc *= da.items[i].as.num;
    }

    return (mal_value_t){.tag = MAL_NUM, .as.num = acc};
}

int actual_main(void) {
    env_t env = {0};

    mal_value_t keys[] = {
        {.tag = MAL_SYMBOL, .as.string = mal_string_new_from_cstr("+")},
        {.tag = MAL_SYMBOL, .as.string = mal_string_new_from_cstr("-")},
        {.tag = MAL_SYMBOL, .as.string = mal_string_new_from_cstr("*")},
        {.tag = MAL_SYMBOL, .as.string = mal_string_new_from_cstr("/")},
    };

    mal_value_t values[] = {
        {.tag = MAL_BUILTIN, .as.builtin = {.impl = builtin_fn_add}},
        {.tag = MAL_BUILTIN, .as.builtin = {.impl = builtin_fn_sub}},
        {.tag = MAL_BUILTIN, .as.builtin = {.impl = builtin_fn_mul}},
        {.tag = MAL_BUILTIN, .as.builtin = {.impl = builtin_fn_div}},
    };

    static_assert(sizeof(keys) == sizeof(values),
                  "keys and values should have same size");

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        mal_hashmap_put(&env.data, keys[i], values[i]);
    }

    while (true) {
        fprintf(stdout, "user> ");
        fflush(stdout);

        static char buf[1024] = {0};
        ssize_t     n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) break;

        string_t line = da_init_fixed(buf, n - 1);

        mal_value_string_t* result = mal_rep(line, &env);
        if (result && result->size > 0) printf("%s\n", result->chars);
    }

    return 0;
}

int main(UNUSED int argc, UNUSED char** argv) {
    gc_init();

    // this is some compiler black magic to make shure that this call is _never_
    // inlined, an as such our GC will work. (TGC depends on the allocations
    // beeing one call deep).
    //
    // NOTE: No longer needed
    int (*volatile func)(void) = actual_main;
    int r = func();

    gc_deinit();

    return r;
}
