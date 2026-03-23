#include "game.h"

// 4x4 grid
int grid[4][4];
int score = 0;

void init_grid() {
  memset(grid, 0, sizeof(grid));
  score = 0;
}

void add_random_tile() {
  int empty[16];
  int count = 0;
  for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) {
    if (grid[i][j] == 0) empty[count++] = i*4 + j;
  }
  if (count > 0) {
    int idx = rand() % count;
    int i = empty[idx] / 4;
    int j = empty[idx] % 4;
    grid[i][j] = (rand() % 10 < 9) ? 2 : 4;
  }
}

void print_grid(WINDOW *win) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      char buf[10];
      sprintf(buf, "%6d ", grid[i][j]);
      if (grid[i][j] > 0) {
        init_pair(2 + log2(grid[i][j]), COLOR_BLACK, COLOR_WHITE);
        wattron(win, COLOR_PAIR(2 + log2(grid[i][j])));
      }
      mvwaddstr(win, i*3 + 2, j*8 + 2, buf);
      if (grid[i][j] > 0) wattroff(win, COLOR_PAIR(2 + log2(grid[i][j])));
    }
  }
  mvwprintw(win, 14, 2, "Score: %d", score);
}

int move_left() {
  int moved = 0;
  for (int i = 0; i < 4; i++) {
    int write = 0;
    for (int j = 0; j < 4; j++) {
      if (grid[i][j] != 0) {
        if (write == j) continue;
        if (write > 0 && grid[i][write-1] == grid[i][j]) {
          grid[i][write-1] *= 2;
          score += grid[i][write-1];
          moved = 1;
        } else {
          grid[i][write] = grid[i][j];
          if (write != j) moved = 1;
          write++;
        }
      }
    }
    for (int j = write; j < 4; j++) grid[i][j] = 0;
  }
  return moved;
}

// Similar for right, up, down (rotate logic or copy)

void play_2048(void) {
  init_grid();
  add_random_tile();
  add_random_tile();

  WINDOW *win = newwin(MAX_Y, MAX_X, 0, 0);
  box(win, 0, 0);
  keypad(win, TRUE);
  start_color();
  for (int i = 0; i < 10; i++) init_pair(i, COLOR_BLACK, COLOR_WHITE);  // Colors for tiles

  mvwprintw(win, 1, 1, "2048 - Arrow keys to swipe. ESC/Q to quit. Reach 2048!");

  while (1) {
    werase(win);
    box(win, 0, 0);
    print_grid(win);
    mvwprintw(win, 16, 1, "Arrow keys move. ESC/Q quit");
    wrefresh(win);

    int ch = wgetch(win);
    if (ch == 27 || ch == 'q' || ch == 'Q') break;

    int moved = 0;
    // Rotate for directions
    switch (ch) {
      case KEY_LEFT: moved = move_left(); break;
      case KEY_RIGHT: // Implement move_right
        // Transpose + reverse rows + left + reverse + transpose
        moved = 1; // Stub
        break;
      // Similar for up/down
    }

    if (moved) add_random_tile();

    // Check win/lose omitted for brevity
  }

  delwin(win);
  endwin();
}

