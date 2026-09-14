# Games in C - zero-dependency terminal game suite.
#   make            build ./games
#   make run        build and launch the catalog browser
#   make play G=snake   build and launch one game directly
#   make catalog    regenerate the dictionaries and the 1000-game catalogue
#   make smoke      run the scripted no-crash harness over every game
#   make web        report how to open the browser version
#   make clean      remove build artefacts

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -O2
CPPFLAGS = -Iinclude
LDLIBS   =

BIN     = games
SRC     = $(wildcard src/*.c) $(wildcard src/engine/*.c) $(wildcard src/games/*.c)
OBJ     = $(SRC:.c=.o)

.PHONY: all run play catalog smoke web clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

play: $(BIN)
	./$(BIN) $(G)

catalog:
	python3 tools/sync_words.py
	python3 tools/sync_quiz.py
	python3 tools/gen_catalog.py

smoke: $(BIN)
	./tools/smoke_test.sh

web:
	@echo "Open web/index.html in any browser - no server required."

clean:
	rm -f $(OBJ) $(BIN)
