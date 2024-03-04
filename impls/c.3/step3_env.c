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
            if (!env_get(env, ast, &value)) {
                fprintf(stderr, "ERROR: Undefined symbol: '%.*s' not found\n",
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
                if (k.tag == MAL_ERR) return k;

                mal_value_t v = mal_eval(e->value, env);
                if (v.tag == MAL_ERR) return v;

                mal_hashmap_put(out, k, v);
            }

            return (mal_value_t){.tag = ast.tag, .as.hashmap = out};
        }
        default: return ast;
    }
}

mal_value_t mal_read(string_t s) { return read_str(s); }

mal_value_t mal_eval_def(mal_value_t args, env_t* env) {
    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'def!', expected sequence of 3, "
                "found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (!is_valid_hashmap_key(da.items[1])) {
        fprintf(stderr, "ERROR: Invalid parameter type for 'def!' key\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t value = mal_eval(da.items[2], env);
    if (value.tag == MAL_ERR) return value;

    env_set(env, da.items[1], value);
    da_free(&da);
    return value;
}

mal_value_t mal_eval_let(mal_value_t args, env_t* outer) {
    env_t env = {.outer = outer};

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'def!', expected sequence of 3, "
                "found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (!is_valid_sequence(da.items[1])) {
        fprintf(stderr,
                "ERROR: Invalid type for first parameter of 'def!', "
                "expected sequence\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t key = {0};
    for (mal_value_list_t* binding = da.items[1].as.list; binding != NULL;
         binding = binding->next) {
        if (key.tag == MAL_ERR) {
            key = binding->value;
            continue;
        }

        mal_value_t value = mal_eval(binding->value, &env);
        if (value.tag == MAL_ERR) {
            da_free(&da);
            return value;
        }

        env_set(&env, key, value);

        key = (mal_value_t){0};
    }

    if (key.tag != MAL_ERR) {
        fprintf(stderr,
                "ERROR: Invalid length for bindings list, dangling key\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t v = mal_eval(da.items[2], &env);
    da_free(&da);
    free(env.data.entries);

    return v;
}

mal_value_t mal_eval(mal_value_t value, env_t* env) {
    if (value.tag != MAL_LIST) return mal_eval_ast(value, env);

    if (value.as.list == NULL) return value;

    mal_value_t fn = value.as.list->value;
    if (fn.tag == MAL_SYMBOL && fn.as.string->size == 4 &&
        memcmp(fn.as.string->chars, "def!", fn.as.string->size) == 0) {
        return mal_eval_def(value, env);
    }

    if (fn.tag == MAL_SYMBOL && fn.as.string->size == 4 &&
        memcmp(fn.as.string->chars, "let*", fn.as.string->size) == 0) {
        return mal_eval_let(value, env);
    }

    mal_value_t args = mal_eval_ast(value, env);
    if (args.tag == MAL_ERR) return args;

    assert(args.tag == MAL_LIST && args.as.list != NULL);
    fn = args.as.list->value;

    if (fn.tag == MAL_BUILTIN) return fn.as.builtin.impl(env, args);

    fprintf(stderr, "ERROR: Can't call non-function value\n");
    return (mal_value_t){.tag = MAL_ERR};
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
                "ERROR: Invalid size for 'builtin_fn_add', expected sequence "
                "of 3, found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    double acc = da.items[1].as.num;
    for (size_t i = 2; i < da.size; i++) {
        if (da.items[i].tag != MAL_NUM) {
            fprintf(
                stderr,
                "ERROR: invalid input to 'builtin_fn_add', expected number\n");
            da_free(&da);
            return (mal_value_t){.tag = MAL_ERR};
        }

        acc += da.items[i].as.num;
    }

    da_free(&da);
    return (mal_value_t){.tag = MAL_NUM, .as.num = acc};
}

mal_value_t builtin_fn_sub(UNUSED env_t* env, mal_value_t args) {
    assert(is_valid_sequence(args));

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size < 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'builtin_fn_sub', expected sequence "
                "of 3, found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    double acc = da.items[1].as.num;
    for (size_t i = 2; i < da.size; i++) {
        if (da.items[i].tag != MAL_NUM) {
            fprintf(
                stderr,
                "ERROR: invalid input to 'builtin_fn_sub', expected number\n");
            da_free(&da);
            return (mal_value_t){.tag = MAL_ERR};
        }

        acc -= da.items[i].as.num;
    }

    da_free(&da);
    return (mal_value_t){.tag = MAL_NUM, .as.num = acc};
}

mal_value_t builtin_fn_div(UNUSED env_t* env, mal_value_t args) {
    assert(is_valid_sequence(args));

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size < 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'builtin_fn_div', expected sequence "
                "of 3, found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    double acc = da.items[1].as.num;
    for (size_t i = 2; i < da.size; i++) {
        if (da.items[i].tag != MAL_NUM) {
            fprintf(
                stderr,
                "ERROR: invalid input to 'builtin_fn_div', expected number\n");
            da_free(&da);
            return (mal_value_t){.tag = MAL_ERR};
        }

        acc /= da.items[i].as.num;
    }

    da_free(&da);
    return (mal_value_t){.tag = MAL_NUM, .as.num = acc};
}

mal_value_t builtin_fn_mul(UNUSED env_t* env, mal_value_t args) {
    assert(is_valid_sequence(args));

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size < 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'builtin_fn_mul', expected sequence "
                "of 3, found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    double acc = da.items[1].as.num;
    for (size_t i = 2; i < da.size; i++) {
        if (da.items[i].tag != MAL_NUM) {
            fprintf(
                stderr,
                "ERROR: invalid input to 'builtin_fn_mul', expected number\n");
            da_free(&da);
            return (mal_value_t){.tag = MAL_ERR};
        }

        acc *= da.items[i].as.num;
    }

    da_free(&da);
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
        env_set(&env, keys[i], values[i]);
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

    // :)
    free(env.data.entries);

    return 0;
}

int main(UNUSED int argc, UNUSED char** argv) {
    gc_init();

    // this is some compiler black magic to make shure that this call is _never_
    // inlined, an as such our GC will work. (TGC depends on the allocations
    // beeing one call deep).
    int (*volatile func)(void) = actual_main;
    int r = func();

    gc_deinit();

    return r;
}
