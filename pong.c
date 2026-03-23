#include "game.h"

#define PADDLE_H 4
#define PADDLE_W 1
int paddle1_y = 10;
int paddle2_y = 10;
int ball_x = MAX_X/2, ball_y = MAX_Y/2;
int ball_dx = 1, ball_dy = 1;
int score1 = 0, score2 = 0;

void draw_pong(WINDOW *win) {
  // Paddles
  for (int i = 0; i < PADDLE_H; i++) {
    mvwaddch(win, paddle1_y + i, 2, '|');
    mvwaddch(win, paddle2_y + i, MAX_X - 3, '|');
  }
  // Ball
  mvwaddch(win, ball_y, ball_x, 'O');
  // Net
  for (int i = 0; i < MAX_Y; i += 2) mvwaddch(win, i, MAX_X/2, '|');
  // Scores
  mvwprintw(win, 0, MAX_X/4, "Player1: %d", score1);
  mvwprintw(win, 0, 3*MAX_X/4, "Player2: %d", score2);
}

void update_pong() {
  ball_x += ball_dx;
  ball_y += ball_dy;

  // Wall bounce
  if (ball_y <= 1 || ball_y >= MAX_Y - 2) ball_dy = -ball_dy;

  // Paddle hit
  if (ball_x == 3 && ball_y >= paddle1_y && ball_y < paddle1_y + PADDLE_H) ball_dx = 1;
  if (ball_x == MAX_X - 4 && ball_y >= paddle2_y && ball_y < paddle2_y + PADDLE_H) ball_dx = -1;

  // Score
  if (ball_x < 1) {
    score2++;
    ball_x = MAX_X/2; ball_y = MAX_Y/2; ball_dx = 1;
  } else if (ball_x > MAX_X - 2) {
    score1++;
    ball_x = MAX_X/2; ball_y = MAX_Y/2; ball_dx = -1;
  }
}

void play_pong(void) {
  WINDOW *win = newwin(MAX_Y, MAX_X, 0, 0);
  box(win, 0, 0);
  keypad(win, TRUE);

  mvwprintw(win, 22, 1, "Player1 WASD, Player2 Arrows. Q quit");

  while (1) {
    werase(win);
    box(win, 0, 0);
    draw_pong(win);
    wrefresh(win);
    game_sleep(80);

    int ch = wgetch(win);
    if (ch == 'q') break;
    if (ch == 'w' && paddle1_y > 1) paddle1_y --;
    if (ch == 's' && paddle1_y < MAX_Y - PADDLE_H - 1) paddle1_y ++;
    if (ch == KEY_UP && paddle2_y > 1) paddle2_y --;
    if (ch == KEY_DOWN && paddle2_y < MAX_Y - PADDLE_H - 1) paddle2_y ++;

    update_pong();
  }
  delwin(win);
  endwin();
}

