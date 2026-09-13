/* engine.h - zero-dependency C99 terminal game engine.
 * Pure stdio + ANSI escapes. No ncurses, no external libraries.
 * Portable across POSIX (termios) and Windows (conio).
 */
#ifndef GIC_ENGINE_H
#define GIC_ENGINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ---------------------------------------------------------------- colours */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_REV     "\033[7m"

#define C_BLACK   "\033[30m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"
#define C_WHITE   "\033[37m"
#define C_GREY    "\033[90m"

#define BG_BLACK  "\033[40m"
#define BG_RED    "\033[41m"
#define BG_GREEN  "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE   "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN   "\033[46m"
#define BG_WHITE  "\033[47m"

/* ------------------------------------------------------------- key codes */
enum {
    KEY_NONE  = 0,
    KEY_ESC   = 27,
    KEY_ENTER = 10,
    KEY_TAB   = 9,
    KEY_BACKSPACE = 127,
    KEY_UP    = 1000,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_PGUP,
    KEY_PGDN,
    KEY_DEL
};

/* --------------------------------------------------------------- screen */
void   scr_init(void);            /* enter raw mode + hide cursor          */
void   scr_shutdown(void);        /* restore terminal on exit              */
void   scr_clear(void);
void   scr_move(int row, int col);          /* 1-based                     */
void   scr_hide_cursor(void);
void   scr_show_cursor(void);
void   scr_flush(void);
void   scr_size(int *rows, int *cols);

void   draw_text(int row, int col, const char *s);
void   draw_textf(int row, int col, const char *fmt, ...);
void   draw_centered(int row, int width, const char *s);
void   draw_box(int row, int col, int h, int w, const char *colour);
void   draw_title(const char *title, const char *subtitle);
void   draw_hline(int row, int col, int len, const char *colour);

/* ---------------------------------------------------------------- input */
int    key_get(void);             /* blocking, decodes arrows/escapes      */
int    key_poll(void);            /* non-blocking, KEY_NONE if nothing     */
void   key_flush(void);           /* discard pending input                 */
int    read_line(char *buf, int max);       /* cooked line entry           */
int    read_int(const char *prompt, int lo, int hi);
int    confirm(const char *prompt);
void   pause_msg(const char *msg);

/* --------------------------------------------------------------- timing */
void   sleep_ms(int ms);
long   now_ms(void);
void   beep(int freq_hz, int ms);   /* real tone on Windows, bell elsewhere */

/* ------------------------------------------------------------------ rng */
void   rng_seed(unsigned s);
int    rnd(int n);                /* 0 .. n-1                              */
int    rnd_range(int lo, int hi); /* inclusive                             */
double rnd_f(void);               /* 0.0 .. 1.0                            */
void   shuffle_int(int *a, int n);

/* ------------------------------------------------------------ high score */
int    score_load(const char *slug);
void   score_save(const char *slug, int score);
void   score_report(const char *slug, int score);

/* ------------------------------------------------------------- registry */
typedef void (*game_fn)(void);

typedef struct {
    const char *slug;
    const char *title;
    const char *genre;
    game_fn     run;
} GameEntry;

extern const GameEntry GAME_REGISTRY[];
extern const int       GAME_REGISTRY_COUNT;

const GameEntry *registry_find(const char *slug);

/* ------------------------------------------------------------- catalog */
typedef struct {
    const char *slug;
    const char *title;
    const char *genre;
    const char *mechanic;
    const char *players;
    int         difficulty;   /* 1..5                                      */
    const char *blurb;
    int         implemented;  /* 1 if a playable GameEntry exists          */
} CatalogEntry;

extern const CatalogEntry CATALOG[];
extern const int          CATALOG_COUNT;

#endif /* GIC_ENGINE_H */
