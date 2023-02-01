CC = gcc
SRC = $(wildcard src/*/*.c) $(wildcard src/*/*/*.c)
OBJ = $(SRC:.c=.o)
LIB = lib/libjson.so
BIN = test
CFLAGS = -Wall -Wextra -Werror -pedantic -fPIC

%.o: %.c
	$(CC) $(CFLAGS) -g -c $< -o $@

$(LIB): $(OBJ)
	$(CC) $(OBJ) -shared -o $(LIB) -lcontainers
	cp src/document/document.h include/json
	cp src/util/errcode.h include/json

$(BIN): $(LIB)
	$(CC) src/test.c -o $(BIN) -Llib -ljson -lcontainers -Iinclude -Wl,-rpath,lib

.PHONY: clean install
clean:
	@rm $(OBJ)
	@rm $(LIB)
	@rm $(BIN)
install:
	@cp -r include/json ~/.local/include
	@cp $(LIB) ~/.local/lib
