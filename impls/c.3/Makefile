CC = gcc

CFLAGS = -std=c11 -g -Wall -Wextra -Werror

LIBS =
LIBS_SRC =
LIBS_HEADERS =

S0_HEADERS = da.h da_config.h common.h
S0_SRC = step0_repl.c $(LIBS_SRC)
S0 = step0_repl

S1_HEADERS = reader.h printer.h types.h da.h da_config.h common.h $(LIBS_HEADERS)
S1_SRC = step1_read_print.c reader.c printer.c types.c common.c $(LIBS_SRC)
S1 = step1_read_print

S2_HEADERS = reader.h printer.h types.h da.h da_config.h common.h $(LIBS_HEADERS)
S2_SRC = step2_eval.c reader.c printer.c types.c common.c $(LIBS_SRC)
S2 = step2_eval

S3_HEADERS = reader.h printer.h types.h env.h da.h da_config.h common.h $(LIBS_HEADERS)
S3_SRC = step3_env.c reader.c printer.c types.c env.c common.c $(LIBS_SRC)
S3 = step3_env

S4_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h $(LIBS_HEADERS)
S4_SRC = step4_if_fn_do.c reader.c printer.c types.c env.c core.c common.c $(LIBS_SRC)
S4 = step4_if_fn_do

S5_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h $(LIBS_HEADERS)
S5_SRC = step5_tco.c reader.c printer.c types.c env.c core.c common.c $(LIBS_SRC)
S5 = step5_tco

S6_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h $(LIBS_HEADERS)
S6_SRC = step6_file.c reader.c printer.c types.c env.c core.c common.c $(LIBS_SRC)
S6 = step6_file

all: $(S0) $(S1) $(S2) $(S3) $(S4) $(S5) $(S6)

$(S0): $(S0_SRC) $(S0_HEADERS)
	$(CC) $(CFLAGS) $(S0_SRC) $(LIBS) -o $(S0)
	
$(S1): $(S1_SRC) $(S1_HEADERS)
	$(CC) $(CFLAGS) $(S1_SRC) $(LIBS) -o $(S1)

$(S2): $(S2_SRC) $(S2_HEADERS)
	$(CC) $(CFLAGS) $(S2_SRC) $(LIBS) -o $(S2)

$(S3): $(S3_SRC) $(S3_HEADERS)
	$(CC) $(CFLAGS) $(S3_SRC) $(LIBS) -o $(S3)

$(S4): $(S4_SRC) $(S4_HEADERS)
	$(CC) $(CFLAGS) $(S4_SRC) $(LIBS) -o $(S4)

$(S5): $(S5_SRC) $(S5_HEADERS)
	$(CC) $(CFLAGS) $(S5_SRC) $(LIBS) -o $(S5)

$(S6): $(S6_SRC) $(S6_HEADERS)
	$(CC) $(CFLAGS) $(S6_SRC) $(LIBS) -o $(S6)

.PHONY clean:
	rm -f $(S0) $(S1) $(S2) $(S3) $(S4) $(S5) $(S6)
