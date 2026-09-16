/* render.c - ANSI drawing primitives. */
#include "engine.h"
#include <stdarg.h>

static int visible_len(const char *s);

/* ---------------------------------------------------- how much room a game
 * asked for.
 *
 * A board wider or taller than the window is not truncated by anything here:
 * the terminal simply discards whatever falls off the edge. The player is
 * left looking at a board with its right-hand columns missing, or a prompt
 * that scrolled away, and nothing says why. Some entries genuinely need more
 * than the 80x24 a terminal is assumed to have - a 16x16 sudoku needs 27
 * rows for the grid alone - so this is a real situation, not a bug to fix by
 * shrinking every board.
 *
 * Every character reaches the screen through scr_move or the draw helpers
 * below, so the extent can be measured in one place and reported. The mark is
 * a high-water one for the whole process: once a game has shown that it wants
 * 94 columns, that stays true even on the frames that happen to draw fewer. */
static int want_rows, want_cols;

static void used(int row, int col)
{
    if (row > want_rows) want_rows = row;
    if (col > want_cols) want_cols = col;
}

void scr_clear(void)        { printf("\033[2J\033[H"); }
void scr_hide_cursor(void)  { printf("\033[?25l"); }
void scr_show_cursor(void)  { printf("\033[?25h"); }

void scr_move(int r, int c)
{
    /* Positioning counts as use even with nothing written after it: the
     * cursor itself has to be visible for a prompt to be usable. */
    used(r, c);
    printf("\033[%d;%dH", r, c);
}

void scr_flush(void)
{
    int rows, cols;
    scr_size(&rows, &cols);
    if (want_rows > rows || want_cols > cols) {
        /* Written straight out rather than through the helpers, so the notice
         * does not itself count towards the extent it is reporting. Row 1 sits
         * above the title, which every game leaves clear. The cursor is saved
         * and restored so a game part-way through a frame is undisturbed. */
        printf("\033[s\033[1;1H\033[K%s  needs %dx%d, terminal is %dx%d - "
               "part of this game is off screen%s\033[u",
               C_YELLOW, want_cols, want_rows, cols, rows, C_RESET);
    }
    fflush(stdout);
}

void draw_text(int row, int col, const char *s)
{
    scr_move(row, col);
    used(row, col + visible_len(s) - 1);
    fputs(s, stdout);
}

void draw_textf(int row, int col, const char *fmt, ...)
{
    va_list ap;
    char buf[512];
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    scr_move(row, col);
    if (n >= 0 && (size_t)n < sizeof buf) {
        /* Formatted once into a buffer so its width can be measured. Anything
         * longer than the buffer is re-formatted straight out: the extent is
         * then understated, which is better than truncating what is drawn. */
        used(row, col + visible_len(buf) - 1);
        fputs(buf, stdout);
    } else {
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }
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
