/* render.c - ANSI drawing primitives. */
#include "engine.h"
#include <stdarg.h>

void scr_clear(void)        { printf("\033[2J\033[H"); }
void scr_move(int r, int c) { printf("\033[%d;%dH", r, c); }
void scr_hide_cursor(void)  { printf("\033[?25l"); }
void scr_show_cursor(void)  { printf("\033[?25h"); }
void scr_flush(void)        { fflush(stdout); }

void draw_text(int row, int col, const char *s)
{
    scr_move(row, col);
    fputs(s, stdout);
}

void draw_textf(int row, int col, const char *fmt, ...)
{
    va_list ap;
    scr_move(row, col);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

/* Count printable columns, skipping ANSI escape sequences. */
static int visible_len(const char *s)
{
    int n = 0;
    while (*s) {
        if (*s == '\033') {
            while (*s && *s != 'm') s++;
            if (*s) s++;
            continue;
        }
        /* Treat a UTF-8 lead byte as one column, skip continuation bytes. */
        if (((unsigned char)*s & 0xC0) != 0x80) n++;
        s++;
    }
    return n;
}

void draw_centered(int row, int width, const char *s)
{
    int len = visible_len(s);
    int col = (width - len) / 2;
    if (col < 1) col = 1;
    draw_text(row, col, s);
}

void draw_hline(int row, int col, int len, const char *colour)
{
    int i;
    scr_move(row, col);
    if (colour) fputs(colour, stdout);
    for (i = 0; i < len; i++) fputs("─", stdout);
    if (colour) fputs(C_RESET, stdout);
}

void draw_box(int row, int col, int h, int w, const char *colour)
{
    int i;
    if (h < 2 || w < 2) return;
    if (colour) fputs(colour, stdout);

    scr_move(row, col);
    fputs("┌", stdout);
    for (i = 0; i < w - 2; i++) fputs("─", stdout);
    fputs("┐", stdout);

    for (i = 1; i < h - 1; i++) {
        scr_move(row + i, col);
        fputs("│", stdout);
        scr_move(row + i, col + w - 1);
        fputs("│", stdout);
    }

    scr_move(row + h - 1, col);
    fputs("└", stdout);
    for (i = 0; i < w - 2; i++) fputs("─", stdout);
    fputs("┘", stdout);

    if (colour) fputs(C_RESET, stdout);
}

void draw_title(const char *title, const char *subtitle)
{
    int rows, cols;
    char line[256];
    scr_size(&rows, &cols);
    scr_clear();

    snprintf(line, sizeof line, "%s%s  %s  %s", C_BOLD, C_CYAN, title, C_RESET);
    draw_centered(2, cols, line);

    if (subtitle && *subtitle) {
        snprintf(line, sizeof line, "%s%s%s", C_GREY, subtitle, C_RESET);
        draw_centered(3, cols, line);
    }
    /* No rule under the header. It used to be drawn across row 4, but every
     * game begins its own drawing on row 4 too, so the rule was overwritten in
     * pieces and read as a broken board edge - a chessboard appeared to have
     * its back rank embedded in a horizontal line. The title and subtitle
     * separate the header well enough on their own, and leaving row 4 clear
     * is what the games have always assumed. */
}
