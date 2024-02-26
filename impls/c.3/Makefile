CC = gcc

CFLAGS = -std=c11 -g -Wall -Wextra

LIBS =
LIBS_SRC = tgc.c
LIBS_HEADERS = tgc.h

S0_HEADERS = da.h da_config.h common.h
S0_SRC = step0_repl.c $(LIBS_SRC)
S0 = step0_repl

S1_HEADERS = reader.h printer.h types.h da.h da_config.h common.h $(LIBS_HEADERS)
S1_SRC = step1_read_print.c reader.c printer.c types.c common.c $(LIBS_SRC)
S1 = step1_read_print

all: $(S0) $(S1)

$(S0): $(S0_SRC) $(S0_HEADERS)
	$(CC) $(CFLAGS) $(S0_SRC) $(LIBS) -o $(S0)
	
$(S1): $(S1_SRC) $(S1_HEADERS)
	$(CC) $(CFLAGS) $(S1_SRC) $(LIBS) -o $(S1)

.PHONY clean:
	rm -f $(S0) $(S1)
