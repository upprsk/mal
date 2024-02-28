#pragma once

#include "types.h"

typedef struct env {
    struct env*         outer;
    mal_value_hashmap_t data;
} env_t;

bool   env_set(env_t* env, mal_value_t key, mal_value_t value);
env_t* env_find(env_t* env, mal_value_t key);
bool   env_get(env_t* env, mal_value_t key, mal_value_t* value);
