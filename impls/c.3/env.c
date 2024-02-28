#include "env.h"

#include <assert.h>

#include "types.h"

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
