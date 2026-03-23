#include "game.h"
#include <time.h>

#define MAZE_H 21
#define MAZE_W 40
char maze[MAZE_H][MAZE_W + 1] = {
"####################",
"# . . . . . . . . #",
"# .###.###.###.###.#",
"# O         G     #",
"# .###.###########.#",
"# ........ . . . . #",
"#.###.###.###.###.#",
"# G . .   . . . . #",
"###################",
". . . . . . . . . . ",
"###################",
". . . . . . . . . . ",
"###################",
"# . . . . . . . . #",
"#.#############.###",
"# .......G...... #",
"##.#########.###.#",
"# . . . . . . . . #",
"# #################",
"#P.............   #",
"####################"
};

int px = 1, py = 20;
int gx = 30, gy = 3; // Simple ghost
int dx = 1, dy = 0;

void draw_maze(WINDOW *win) {
  for (int i = 0; i < MAZE_H; i++) {
    mvwaddstr(win, i, 0, maze[i]);
  }
  maze[py][px] = ' ';
  maze[gy][gx] = ' ';
  mvwaddch(win, py, px, '@');
  mvwaddch(win, gy, gx, 'G');
}

void update_ghost() {
  gx += dx;
  if (gx >= MAZE_W-1 || gx <= 1 || maze[gy][gx] == '#') dx = -dx;
}

void play_pacman(void) {
  WINDOW *win = newwin(MAZE_H+2, MAZE_W+2, 0, 0);
  box(win, 0, 0);
  keypad(win, TRUE);

  int score = 0;
  srand(time(NULL));

  mvwprintw(win, 23, 1, "Arrow keys move. Q to quit. Eat . score points!");

  while (1) {
    werase(win);
    box(win, 0, 0);
    draw_maze(win);
    mvwprintw(win, 0, 1, "Pacman - Score: %d", score);
    wrefresh(win);
    game_sleep(100);

    int ch = wgetch(win);
    switch (ch) {
      case KEY_UP: dy = -1; dx = 0; break;
      case KEY_DOWN: dy = 1; dx = 0; break;
      case KEY_LEFT: dx = -1; dy = 0; break;
      case KEY_RIGHT: dx = 1; dy = 0; break;
      case 'q': return;
    }

    // Move player
    int nx = px + dx, ny = py + dy;
    if (maze[ny][nx] != '#' ) {
      if (maze[ny][nx] == '.') score += 10;
      px = nx;
      py = ny;
    }

    update_ghost();

    // Collision stub
    if (px == gx && py == gy) {
      mvwprintw(win, 10, 10, "GAME OVER!");
      wrefresh(win);
      game_sleep(2000);
      return;
    }
  }
  delwin(win);
}

