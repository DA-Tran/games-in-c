# Games in C - zero-dependency terminal game suite.
#   make            build ./games
#   make run        build and launch the catalog browser
#   make play G=snake   build and launch one game directly
#   make catalog    regenerate the dictionaries and the 1000-game catalogue
#   make smoke      run the scripted no-crash harness over every game
#   make strict     compile everything under a much stricter warning set
#   make asan       build ./games-asan with the address sanitiser
#   make smoke-asan run the harness against the sanitised build
#   make web        report how to open the browser version
#   make clean      remove build artefacts

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -O2
CPPFLAGS = -Iinclude
LDLIBS   =

BIN     = games
SRC     = $(wildcard src/*.c) $(wildcard src/engine/*.c) $(wildcard src/games/*.c)
OBJ     = $(SRC:.c=.o)

.PHONY: all run play catalog smoke stress strict asan smoke-asan web clean

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

stress: $(ASAN_BIN)
	./tools/stress_test.sh

# The ordinary build only warns about files it happens to recompile, so a
# clean tree can look silent while a warning sits in an untouched object.
# This checks every translation unit every time, under flags strict enough to
# have caught a dead chequerboard, two games that were secretly identical and
# a word list that rendered the same whether or not you had found the word.
STRICT = -std=c99 $(CPPFLAGS) -O2 -Wall -Wextra -Wshadow -Wstrict-prototypes \
         -Wmissing-prototypes -Wold-style-definition -Wduplicated-cond \
         -Wduplicated-branches -Wnull-dereference -Wformat=2 -Wredundant-decls \
         -Wundef -Wwrite-strings -Werror

# Every file is really compiled, not just parsed. -fsyntax-only stops before
# code generation, which silently skips -Wunused-function and
# -Wmaybe-uninitialized - the two that catch a function nothing calls any
# more and a variable read before it is set.
strict:
	@set -e; for f in $(SRC); do $(CC) $(STRICT) -c $$f -o /dev/null; done; \
	 echo "strict: clean across $(words $(SRC)) files"

# A sanitised build. The ordinary harness only notices a bug that crashes;
# a tableau overwritten by one card past its end usually does not, so the
# same scripted run is also played against this binary.
ASAN_BIN    = games-asan
ASAN_FLAGS  = -std=c99 -Wall -Wextra -O1 -g -fsanitize=address,undefined \
              -fno-omit-frame-pointer

asan: $(ASAN_BIN)

$(ASAN_BIN): $(SRC)
	$(CC) $(ASAN_FLAGS) $(CPPFLAGS) -o $@ $(SRC) $(LDLIBS)

smoke-asan: $(ASAN_BIN)
	BIN=./$(ASAN_BIN) TIMEOUT=60 ./tools/smoke_test.sh

web:
	@echo "Open web/index.html in any browser - no server required."

clean:
	rm -f $(OBJ) $(BIN) $(ASAN_BIN)
