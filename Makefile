CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude

SRC = src/mymalloc.c
TEST_SRC = tests/test_malloc.c
TEST_BIN = tests/run_tests

.PHONY: all test clean

all: $(TEST_BIN)

$(TEST_BIN): $(SRC) $(TEST_SRC) include/mymalloc.h src/internal.h
	$(CC) $(CFLAGS) -o $@ $(SRC) $(TEST_SRC)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN)