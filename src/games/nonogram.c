/* GIC:PARAMETERISED nonogram
 * nonogram.c - picross from 5x5 up to 30x30, mono or multi-colour.
 * params: size = grid edge, variant = 0 monochrome, 1 colour
 *
 * In colour mode each run clue carries its own colour, and a cell must be
 * painted in the right colour to count as solved.
 */
#include "engine.h"
#include "games.h"

#define MAXN 30
#define MAXCLUE 16
#define NCOLS 3

static int N, VARIANT;
static int sol[MAXN][MAXN];                     /* 0 blank, else colour 1..NCOLS */
static int mark[MAXN][MAXN];                    /* 0 blank, 1..NCOLS paint, -1 cross */
static int rowclue[MAXN][MAXCLUE], rowcol[MAXN][MAXCLUE], rown[MAXN];
static int colclue[MAXN][MAXCLUE], colcol[MAXN][MAXCLUE], coln[MAXN];
static const char *PAINT[NCOLS + 1] = {C_GREY, C_WHITE, C_CYAN, C_YELLOW};

static void runs(const int *line, int *out, int *cols, int *n)
{
    int i, run = 0, col = 0;
    *n = 0;
    for (i = 0; i < N; i++) {
        if (line[i] && line[i] == col) run++;
        else {
            if (run && *n < MAXCLUE) { out[*n] = run; cols[*n] = col; (*n)++; }
            run = line[i] ? 1 : 0;
            col = line[i];
        }
    }
    if (run && *n < MAXCLUE) { out[*n] = run; cols[*n] = col; (*n)++; }
    if (!*n) { out[0] = 0; cols[0] = 1; *n = 1; }
}

static void build_clues(void)
{
    int i, j, line[MAXN];
    for (i = 0; i < N; i++) runs(sol[i], rowclue[i], rowcol[i], &rown[i]);
    for (j = 0; j < N; j++) {
        for (i = 0; i < N; i++) line[i] = sol[i][j];
        runs(line, colclue[j], colcol[j], &coln[j]);
    }
}

static int correct(void)
{
    int i, j;
    for (i = 0; i < N; i++) for (j = 0; j < N; j++) {
        int want = sol[i][j];
        int got = mark[i][j] > 0 ? mark[i][j] : 0;
        if (want != got) return 0;
    }
    return 1;
}

static void render(int cr, int cc, int pen, const char *msg)
{
    int i, j, k, maxrow = 0, maxcol = 0, left, top;
    char buf[64];

    for (i = 0; i < N; i++) {
        int len = 0;
        for (k = 0; k < rown[i]; k++) len += (rowclue[i][k] > 9) ? 3 : 2;
        if (len > maxrow) maxrow = len;
        if (coln[i] > maxcol) maxcol = coln[i];
    }
    left = maxrow + 3;
    top = 5 + maxcol;

    {
        char sub[100];
        snprintf(sub, sizeof sub, "%dx%d %s - Enter paints, X crosses%s, Q quits",
                 N, N, VARIANT ? "colour" : "mono",
                 VARIANT ? ", 1-3 picks a colour" : "");
        draw_title("NONOGRAM", sub);
    }

    for (k = 0; k < maxcol; k++)
        for (j = 0; j < N; j++) {
            int idx = k - (maxcol - coln[j]);
            if (idx < 0) continue;
            scr_move(5 + k, left + j * 2);
            printf("%s%2d%s", PAINT[VARIANT ? colcol[j][idx] : 1], colclue[j][idx], C_RESET);
        }

    for (i = 0; i < N; i++) {
        buf[0] = '\0';
        for (k = 0; k < rown[i]; k++) {
            char n[8];
            snprintf(n, sizeof n, "%d ", rowclue[i][k]);
            strncat(buf, n, sizeof buf - strlen(buf) - 1);
        }
        scr_move(top + i, 1);
        printf("%s%*s%s", C_CYAN, left - 2, buf, C_RESET);
        for (j = 0; j < N; j++) {
            int sel = (i == cr && j == cc), m = mark[i][j];
            const char *bg = sel ? BG_BLUE : "";
            scr_move(top + i, left + j * 2);
            if (m > 0)       printf("%s%s##%s", bg, PAINT[m], C_RESET);
            else if (m < 0)  printf("%s%s x%s", bg, C_RED, C_RESET);
            else             printf("%s%s .%s", bg, C_GREY, C_RESET);
        }
    }
    if (VARIANT) {
        scr_move(top + N + 1, left);
        printf("Pen: ");
        for (k = 1; k <= NCOLS; k++)
            printf("%s%s##%s ", k == pen ? C_REV : "", PAINT[k], C_RESET);
    }
    draw_text(top + N + 3, 4, "                                                            ");
    if (msg) draw_textf(top + N + 3, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_nonogram(const GParams *p)
{
    N = gp_int(p->size, 10);
    if (N < 5) N = 5;
    if (N > MAXN) N = MAXN;
    VARIANT = p->variant ? 1 : 0;

    for (;;) {
        int i, j, cr = 0, cc = 0, pen = 1;
        long start;
        for (i = 0; i < N; i++) for (j = 0; j < N; j++) {
            if (rnd(100) < 55) sol[i][j] = VARIANT ? 1 + rnd(NCOLS) : 1;
            else               sol[i][j] = 0;
        }
        build_clues();
        memset(mark, 0, sizeof mark);
        start = now_ms();

        while (!correct()) {
            int k;
            render(cr, cc, pen, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < N - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < N - 1) cc++;
            if (VARIANT && k >= '1' && k <= '0' + NCOLS) pen = k - '0';
            if (k == KEY_ENTER || k == ' ')
                mark[cr][cc] = (mark[cr][cc] == pen) ? 0 : pen;
            if (k == 'x' || k == 'X') mark[cr][cc] = (mark[cr][cc] < 0) ? 0 : -1;
        }
        {
            int secs = (int)((now_ms() - start) / 1000);
            char m[64];
            snprintf(m, sizeof m, "Picture complete in %d:%02d.", secs / 60, secs % 60);
            render(cr, cc, pen, m);
            score_report(p->title ? p->title : "nonogram", N * N * 4 - secs);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
