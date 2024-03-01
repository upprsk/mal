#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "core.h"
#include "da.h"
#include "env.h"
#include "printer.h"
#include "reader.h"
#include "tgc.h"
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
            mal_value_hashmap_t* out =
                tgc_alloc(&gc, sizeof(mal_value_hashmap_t));

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
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (!is_valid_hashmap_key(da.items[1])) {
        fprintf(stderr, "ERROR: Invalid parameter type for 'def!' key\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t value = mal_eval(da.items[2], env);
    if (value.tag == MAL_ERR) return value;

    env_set(env, da.items[1], value);
    return value;
}

typedef struct mal_eval_result {
    mal_value_t value;
    env_t*      env;
} mal_eval_result_t;

#define mal_result_err() \
    (mal_eval_result_t) { .value = {.tag = MAL_ERR}, .env = NULL }

mal_eval_result_t mal_eval_let(mal_value_t args, env_t* outer) {
    env_t* env = tgc_alloc(&gc, sizeof(env_t));
    *env = (env_t){.outer = outer};

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'def!', expected sequence of 3, "
                "found %zu\n",
                da.size);
        return mal_result_err();
    }

    if (!is_valid_sequence(da.items[1])) {
        fprintf(stderr,
                "ERROR: Invalid type for first parameter of 'def!', "
                "expected sequence\n");
        return mal_result_err();
    }

    mal_value_t key = {0};
    for (mal_value_list_t* binding = da.items[1].as.list; binding != NULL;
         binding = binding->next) {
        if (key.tag == MAL_ERR) {
            key = binding->value;
            continue;
        }

        mal_value_t value = mal_eval(binding->value, env);
        if (value.tag == MAL_ERR) return mal_result_err();

        env_set(env, key, value);

        key = (mal_value_t){0};
    }

    if (key.tag != MAL_ERR) {
        fprintf(stderr,
                "ERROR: Invalid length for bindings list, dangling key\n");
        return mal_result_err();
    }

    return (mal_eval_result_t){.value = da.items[2], .env = env};
    // return mal_eval(da.items[2], env);
}

mal_value_t mal_eval_do(mal_value_t args, env_t* env) {
    mal_value_t last_value = {0};

    assert(args.as.list->next != NULL);
    for (mal_value_list_t* l = args.as.list->next; l->next != NULL;
         l = l->next) {
        mal_value_t value = mal_eval(l->value, env);
        if (value.tag == MAL_ERR) return value;

        last_value = value;
    }

    return last_value;
}

mal_value_t mal_eval_if(mal_value_t args, env_t* env) {
    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3 && da.size != 4) {
        fprintf(stderr,
                "ERROR: Invalid size for 'if', expected sequence of 3, "
                "found %zu\n",
                da.size);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t condition = mal_eval(da.items[1], env);
    if (condition.tag == MAL_ERR) return condition;

    if (condition.tag != MAL_NIL && condition.tag != MAL_FALSE) {
        return da.items[2];
        // return mal_eval(da.items[2], env);
    }

    if (da.size == 4) {
        return da.items[3];
        // return mal_eval(da.items[3], env);
    }
    return (mal_value_t){.tag = MAL_NIL};
}

