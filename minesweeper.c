#include "game.h"

#define MS_H 16
#define MS_W 30
char ms_board[MS_H][MS_W + 1];
int mines[MS_H * MS_W];

void play_minesweeper(void) {
  WINDOW *win = newwin(20, 35, 0, 0);
  box(win, 0, 0);
  mvwprintw(win, 1, 1, "MINESWEEPER - Numbers to click stub Q quit");
  wrefresh(win);
  wgetch(win);
  delwin(win);
  endwin();
}

