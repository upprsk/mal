#include "reader.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "da.h"
#include "types.h"

/// Tokens are just strings...
typedef string_t token_t;

/// Dynamic array for tokens
typedef struct vartok {
    token_t* items;
    size_t   size;
    size_t   capacity;
} vartok_t;

/// Provide `peek` and `next` "methods".
typedef struct reader {
    vartok_t tokens;
    size_t   idx;
} reader_t;

// =============================================================================

static bool reader_at_end(reader_t const* r) {
    return r->idx >= r->tokens.size;
}

static token_t reader_peek(reader_t const* r) {
    if (reader_at_end(r)) {
        return (token_t){.items = "\0", .size = 1, .capacity = 1};
    }

    return r->tokens.items[r->idx];
}

static token_t reader_next(reader_t* r) {
    token_t tok = reader_peek(r);
    if (!reader_at_end(r)) r->idx++;

    return tok;
}

// =============================================================================

/// If the given character is considered whitespace
static bool is_whitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n': return true;
        default: return false;
    }
}

/// If the given character is considered whitespace
static bool is_digit(char c) {
    switch (c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': return true;
        default: return false;
    }
}

/// If the given character is considered special
static bool is_special(char c) {
    switch (c) {
        case ',':  // very special
        case ';':  // very special

        case '[':
        case ']':
        case '{':
        case '}':
        case '(':
        case ')':
        case '\'':
        case '`':
        case '~':
        case '^':
        case '@': return true;
        default: return false;
    }
}

// Tokenize the given string into an array of tokens. The number of errors is
// returned in `err`. `err` may be null.
//
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static vartok_t tokenize(string_t s, size_t* err) {
    vartok_t tokens = {0};

    if (err != NULL) *err = 0;

    for (size_t i = 0; i < s.size;) {
        // handle nulls in the string
        if (s.items[i] == 0) break;

        // - `[\s,]*`: Matches any number of whitespaces or commas. This is not
        // captured so it will be ignored and not tokenized.
        while (i < s.size && (is_whitespace(s.items[i]) || s.items[i] == ',')) {
            i++;
        }

        // handle the whitespace shenanigans
        if (i >= s.size) break;

        switch (s.items[i]) {
            // `~@`: Captures the special two-characters `~@` (tokenized).
            case '~': {
                if (i + 1 < s.size && s.items[i + 1] == '@') {
                    da_append(&tokens, (string_t){.items = &s.items[i],
                                                  .size = 2,
                                                  .capacity = 2});
                    i++;
                } else {
                    da_append(&tokens, (string_t){.items = &s.items[i],
                                                  .size = 1,
                                                  .capacity = 1});
                }

                i++;
            } break;
            // `[\[\]{}()'`~^@]`: Captures any special single character, one
            // of `[]{}()'`~^@` (tokenized).
            case '[':
            case ']':
            case '{':
            case '}':
            case '(':
            case ')':
            case '\'':
            case '`':
            // case '~':
            case '^':
            case '@': {
                da_append(
                    &tokens,
                    (string_t){.items = &s.items[i], .size = 1, .capacity = 1});

                i++;
            } break;

            // `"(?:\\.|[^\\"])*"?`: Starts capturing at a double-quote and
            // stops at the next double-quote unless it was preceded by a
            // backslash in which case it includes it until the next
            // double-quote (tokenized). It will also match unbalanced
            // strings (no ending double-quote) which should be reported as
            // an error.
            case '"': {
                size_t start = i;
                for (i++; i < s.size && s.items[i] != '"'; i++) {
                    if (s.items[i] == '\\') i++;
                }

                if (s.items[i] != '"') {
                    fprintf(stderr, "ERROR: unbalanced string: `%.*s`\n",
                            (int)(i - start), &s.items[start]);
                    if (err != NULL) *err += 1;
                } else {
                    da_append(&tokens, (string_t){.items = &s.items[start],
                                                  .size = i - start + 1,
                                                  .capacity = i - start + 1});
                }

                i++;
            } break;

            // `;.*`: Captures any sequence of characters starting with `;`
            // (tokenized).
            case ';': {
                for (; i < s.size && s.items[i] != '\n'; i++) {
                }
            } break;

            // `[^\s\[\]{}('"`,;)]*`: Captures a sequence of zero or more
            // non special characters (e.g. symbols, numbers, "true",
            // "false", and "nil") and is sort of the inverse of the one
            // above that captures special characters (tokenized).
            default: {
                size_t start = i;
                for (; i < s.size && !is_whitespace(s.items[i]) &&
                       !is_special(s.items[i]);
                     i++) {
                }

                da_append(&tokens, (string_t){.items = &s.items[start],
                                              .size = i - start,
                                              .capacity = i - start});
            } break;
        }
    }

    return tokens;
}

