#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "da.h"
#include "printer.h"
#include "reader.h"
#include "types.h"

mal_value_t         mal_read(string_t s) { return read_str(s); }
mal_value_t         mal_eval(mal_value_t value) { return value; }
mal_value_string_t* mal_print(mal_value_t value) { return pr_str(value, true); }

mal_value_string_t* mal_rep(string_t s) {
    mal_value_t val = mal_eval(mal_read(s));
    if (val.tag == MAL_ERR) return NULL;

    return mal_print(val);
}

int main(UNUSED int argc, UNUSED char** argv) {
    gc_init();

    while (true) {
        fprintf(stdout, "user> ");
        fflush(stdout);

        static char buf[1024] = {0};
        ssize_t     n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) break;

        string_t line = da_init_fixed(buf, n - 1);

        mal_value_string_t* result = mal_rep(line);
        if (result && result->size > 0) printf("%s\n", result->chars);
    }

    gc_deinit();

    return 0;
}
