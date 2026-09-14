/* games.h - prototypes for every family engine.
 * Each family takes a GParams describing the configuration the
 * catalogue entry asked for. See each source file for field meanings.
 */
#ifndef GIC_GAMES_H
#define GIC_GAMES_H

#include "engine.h"

void fam_tictactoe(const GParams *p);
void fam_connect4(const GParams *p);
void fam_reversi(const GParams *p);
void fam_gomoku(const GParams *p);
void fam_checkers(const GParams *p);
void fam_mancala(const GParams *p);
void fam_nim(const GParams *p);
void fam_dots_boxes(const GParams *p);
void fam_battleship(const GParams *p);
void fam_mastermind(const GParams *p);
void fam_minesweeper(const GParams *p);
void fam_sudoku(const GParams *p);
void fam_fifteen(const GParams *p);
void fam_lights_out(const GParams *p);
void fam_flood_it(const GParams *p);
void fam_sokoban(const GParams *p);
void fam_memory(const GParams *p);
void fam_2048(const GParams *p);
void fam_hanoi(const GParams *p);
void fam_nonogram(const GParams *p);
void fam_snake(const GParams *p);
void fam_tetris(const GParams *p);
void fam_pong(const GParams *p);
void fam_breakout(const GParams *p);
void fam_invaders(const GParams *p);
void fam_dino(const GParams *p);
void fam_flappy(const GParams *p);
void fam_frogger(const GParams *p);
void fam_pacman(const GParams *p);
void fam_asteroids(const GParams *p);
void fam_blackjack(const GParams *p);
void fam_video_poker(const GParams *p);
void fam_war(const GParams *p);
void fam_go_fish(const GParams *p);
void fam_yahtzee(const GParams *p);
void fam_pig(const GParams *p);
void fam_slots(const GParams *p);
void fam_higher_lower(const GParams *p);
void fam_hangman(const GParams *p);
void fam_wordle(const GParams *p);
void fam_anagram(const GParams *p);
void fam_typing(const GParams *p);
void fam_guess_number(const GParams *p);
void fam_bulls_cows(const GParams *p);
void fam_rps(const GParams *p);
void fam_simon(const GParams *p);
void fam_snakes_ladders(const GParams *p);
void fam_dungeon(const GParams *p);
void fam_piano(const GParams *p);
void fam_maze(const GParams *p);
void fam_quiz(const GParams *p);
void fam_constraint(const GParams *p);
void fam_pegsolitaire(const GParams *p);
void fam_matchthree(const GParams *p);
void fam_wordsearch(const GParams *p);

#endif /* GIC_GAMES_H */
