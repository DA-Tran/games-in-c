#include "game.h"

int deck[52];
int player_hand[11], dealer_hand[11];
int player_count = 0, dealer_count = 0;

int card_value(int card) {
  int val = (card % 13) + 1;
  return (val > 10) ? 10 : val;
}

void play_blackjack(void) {
  // Stub deck shuffle
  WINDOW *win = newwin(MAX_Y, MAX_X, 0, 0);
  box(win, 0, 0);
  mvwprintw(win, 1, 1, "BLACKJACK - H hit S stand Q quit");
  // Game logic stub
  wrefresh(win);
  wgetch(win);
  delwin(win);
  endwin();
}

