#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "da.h"
#include "printer.h"
#include "reader.h"
#include "tgc.h"
#include "types.h"

mal_value_t mal_read(string_t s) { return read_str(s); }
mal_value_t mal_eval(mal_value_t value) { return value; }
string_t    mal_print(mal_value_t value) { return pr_str(value); }

string_t mal_rep(string_t s) { return mal_print(mal_eval(mal_read(s))); }

int actual_main(void) {
    while (true) {
        fprintf(stdout, "user> ");
        fflush(stdout);

        static char buf[1024] = {0};
        ssize_t     n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) break;

        string_t line = da_init_fixed(buf, n - 1);

        string_t result = mal_rep(line);
        printf("%.*s\n", (int)result.size, result.items);
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
