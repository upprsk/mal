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
#include "types.h"

mal_value_t mal_eval(mal_value_t value, env_t* env);

mal_value_t mal_eval_ast(mal_value_t ast, env_t* env) {
    switch (ast.tag) {
        case MAL_SYMBOL: {
            mal_value_t value = {0};
            if (!env_get(env, ast, &value)) {
                // fprintf(stderr, "ERROR: Undefined symbol: '%.*s' not
                // found\n",
                //         (int)ast.as.string->size, ast.as.string->chars);

                return (mal_value_t){
                    .tag = MAL_EXCEPTION,
                    .as.atom = mal_exception_newfmt("'%s' not found",
                                                    ast.as.string->chars)};
            }

            return value;
        } break;
        case MAL_VEC:
        case MAL_LIST: {
            mal_value_list_t* out = NULL;

            for (mal_value_list_t* l = ast.as.list; l != NULL; l = l->next) {
                mal_value_t v = mal_eval(l->value, env);
                if (is_error(v)) return v;
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
                if (is_error(k)) return k;

                mal_value_t v = mal_eval(e->value, env);
                if (is_error(v)) return v;

                mal_hashmap_put(out, k, v);
            }

            return (mal_value_t){.tag = ast.tag, .as.hashmap = out};
        }
        default: return ast;
    }
}

mal_value_t mal_read(string_t s) { return read_str(s); }

static mal_value_t mal_eval_def(mal_value_t args, env_t* env) {
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
    if (is_error(value)) {
        da_free(&da);
        return value;
    }

    env_set(env, da.items[1], value);

    da_free(&da);
    return value;
}

