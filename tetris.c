#include "game.h"

#define BOARD_H 20
#define BOARD_W 10
char board[BOARD_H][BOARD_W + 1];

int cur_x = 4, cur_y = 0;
int piece = 0;  // 0-6 tetromino

void init_board() {
  memset(board, ' ', BOARD_H * (BOARD_W + 1));
}

void play_tetris(void) {
  init_board();
  WINDOW *win = newwin(25, 40, 0, 0);
  box(win, 0, 0);
  keypad(win, TRUE);

  mvwprintw(win, 22, 1, "Arrow left/right/down, SPACE rotate, Q quit");

  while (1) {
    werase(win);
    box(win, 0, 0);
    // Draw board
    for (int i = 0; i < BOARD_H; i++) {
      mvwaddstr(win, i + 1, 1, board[i]);
    }
    // Draw current piece stub
    mvwaddstr(win, cur_y + 1, cur_x + 1, "##");
    wrefresh(win);

    int ch = wgetch(win);
    if (ch == 'q') break;
    if (ch == KEY_LEFT && cur_x > 0) cur_x--;
    if (ch == KEY_RIGHT && cur_x < BOARD_W - 2) cur_x++;
    if (ch == KEY_DOWN) cur_y++;
    if (ch == ' ') {} // Rotate stub
  }
  delwin(win);
  endwin();
}