mal_value_t mal_eval_fn(mal_value_t args, env_t* env) {
    assert(args.as.list != NULL);

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'fn*', expected sequence of 3, "
                "found %zu\n",
                da.size);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t values = da.items[1];
    mal_value_t body = da.items[2];
    da_free(&da);

    if (values.tag != MAL_LIST && values.tag != MAL_VEC) {
        fprintf(stderr,
                "ERROR: Invalid type for first parameter of 'fn*', "
                "expected sequence\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    list_to_da(values.as.list, &da);

    bool   is_variadic = false;
    size_t actual_size = 0;
    for (size_t i = 0; i < da.size; i++) {
        if (!is_valid_hashmap_key(da.items[i])) {
            fprintf(stderr,
                    "ERROR: Invalid type for parameter name of 'fn*', expected "
                    "hashable\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        if (!is_variadic) actual_size++;

        if (da.items[i].as.string->size == 1 &&
            da.items[i].as.string->chars[0] == '&') {
            is_variadic = true;
        }
    }

    mal_value_fn_t* fn = tgc_alloc(&gc, sizeof(mal_value_fn_t));
    *fn = (mal_value_fn_t){.is_variadic = is_variadic,
                           .binds = da,
                           .outer_env = env,
                           .body = body};

    return (mal_value_t){.tag = MAL_FN, .as.fn = fn};
}

mal_value_t mal_eval(mal_value_t value, env_t* env) {
    while (true) {
        if (value.tag != MAL_LIST) return mal_eval_ast(value, env);
        if (value.as.list == NULL) return value;

        mal_value_t fn = value.as.list->value;
        if (fn.tag == MAL_SYMBOL && fn.as.string->size == 4 &&
            memcmp(fn.as.string->chars, "def!", fn.as.string->size) == 0) {
            return mal_eval_def(value, env);
        }

        if (fn.tag == MAL_SYMBOL && fn.as.string->size == 4 &&
            memcmp(fn.as.string->chars, "let*", fn.as.string->size) == 0) {
            mal_eval_result_t r = mal_eval_let(value, env);
            env = r.env;
            value = r.value;
            continue;
        }

        if (fn.tag == MAL_SYMBOL && fn.as.string->size == 2 &&
            memcmp(fn.as.string->chars, "do", fn.as.string->size) == 0) {
            value = mal_eval_do(value, env);
            continue;
        }

        if (fn.tag == MAL_SYMBOL && fn.as.string->size == 2 &&
            memcmp(fn.as.string->chars, "if", fn.as.string->size) == 0) {
            value = mal_eval_if(value, env);
            continue;
        }

        if (fn.tag == MAL_SYMBOL && fn.as.string->size == 3 &&
            memcmp(fn.as.string->chars, "fn*", fn.as.string->size) == 0) {
            return mal_eval_fn(value, env);
        }

        mal_value_t args = mal_eval_ast(value, env);
        if (args.tag == MAL_ERR) return args;

        assert(args.tag == MAL_LIST && args.as.list != NULL);
        fn = args.as.list->value;

        if (fn.tag == MAL_BUILTIN) return fn.as.builtin.impl(env, args);
        if (fn.tag != MAL_FN) {
            fprintf(stderr, "ERROR: Can't call non-function value\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        env_t* fn_env = tgc_alloc(&gc, sizeof(env_t));

        mal_value_list_da_t exprs = {0};
        list_to_da(args.as.list->next, &exprs);

        if (!fn.as.fn->is_variadic && exprs.size != fn.as.fn->binds.size) {
            fprintf(stderr,
                    "ERROR: parameter count mismatch, function expected %zu "
                    "parameters, received %zu\n",
                    fn.as.fn->binds.size, exprs.size);
            return (mal_value_t){.tag = MAL_ERR};
        }

        if (fn.as.fn->is_variadic && exprs.size < fn.as.fn->binds.size - 2) {
            fprintf(stderr,
                    "ERROR: parameter count mismatch, variadic function "
                    "expected at east %zu parameters, received %zu\n",
                    fn.as.fn->binds.size - 2, exprs.size);
            return (mal_value_t){.tag = MAL_ERR};
        }

        env_init_from(fn_env, fn.as.fn->outer_env, fn.as.fn->binds.items,
                      exprs.items, fn.as.fn->binds.size, exprs.size);

        da_free(&exprs);

        value = fn.as.fn->body;
        env = fn_env;
        // return mal_eval(fn.as.fn->body, fn_env);
    }
}

string_t mal_print(mal_value_t value) { return pr_str(value, true); }

string_t mal_rep(string_t s, env_t* env) {
    mal_value_t val = mal_eval(mal_read(s), env);
    if (val.tag == MAL_ERR) return (string_t){0};

    return mal_print(val);
}

int actual_main(void) {
    env_t env = {0};

    core_env_populate(&env);

    static char const not_src[] = "(def! not (fn* (a) (if a false true)))";

    string_t s = mal_rep(string_init_with_cstr((char*)not_src), &env);
    da_free(&s);

    while (true) {
        fprintf(stdout, "user> ");
        fflush(stdout);

        static char buf[1024] = {0};
        ssize_t     n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) break;

        string_t line = da_init_fixed(buf, n - 1);

        string_t result = mal_rep(line, &env);
        if (result.size > 0) printf("%.*s\n", (int)result.size, result.items);
    }

    return 0;
}

int main(int argc, UNUSED char** argv) {
    tgc_start(&gc, &argc);

    // this is some compiler black magic to make shure that this call is _never_
    // inlined, an as such our GC will work. (TGC depends on the allocations
    // beeing one call deep).
    int (*volatile func)(void) = actual_main;
    int r = func();

    tgc_stop(&gc);

    return r;
}
