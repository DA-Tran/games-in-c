/* GIC:PARAMETERISED mastermind
 * mastermind.c - code breaking at any width and palette size.
 * params: count = pegs (3..6), level = colours (6, 8 or 10)
 * The guess allowance scales with the size of the search space.
 */
#include "engine.h"
#include "games.h"

#define MAXPEGS 6
#define MAXCOLOURS 10
#define MAXTRIES 14

static int PEGS, COLOURS, TRIES;

static const char *CNAME = "RGBYMCWOPT";
static const char *CCOL[MAXCOLOURS] = {
    C_RED, C_GREEN, C_BLUE, C_YELLOW, C_MAGENTA,
    C_CYAN, C_WHITE, C_BOLD C_RED, C_BOLD C_GREEN, C_BOLD C_BLUE};

static int secret[MAXPEGS];
static int guess[MAXTRIES][MAXPEGS];
static int exact[MAXTRIES], partial[MAXTRIES];

static void feedback(const int *g, int *ex, int *pa)
{
    int sused[MAXPEGS] = {0}, gused[MAXPEGS] = {0}, i, j;
    *ex = *pa = 0;
    for (i = 0; i < PEGS; i++)
        if (g[i] == secret[i]) { (*ex)++; sused[i] = gused[i] = 1; }
    for (i = 0; i < PEGS; i++) {
        if (gused[i]) continue;
        for (j = 0; j < PEGS; j++) {
            if (sused[j] || g[i] != secret[j]) continue;
            sused[j] = gused[i] = 1;
            (*pa)++;
            break;
        }
    }
}

static void render(int row, int cur, int *work, int reveal, const char *msg)
{
    int r, i;
    {
        char sub[120];
        char pal[32];
        int c2;
        for (c2 = 0; c2 < COLOURS; c2++) pal[c2] = CNAME[c2];
        pal[COLOURS] = '\0';
        snprintf(sub, sizeof sub, "%d pegs, %d colours (%s), %d guesses",
                 PEGS, COLOURS, pal, TRIES);
        draw_title("MASTERMIND", sub);
    }
    draw_text(6, 24, "Left/Right pick a peg, Up/Down change colour, Enter submits");

    scr_move(8, 24);
    printf("Secret: ");
    for (i = 0; i < PEGS; i++) {
        if (reveal) printf("%s ● %s", CCOL[secret[i]], C_RESET);
        else printf("%s ? %s", C_GREY, C_RESET);
    }

    for (r = 0; r < TRIES; r++) {
        scr_move(10 + r, 24);
        printf("%s%2d%s ", r == row ? C_BOLD : C_GREY, r + 1, C_RESET);
        for (i = 0; i < PEGS; i++) {
            if (r < row) printf("%s ● %s", CCOL[guess[r][i]], C_RESET);
            else if (r == row)
                printf("%s%s %c %s", i == cur ? C_REV : "", CCOL[work[i]], CNAME[work[i]], C_RESET);
            else printf("%s · %s", C_GREY, C_RESET);
        }
        if (r < row)
            printf("   %s%d exact%s  %s%d partial%s",
                   C_GREEN, exact[r], C_RESET, C_YELLOW, partial[r], C_RESET);
    }
    draw_text(22, 20, "                                                        ");
    if (msg) draw_textf(22, 24, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_mastermind(const GParams *p)
{
    PEGS = gp_int(p->count, 4);
    if (PEGS < 3) PEGS = 3;
    if (PEGS > MAXPEGS) PEGS = MAXPEGS;
    COLOURS = gp_int(p->level, 6);
    if (COLOURS < 4) COLOURS = 4;
    if (COLOURS > MAXCOLOURS) COLOURS = MAXCOLOURS;
    /* Wider codes and bigger palettes need more attempts to stay fair. */
    TRIES = 8 + (PEGS - 3) + (COLOURS - 6) / 2;
    if (TRIES > MAXTRIES) TRIES = MAXTRIES;

    for (;;) {
        int work[MAXPEGS] = {0,0,0,0,0,0};
        int row = 0, cur = 0, i, won = 0;
        for (i = 0; i < PEGS; i++) secret[i] = rnd(COLOURS);

        while (row < TRIES && !won) {
            int k;
            render(row, cur, work, 0, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  cur = (cur + PEGS - 1) % PEGS;
            if (k == KEY_RIGHT) cur = (cur + 1) % PEGS;
            if (k == KEY_UP)    work[cur] = (work[cur] + 1) % COLOURS;
            if (k == KEY_DOWN)  work[cur] = (work[cur] + COLOURS - 1) % COLOURS;
            if (k != KEY_ENTER && k != ' ') continue;

            for (i = 0; i < PEGS; i++) guess[row][i] = work[i];
            feedback(work, &exact[row], &partial[row]);
            if (exact[row] == PEGS) won = 1;
            row++;
        }
        render(row, cur, work, 1,
               won ? "Code broken!" : "Out of guesses - the code is revealed.");
        if (won) score_report(p->title ? p->title : "mastermind", (TRIES - row + 1) * 10);
        if (!confirm("\n  Play again?")) return;
    }
}