static mal_value_t mal_eval_defmacro(mal_value_t args, env_t* env) {
    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'defmacro!', expected sequence of 3, "
                "found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (!is_valid_hashmap_key(da.items[1])) {
        fprintf(stderr, "ERROR: Invalid parameter type for 'defmacro!' key\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t value = mal_eval(da.items[2], env);
    if (is_error(value)) {
        da_free(&da);
        return value;
    }

    if (value.tag != MAL_FN) {
        fprintf(stderr,
                "ERROR: Invalid parameter type for 'defmacro!' value, not a "
                "function\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    value.as.fn->is_macro = true;
    env_set(env, da.items[1], value);

    da_free(&da);
    return value;
}

#define mal_result_err() \
    (mal_eval_result_t) { .value = {.tag = MAL_ERR}, .env = NULL }

static mal_eval_result_t mal_eval_let(mal_value_t args, env_t* outer) {
    env_t* env = env_new(outer);

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'let*', expected sequence of 3, "
                "found %zu\n",
                da.size);
        da_free(&da);
        return mal_result_err();
    }

    if (!is_valid_sequence(da.items[1])) {
        fprintf(stderr,
                "ERROR: Invalid type for first parameter of 'let*', "
                "expected sequence\n");
        da_free(&da);
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
        if (is_error(value)) {
            da_free(&da);
            return (mal_eval_result_t){.value = value, .env = NULL};
        }

        env_set(env, key, value);

        key = (mal_value_t){0};
    }

    if (key.tag != MAL_ERR) {
        fprintf(stderr,
                "ERROR: Invalid length for bindings list, dangling key\n");
        da_free(&da);
        return mal_result_err();
    }

    mal_eval_result_t v = (mal_eval_result_t){.value = da.items[2], .env = env};
    da_free(&da);

    return v;
}

static mal_value_t mal_eval_do(mal_value_t args, env_t* env) {
    if (args.as.list->next == NULL) {
        fprintf(stderr,
                "ERROR: Missing parameter for 'do', expected at least one\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_list_t* l = args.as.list->next;
    for (; l->next != NULL; l = l->next) {
        mal_value_t value = mal_eval(l->value, env);
        if (is_error(value)) return value;
    }

    return l->value;
}

static mal_value_t mal_eval_if(mal_value_t args, env_t* env) {
    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3 && da.size != 4) {
        fprintf(stderr,
                "ERROR: Invalid size for 'if', expected sequence of 3, "
                "found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t condition = mal_eval(da.items[1], env);
    if (is_error(condition)) {
        da_free(&da);
        return condition;
    }

    if (condition.tag != MAL_NIL && condition.tag != MAL_FALSE) {
        mal_value_t v = da.items[2];

        da_free(&da);
        return v;
    }

    if (da.size == 4) {
        mal_value_t v = da.items[3];
        da_free(&da);

        return v;
    }

    da_free(&da);
    return (mal_value_t){.tag = MAL_NIL};
}

static mal_value_t mal_eval_fn(mal_value_t args, env_t* env) {
    assert(args.as.list != NULL);

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3) {
        fprintf(stderr,
                "ERROR: Invalid size for 'fn*', expected sequence of 3, "
                "found %zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t values = da.items[1];
    mal_value_t body = da.items[2];
    da_free(&da);

    if (values.tag != MAL_LIST && values.tag != MAL_VEC) {
        fprintf(stderr,
                "ERROR: Invalid type for first parameter of 'fn*', "
                "expected sequence\n");
        da_free(&da);
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
            da_free(&da);
            return (mal_value_t){.tag = MAL_ERR};
        }

        if (!is_variadic) actual_size++;

        if (da.items[i].as.string->size == 1 &&
            da.items[i].as.string->chars[0] == '&') {
            is_variadic = true;
        }
    }

    return (mal_value_t){
        .tag = MAL_FN, .as.fn = mal_fn_new(is_variadic, false, da, body, env)};
}

static mal_value_t mal_eval_try(mal_value_t args, env_t* env) {
    assert(args.as.list != NULL);

    mal_value_list_da_t da = {0};
    list_to_da(args.as.list, &da);

    if (da.size != 3 && da.size != 2) {
        fprintf(stderr,
                "ERROR: Invalid size for 'try*', expected sequence of 3, found "
                "%zu\n",
                da.size);
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (da.size == 3 && da.items[2].tag != MAL_LIST) {
        fprintf(stderr,
                "ERROR: Invalid type for second parameter of 'try*', expected "
                "list\n");
        da_free(&da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (da.size == 2) {
        mal_value_t r = mal_eval(da.items[1], env);
        if (r.tag != MAL_EXCEPTION) return r;

        return r.as.atom->value;
    }

    mal_value_list_da_t catch_da = {0};
    list_to_da(da.items[2].as.list, &catch_da);

    if (catch_da.size != 3) {
        fprintf(
            stderr,
            "ERROR: Invalid size for 'catch*', expected sequence of 3, found "
            "%zu\n",
            da.size);
        da_free(&da);
        da_free(&catch_da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (!mal_value_equal_cstr(catch_da.items[0], "catch*")) {
        fprintf(
            stderr,
            "ERROR: Invalid input for 'try*', expected sequence of 'catch*'\n");
        da_free(&da);
        da_free(&catch_da);
        return (mal_value_t){.tag = MAL_ERR};
    }

    mal_value_t r = mal_eval(da.items[1], env);
    if (r.tag != MAL_EXCEPTION) return r;

    env_t* catch_env = env_new(env);
    env_set(catch_env, catch_da.items[1], r.as.atom->value);

    return mal_eval(catch_da.items[2], catch_env);
}

mal_eval_result_t mal_eval_apply(mal_value_t fn_value, mal_value_list_t* args,
                                 UNUSED env_t* env) {
    if (fn_value.tag != MAL_FN) {
        fprintf(stderr, "ERROR: Can't call non-function value\n");
        return mal_result_err();
    }

    mal_value_fn_t* fn = fn_value.as.fn;

    mal_value_list_da_t args_da = {0};
    list_to_da(args, &args_da);

    if (!fn->is_variadic && args_da.size != fn->binds.size) {
        fprintf(stderr,
                "ERROR: parameter count mismatch, function expected %zu "
                "parameters, received %zu\n",
                fn->binds.size, args_da.size);
        return mal_result_err();
    }

    if (fn->is_variadic && args_da.size < fn->binds.size - 2) {
        fprintf(stderr,
                "ERROR: parameter count mismatch, variadic function "
                "expected at east %zu parameters, received %zu\n",
                fn->binds.size - 2, args_da.size);
        return mal_result_err();
    }

    env_t* fn_env = env_new(NULL);
    env_init_from(fn_env, fn->outer_env, fn->binds.items, args_da.items,
                  fn->binds.size, args_da.size);

    da_free(&args_da);

    return (mal_eval_result_t){.value = fn->body, .env = fn_env};
}

static mal_value_t mal_quasiquote(mal_value_t arg) {
    // If ast is a list ...
    if (arg.tag == MAL_LIST || arg.tag == MAL_VEC) {
        mal_value_list_t* arg_list = arg.as.list;
        if (arg_list == NULL) {
            if (arg.tag == MAL_VEC) {
                mal_value_list_t* out =
                    list_new((mal_value_t){.tag = MAL_LIST}, NULL);
                out = list_prepend(
                    out, (mal_value_t){
                             .tag = MAL_SYMBOL,
                             .as.string = mal_string_new_from_cstr("vec")});
                return (mal_value_t){.tag = MAL_LIST, .as.list = out};
            }

            return arg;
        }

        // If ast is a list starting with the "unquote" symbol, return its
        // second element.
        if (arg.tag == MAL_LIST &&
            mal_value_equal_cstr(arg_list->value, "unquote")) {
            if (arg.as.list->next) return arg.as.list->next->value;

            return (mal_value_t){.tag = MAL_NIL};
        }

        // If ast is a list failing previous test, the result will be a list
        // populated by the following process:
        //
        // The result is initially an empty list. Iterate over each element elt
        // of ast in reverse order:
        // - If elt is a list starting with the "splice-unquote" symbol, replace
        //   the current result with a list containing: the "concat" symbol, the
        //   second element of elt, then the previous result.
        // - Else replace the current result with a list containing: the "cons"
        //   symbol, the result of calling quasiquote with elt as argument, then
        //   the previous result.

        // The result is initially an empty list.
        mal_value_list_t* out = list_new((mal_value_t){.tag = MAL_LIST}, NULL);

        // Iterate over each element elt of ast in reverse order:
        mal_value_list_da_t ast = {0};
        list_to_da(arg_list, &ast);
        for (ssize_t i = ast.size - 1; i >= 0; i--) {
            mal_value_t elt = ast.items[i];
            // If elt is a list starting with the "splice-unquote" symbol,
            if (elt.tag == MAL_LIST && elt.as.list != NULL &&
                mal_value_equal_cstr(elt.as.list->value, "splice-unquote")) {
                // replace the current result with a list containing (reversed):
                // then the previous result.
                mal_value_list_t* tmp = out;

                // the second element of elt,
                assert(elt.as.list->next != NULL);
                tmp = list_prepend(tmp, elt.as.list->next->value);

                // the "concat" symbol,
                tmp = list_prepend(
                    tmp, (mal_value_t){
                             .tag = MAL_SYMBOL,
                             .as.string = mal_string_new_from_cstr("concat")});

                out = list_new((mal_value_t){.tag = MAL_LIST, .as.list = tmp},
                               NULL);
                continue;
            }

            // Else replace the current result with a list containing
            // (reversed): then the previous result.
            mal_value_list_t* tmp = out;

            // the result of calling quasiquote with elt as argument
            tmp = list_prepend(tmp, mal_quasiquote(elt));

            // the "cons" symbol
            tmp = list_prepend(
                tmp,
                (mal_value_t){.tag = MAL_SYMBOL,
                              .as.string = mal_string_new_from_cstr("cons")});

            out =
                list_new((mal_value_t){.tag = MAL_LIST, .as.list = tmp}, NULL);
        }

        da_free(&ast);
        if (arg.tag == MAL_VEC) {
            out = list_prepend(
                out,
                (mal_value_t){.tag = MAL_SYMBOL,
                              .as.string = mal_string_new_from_cstr("vec")});
            return (mal_value_t){.tag = MAL_LIST, .as.list = out};
        }

        return out->value;
    }

    if (arg.tag == MAL_HASHMAP || arg.tag == MAL_SYMBOL) {
        mal_value_list_t* tmp = NULL;
        tmp = list_prepend(tmp, arg);
        tmp = list_prepend(
            tmp, (mal_value_t){.tag = MAL_SYMBOL,
                               .as.string = mal_string_new_from_cstr("quote")});

        return (mal_value_t){.tag = MAL_LIST, .as.list = tmp};
    }

    return arg;
}

mal_value_t mal_macroexpand(mal_value_t ast, env_t* env) {
    while (is_macro_call(ast, env)) {
        mal_value_t macro_fn = {0};
        assert(env_get(env, ast.as.list->value, &macro_fn));

        mal_eval_result_t r = mal_eval_apply(macro_fn, ast.as.list->next, env);
        if (is_error(r.value)) return r.value;

        ast = mal_eval(r.value, r.env);
    }

    return ast;
}

mal_value_t mal_eval(mal_value_t value, env_t* env) {
    while (true) {
        value = mal_macroexpand(value, env);

        if (value.tag != MAL_LIST) return mal_eval_ast(value, env);
        if (value.as.list == NULL) return value;

        mal_value_t fn = value.as.list->value;

        if (mal_value_equal_cstr(fn, "def!")) {
            return mal_eval_def(value, env);
        }

        if (mal_value_equal_cstr(fn, "defmacro!")) {
            return mal_eval_defmacro(value, env);
        }

        if (mal_value_equal_cstr(fn, "let*")) {
            mal_eval_result_t r = mal_eval_let(value, env);
            env = r.env;
            value = r.value;
            continue;
        }

        if (mal_value_equal_cstr(fn, "do")) {
            value = mal_eval_do(value, env);
            continue;
        }

        if (mal_value_equal_cstr(fn, "if")) {
            value = mal_eval_if(value, env);
            continue;
        }

        if (mal_value_equal_cstr(fn, "fn*")) {
            return mal_eval_fn(value, env);
        }

        if (mal_value_equal_cstr(fn, "try*")) {
            return mal_eval_try(value, env);
        }

        if (mal_value_equal_cstr(fn, "quote")) {
            if (value.as.list->next == NULL) {
                fprintf(stderr, "ERROR: Missing argument for 'quote'\n");
                return (mal_value_t){.tag = MAL_ERR};
            }

            return value.as.list->next->value;
        }

        if (mal_value_equal_cstr(fn, "quasiquoteexpand")) {
            if (value.as.list->next == NULL) {
                fprintf(stderr,
                        "ERROR: Missing argument for 'quasiquoteexpand'\n");
                return (mal_value_t){.tag = MAL_ERR};
            }

            return mal_quasiquote(value.as.list->next->value);
        }

        if (mal_value_equal_cstr(fn, "quasiquote")) {
            if (value.as.list->next == NULL) {
                fprintf(stderr, "ERROR: Missing argument for 'quasiquote'\n");
                return (mal_value_t){.tag = MAL_ERR};
            }

            value = mal_quasiquote(value.as.list->next->value);
            continue;
        }

        if (mal_value_equal_cstr(fn, "macroexpand")) {
            if (value.as.list->next == NULL) {
                fprintf(stderr, "ERROR: Missing argument for 'macroexpand'\n");
                return (mal_value_t){.tag = MAL_ERR};
            }

            return mal_macroexpand(value.as.list->next->value, env);
        }

        mal_value_t args = mal_eval_ast(value, env);
        if (is_error(args)) return args;

        assert(args.tag == MAL_LIST && args.as.list != NULL);
        fn = args.as.list->value;

        if (fn.tag == MAL_BUILTIN) return fn.as.builtin.impl(env, args);

        mal_eval_result_t r = mal_eval_apply(fn, args.as.list->next, env);
        if (is_error(r.value)) return r.value;

        value = r.value;
        env = r.env;
    }
}

mal_value_string_t* mal_print(mal_value_t value) { return pr_str(value, true); }

mal_value_string_t* mal_rep(string_t s, env_t* env, mal_value_t* exception) {
    mal_value_t val = mal_eval(mal_read(s), env);
    if (is_error(val)) {
        if (val.tag == MAL_EXCEPTION && exception != NULL) {
            *exception = val.as.atom->value;
        }

        return NULL;
    }

    return mal_print(val);
}

int main(UNUSED int argc, UNUSED char** argv) {
    gc_init();

    env_t env = {0};

    {
        int start_index = argc > 1 ? 2 : 1;

        mal_value_list_t* argv_lst = NULL;
        for (int i = start_index; i < argc; i++) {
            mal_value_t arg = {.tag = MAL_STRING,
                               .as.string = mal_string_new_from_cstr(argv[i])};
            argv_lst = list_append(argv_lst, arg);
        }

        env_set(&env,
                (mal_value_t){.tag = MAL_SYMBOL,
                              .as.string = mal_string_new_from_cstr("*ARGV*")},
                (mal_value_t){.tag = MAL_LIST, .as.list = argv_lst});
    }

    core_env_populate(&env);

    {
        static char const not_src[] = "(def! not (fn* (a) (if a false true)))";

        mal_value_t r =
            mal_eval(mal_read(string_init_with_cstr((char*)not_src)), &env);
        assert(!is_error(r));
    }

    {
        static char const not_src[] =
            "(def! load-file"
            "    (fn* (f) (eval (read-string"
            "        (str \"(do \" (slurp f) \"\nnil)\")))))";

        mal_value_t r =
            mal_eval(mal_read(string_init_with_cstr((char*)not_src)), &env);
        assert(!is_error(r));
    }

    {
        static char const not_src[] =
            "(defmacro! cond (fn* (& xs) (if (> (count xs) 0) (list 'if (first "
            "xs) (if (> (count xs) 1) (nth xs 1) (throw \"odd number of forms "
            "to cond\")) (cons 'cond (rest (rest xs)))))))";
        mal_value_t r =
            mal_eval(mal_read(string_init_with_cstr((char*)not_src)), &env);
        assert(!is_error(r));
    }

    if (argc > 1) {
        mal_value_string_t* filename = mal_string_new_from_cstr(argv[1]);

        mal_value_list_t* lst = list_new(
            (mal_value_t){.tag = MAL_SYMBOL,
                          .as.string = mal_string_new_from_cstr("load-file")},
            NULL);
        lst = list_append(
            lst, (mal_value_t){.tag = MAL_STRING, .as.string = filename});

        mal_value_t r =
            mal_eval((mal_value_t){.tag = MAL_LIST, .as.list = lst}, &env);
        if (r.tag == MAL_EXCEPTION) {
            mal_value_string_t* s = pr_str(r.as.atom->value, false);
            printf("Uncaught exception: %s\n", s->chars);
        }
    } else {
        while (true) {
            fprintf(stdout, "user> ");
            fflush(stdout);

            static char buf[1024] = {0};
            ssize_t     n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n <= 0) break;

            string_t line = da_init_fixed(buf, n - 1);

            mal_value_t         exception = {0};
            mal_value_string_t* result = mal_rep(line, &env, &exception);
            if (result && result->size > 0) {
                printf("%s\n", result->chars);
            } else if (exception.tag != MAL_ERR) {
                mal_value_string_t* s = pr_str(exception, false);
                printf("Uncaught exception: %s\n", s->chars);
            }
        }
    }

    // :)
    free(env.data.entries);

    gc_deinit();

    return 0;
}
