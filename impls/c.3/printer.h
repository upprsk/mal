#pragma once

#include <stdbool.h>

#include "common.h"
#include "types.h"

mal_value_string_t* pr_str(mal_value_t value, bool print_readably);
