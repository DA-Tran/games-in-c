CC = gcc
CFLAGS = -Wall -g
LIBS = -lncurses -ltinfo

all: menu

menu: game.h menu.c hangman.c rock_paper_scissors.c snakes_and_ladders.c 2048.c snake_game.c pacman.c dino_game.c virtual_piano.c
	$(CC) $(CFLAGS) -o menu menu.c hangman.c rock_paper_scissors.c snakes_and_ladders.c 2048.c snake_game.c pacman.c dino_game.c virtual_piano.c $(LIBS)

clean:
	rm -f menu *.o

.PHONY: all clean

