#include "game.h"

#define GROUND_Y 20
#define DINO_X 5
int dino_y = GROUND_Y;
int cacti_x[5];
int score = 0;
int game_speed = 100;

void init_dino() {
  dino_y = GROUND_Y;
  score = 0;
  game_speed = 100;
  for (int i = 0; i < 5; i++) cacti_x[i] = 80 + i*20;
}

void update_dino(WINDOW *win) {
  // Ground
  for (int x = 0; x < MAX_X; x++) mvwaddch(win, GROUND_Y + 1, x, '_');

  // Dino
  mvwaddch(win, dino_y, DINO_X, (dino_y < GROUND_Y) ? '^' : 'v');  // Jump

  // Cacti
  for (int i = 0; i < 5; i++) {
    if (cacti_x[i] > 0) {
      mvwaddch(win, GROUND_Y, cacti_x[i], '|');
      cacti_x[i] --;
    } else {
      cacti_x[i] = MAX_X;
    }
  }

  // Collision
  if (dino_y == GROUND_Y && cacti_x[0] == DINO_X + 1) {
    mvwprintw(win, 10, 30, "GAME OVER! Score: %d", score);
    wrefresh(win);
    game_sleep(2000);
    return;
  }

  score ++;
}

void play_dino(void) {
  init_dino();
  WINDOW *win = newwin(MAX_Y, MAX_X, 0, 0);
  box(win, 0, 0);
  keypad(win, TRUE);

  mvwprintw(win, 0, 1, "DINO GAME - SPACE to jump, Q quit");
  mvwprintw(win, 22, 1, "Score: %d", score);

  int jumping = 0;
  while (1) {
    werase(win);
    box(win, 0, 0);
    update_dino(win);
    mvwprintw(win, 22, 1, "Score: %d Speed:%d", score, game_speed);
    wrefresh(win);

    game_sleep(game_speed);

    int ch = getch();
    if (ch == 'q') break;
    if (ch == ' ') {
      if (dino_y == GROUND_Y) jumping = 1;
    }

    if (jumping) {
      dino_y --;
      if (dino_y <= GROUND_Y - 5) jumping = 0;
    } else if (dino_y < GROUND_Y) {
      dino_y ++;
    }

    game_speed = 150 - (score / 10); if (game_speed < 30) game_speed = 30;
  }

  delwin(win);
  endwin();
}

