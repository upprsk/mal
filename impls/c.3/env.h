#pragma once

#include "types.h"

typedef struct env {
    gc_obj_t obj;

    struct env*         outer;
    mal_value_hashmap_t data;
} env_t;

env_t* env_new(env_t* outer);

void env_init_from(env_t* env, env_t* outer, mal_value_t* binds,
                   mal_value_t* exprs, size_t binds_len, size_t exprs_len);

bool   env_set(env_t* env, mal_value_t key, mal_value_t value);
env_t* env_find(env_t* env, mal_value_t key);
bool   env_get(env_t* env, mal_value_t key, mal_value_t* value);
