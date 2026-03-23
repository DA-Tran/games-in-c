#include "game.h"

struct Note {
  char key;
  int freq;
};
struct Note notes[] = {
  {'q', 261}, // C4
  {'w', 293}, // D4
  {'e', 329}, // E4
  {'r', 349}, // F4
  {'t', 392}, // G4
  {'y', 440}, // A4
  {'u', 494}, // B4
  {'i', 523}, // C5
  {0,0}
};

void play_piano(void) {
  WINDOW *win = newwin(10, MAX_X, 0, 0);
  box(win, 0, 0);

  mvwprintw(win, 1, 1, "VIRTUAL PIANO - Press QWERTYUI for notes (ESC quit)");
  mvwprintw(win, 3, 1, "Notes: q=C4 w=D4 e=E4 r=F4 t=G4 y=A4 u=B4 i=C5");

  wrefresh(win);

  while (1) {
    int ch = getch();
    if (ch == 27) break; // ESC

    for (int n = 0; notes[n].key; n++) {
      if (ch == notes[n].key) {
        game_beep(notes[n].freq, 200);
        mvwprintw(win, 5, 1, "Playing: %c (%d Hz)     ", notes[n].key, notes[n].freq);
        wrefresh(win);
        game_sleep(200);
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 1, 1, "VIRTUAL PIANO - Press QWERTYUI for notes (ESC quit)");
        mvwprintw(win, 3, 1, "Notes: q=C4 w=D4 e=E4 r=F4 t=G4 y=A4 u=B4 i=C5");
        wrefresh(win);
        break;
      }
    }
  }
  delwin(win);
  endwin();
}

