CC = gcc

CFLAGS = -std=gnu11 -g -Wall -Wextra -Werror

LIBS =
LIBS_SRC = linenoise.c
LIBS_HEADERS = linenoise.h

S0_HEADERS = da.h da_config.h common.h
S0_SRC = step0_repl.c $(LIBS_SRC)
S0 = step0_repl

S1_HEADERS = reader.h printer.h types.h da.h da_config.h common.h
S1_SRC = step1_read_print.c reader.c printer.c types.c common.c
S1 = step1_read_print

S2_HEADERS = reader.h printer.h types.h da.h da_config.h common.h
S2_SRC = step2_eval.c reader.c printer.c types.c common.c
S2 = step2_eval

S3_HEADERS = reader.h printer.h types.h env.h da.h da_config.h common.h
S3_SRC = step3_env.c reader.c printer.c types.c env.c common.c
S3 = step3_env

S4_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h
S4_SRC = step4_if_fn_do.c reader.c printer.c types.c env.c core.c common.c
S4 = step4_if_fn_do

S5_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h
S5_SRC = step5_tco.c reader.c printer.c types.c env.c core.c common.c
S5 = step5_tco

S6_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h
S6_SRC = step6_file.c reader.c printer.c types.c env.c core.c common.c
S6 = step6_file

S7_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h
S7_SRC = step7_quote.c reader.c printer.c types.c env.c core.c common.c
S7 = step7_quote

S8_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h
S8_SRC = step8_macros.c reader.c printer.c types.c env.c core.c common.c
S8 = step8_macros

S9_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h
S9_SRC = step9_try.c reader.c printer.c types.c env.c core.c common.c
S9 = step9_try

SA_HEADERS = reader.h printer.h types.h env.h core.h da.h da_config.h common.h $(LIBS_HEADERS)
SA_SRC = stepA_mal.c reader.c printer.c types.c env.c core.c common.c $(LIBS_SRC)
SA = stepA_mal

all: $(S0) $(S1) $(S2) $(S3) $(S4) $(S5) $(S6) $(S7) $(S8) $(S9) $(SA)

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

$(S7): $(S7_SRC) $(S7_HEADERS)
	$(CC) $(CFLAGS) $(S7_SRC) $(LIBS) -o $(S7)

$(S8): $(S8_SRC) $(S8_HEADERS)
	$(CC) $(CFLAGS) $(S8_SRC) $(LIBS) -o $(S8)

$(S9): $(S9_SRC) $(S9_HEADERS)
	$(CC) $(CFLAGS) $(S9_SRC) $(LIBS) -o $(S9)

$(SA): $(SA_SRC) $(SA_HEADERS)
	$(CC) $(CFLAGS) $(SA_SRC) $(LIBS) -o $(SA)

.PHONY clean:
	rm -f $(S0) $(S1) $(S2) $(S3) $(S4) $(S5) $(S6) $(S7) $(S8) $(S9) $(SA)