// =============================================================================

static mal_value_t read_form(reader_t* r);
static mal_value_t read_list(reader_t* r);
static mal_value_t read_vector(reader_t* r);
static mal_value_t read_hashmap(reader_t* r);
static mal_value_t read_atom(reader_t* r);

// -----------------------------------------------------------------------------

// Read a value, using `read_form` or `read_list` depending on `(`
static mal_value_t read_form(reader_t* r) {
    if (reader_at_end(r)) {
        fprintf(stderr, "ERROR: unexpected EOF, expected form\n");
        return (mal_value_t){.tag = MAL_ERR};
    }

    if (reader_peek(r).items[0] == '(') return read_list(r);
    if (reader_peek(r).items[0] == '[') return read_vector(r);
    if (reader_peek(r).items[0] == '{') return read_hashmap(r);
    return read_atom(r);
}

// Read a list in between a pair given characters
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static mal_value_t read_sequence(reader_t* r, mal_value_tag_t tag, char endc) {
    reader_next(r);
    mal_value_t value = {.tag = tag};

    while (true) {
        if (reader_at_end(r)) {
            fprintf(stderr, "ERROR: unexpected EOF, expected `%c`\n", endc);
            return (mal_value_t){.tag = MAL_ERR};
        }

        if (reader_peek(r).items[0] == endc) {
            reader_next(r);
            return value;
        }

        mal_value_t child = read_form(r);
        value.as.list = list_append(value.as.list, child);
    }
}

// Read a list in between a pair of `(` and `)`.
static mal_value_t read_list(reader_t* r) {
    return read_sequence(r, MAL_LIST, ')');
}

// Read a list in between a pair of `[` and `]`.
static mal_value_t read_vector(reader_t* r) {
    return read_sequence(r, MAL_VEC, ']');
}

