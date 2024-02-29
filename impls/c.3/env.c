#include "env.h"

#include <assert.h>
#include <stdio.h>

#include "types.h"

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
void env_init_from(env_t* env, env_t* outer, mal_value_t* binds,
                   mal_value_t* exprs, size_t binds_len, size_t exprs_len) {
    // NOLINTEND(bugprone-easily-swappable-parameters)
    *env = (env_t){.outer = outer};

    for (size_t i = 0; i < binds_len; i++) {
        if (binds[i].as.string->size == 1 &&
            binds[i].as.string->chars[0] == '&') {
            assert(i + 1 < binds_len);

            mal_value_t rest_name = binds[i + 1];

            mal_value_list_t* rest = NULL;
            for (size_t j = i; j < exprs_len; j++) {
                rest = list_append(rest, exprs[j]);
            }

            env_set(env, rest_name,
                    (mal_value_t){.tag = MAL_LIST, .as.list = rest});
            break;
        }

        env_set(env, binds[i], exprs[i]);
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool env_set(env_t* env, mal_value_t key, mal_value_t value) {
    assert(env != NULL);

    return mal_hashmap_put(&env->data, key, value);
}

env_t* env_find(env_t* env, mal_value_t key) {
    if (env == NULL) return NULL;

    mal_value_t value = {0};
    if (mal_hashmap_get(&env->data, key, &value)) return env;
    return env_find(env->outer, key);
}

bool env_get(env_t* env, mal_value_t key, mal_value_t* value) {
    env_t* container = env_find(env, key);
    if (container == NULL) return false;

    return mal_hashmap_get(&container->data, key, value);
}
