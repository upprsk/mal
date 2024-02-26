#include "types.h"

#include "tgc.h"

mal_value_list_t* list_prepend(mal_value_list_t* l, mal_value_t value) {
    mal_value_list_t* new_node = tgc_alloc(&gc, sizeof(mal_value_list_t));

    // Put the value in, and make the rest of the list the next element.
    *new_node = (mal_value_list_t){.value = value, .next = l};

    return new_node;
}

mal_value_list_t* list_append(mal_value_list_t* l, mal_value_t value) {
    mal_value_list_t* end = list_end(l);

    mal_value_list_t* new_node = tgc_alloc(&gc, sizeof(mal_value_list_t));
    *new_node = (mal_value_list_t){.value = value};

    if (end == NULL) {
        l = new_node;
    } else {
        end->next = new_node;
    }

    return l;
}

mal_value_list_t* list_end(mal_value_list_t* l) {
    if (l == NULL) return NULL;
    for (; l->next != NULL; l = l->next) {
    }

    return l;
}

tgc_t gc;