// Read a hashmap in between a pair of `{` and `}`.
static mal_value_t read_hashmap(reader_t* r) {
    reader_next(r);

    mal_value_hashmap_t* hm = mal_hashmap_new();

    while (true) {
        if (reader_at_end(r)) {
            fprintf(stderr, "ERROR: unexpected EOF, expected `}`\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        if (reader_peek(r).items[0] == '}') {
            reader_next(r);
            return (mal_value_t){.tag = MAL_HASHMAP, .as.hashmap = hm};
        }

        mal_value_t key = read_form(r);
        if (!is_valid_hashmap_key(key)) {
            fprintf(stderr, "ERROR: form can't be used as key in hashmap\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        if (reader_at_end(r)) {
            fprintf(
                stderr,
                "ERROR: unexpected EOF, expected hashmap value after key\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        if (reader_peek(r).items[0] == '}') {
            fprintf(stderr, "ERROR: uneven number of elements in map\n");
            return (mal_value_t){.tag = MAL_ERR};
        }

        mal_value_t value = read_form(r);
        mal_hashmap_put(hm, key, value);
    }
}

static mal_value_t read_atom(reader_t* r) {
    token_t tok = reader_next(r);

    switch (tok.items[0]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return (mal_value_t){.tag = MAL_NUM,
                                 .as.num = strtod(tok.items, NULL)};
        case '-': {
            if (tok.size == 1) {
                return (mal_value_t){
                    .tag = MAL_SYMBOL,
                    .as.string = mal_string_new(tok.items, tok.size)};
            }

            for (size_t i = 1; i < tok.size; i++) {
                if (!is_digit(tok.items[i])) {
                    return (mal_value_t){
                        .tag = MAL_SYMBOL,
                        .as.string = mal_string_new(tok.items, tok.size)};
                }
            }

            return (mal_value_t){.tag = MAL_NUM,
                                 .as.num = -strtod(&tok.items[1], NULL)};
        } break;
        case '"': {
            return (mal_value_t){.tag = MAL_STRING,
                                 .as.string = mal_string_escape(mal_string_new(
                                     tok.items + 1, tok.size - 2))};
        }
        case '@': {
            mal_value_list_t* lst = NULL;
            lst = list_append(
                lst,
                (mal_value_t){.tag = MAL_SYMBOL,
                              .as.string = mal_string_new_from_cstr("deref")});
            lst = list_append(lst, read_form(r));

            return (mal_value_t){.tag = MAL_LIST, .as.list = lst};
        } break;
        case '~': {
            if (tok.size == 2 && tok.items[1] == '@') {
                mal_value_list_t* lst = NULL;
                lst = list_append(
                    lst, (mal_value_t){.tag = MAL_SYMBOL,
                                       .as.string = mal_string_new_from_cstr(
                                           "splice-unquote")});
                lst = list_append(lst, read_form(r));

                return (mal_value_t){.tag = MAL_LIST, .as.list = lst};
            }

            mal_value_list_t* lst = NULL;
            lst = list_append(
                lst, (mal_value_t){
                         .tag = MAL_SYMBOL,
                         .as.string = mal_string_new_from_cstr("unquote")});
            lst = list_append(lst, read_form(r));

            return (mal_value_t){.tag = MAL_LIST, .as.list = lst};
        } break;
        case '`': {
            mal_value_list_t* lst = NULL;
            lst = list_append(
                lst, (mal_value_t){
                         .tag = MAL_SYMBOL,
                         .as.string = mal_string_new_from_cstr("quasiquote")});
            lst = list_append(lst, read_form(r));

            return (mal_value_t){.tag = MAL_LIST, .as.list = lst};
        } break;
        case '\'': {
            mal_value_list_t* lst = NULL;
            lst = list_append(
                lst,
                (mal_value_t){.tag = MAL_SYMBOL,
                              .as.string = mal_string_new_from_cstr("quote")});
            lst = list_append(lst, read_form(r));

            return (mal_value_t){.tag = MAL_LIST, .as.list = lst};
        } break;
        default: {
            // check for `nil`
            if (tok.size == 3 && memcmp(tok.items, "nil", tok.size) == 0) {
                return (mal_value_t){.tag = MAL_NIL};
            }

            // check for `true`
            if (tok.size == 4 && memcmp(tok.items, "true", tok.size) == 0) {
                return (mal_value_t){.tag = MAL_TRUE};
            }

            // check for `false`
            if (tok.size == 5 && memcmp(tok.items, "false", tok.size) == 0) {
                return (mal_value_t){.tag = MAL_FALSE};
            }

            // symbol or keyword
            mal_value_tag_t tag =
                tok.items[0] == ':' ? MAL_KEYWORD : MAL_SYMBOL;
            return (mal_value_t){
                .tag = tag, .as.string = mal_string_new(tok.items, tok.size)};
        }
    }
}

mal_value_t read_str(string_t s) {
    size_t   err = 0;
    vartok_t tokens = tokenize(s, &err);
    if (err != 0) {
        da_free(&tokens);
        return (mal_value_t){.tag = MAL_ERR};
    }

    reader_t    reader = {.tokens = tokens};
    mal_value_t value = read_form(&reader);
    if (value.tag == MAL_ERR) {
        da_free(&tokens);
        return value;
    }

    if (!reader_at_end(&reader)) {
        token_t tok = reader_peek(&reader);
        fprintf(stderr, "ERROR: expected EOF, found `%.*s`\n", (int)tok.size,
                tok.items);

        da_free(&tokens);
        return (mal_value_t){.tag = MAL_ERR};
    }

    da_free(&tokens);

    return value;
}
