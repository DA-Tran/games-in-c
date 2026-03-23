#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#define CLEAR_SCREEN "cls"
#define SLEEP(ms) Sleep(ms)
#define BEEP(f,d) Beep(f, d)
#define GETCH _getch
#else
#define CLEAR_SCREEN "clear"
#define SLEEP(ms) usleep(ms*1000)
#define BEEP(f,d) printf("\a")
#define GETCH getchar
#endif

// ANSI escape codes
#define ANSI_CLEAR "\033[2J\033[H"
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_BLUE "\033[34m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"

void clear_screen(void);
void print_header(const char *title);
void print_footer(const char *msg);
void sleep_ms(int ms);
void beep(int freq, int duration);
int get_choice(int min, int max);
char get_char(void);

// Game prototypes (original 8)
void play_pacman(void);
void play_hangman(void);
void play_rps(void);  // Rock Paper Scissors
void play_snakes_ladders(void);
void play_dino(void);
void play_piano(void);
void play_snake(void);
void play_2048(void);

#endif

