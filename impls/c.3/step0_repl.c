#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "da.h"

typedef struct string {
    char*  items;
    size_t size;
    size_t capacity;
} string_t;

string_t mal_read(string_t s) { return s; }
string_t mal_eval(string_t s) { return s; }
string_t mal_print(string_t s) { return s; }

string_t mal_rep(string_t s) { return mal_print(mal_eval(mal_read(s))); }

int main() {
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
