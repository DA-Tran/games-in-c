/* GIC:PARAMETERISED sliding
 * fifteen.c - sliding tile puzzles from 3x3 to 7x7.
 * params: size = board edge (3..7)
 * Shuffled by walking the blank, so every deal is solvable.
 */
#include "engine.h"
#include "games.h"

#define MAXN 7
static int N;
static int t[MAXN][MAXN], er, ec;

static int slide(int dr, int dc)
{
    int r = er + dr, c = ec + dc;
    if (r < 0 || r >= N || c < 0 || c >= N) return 0;
    t[er][ec] = t[r][c];
    t[r][c] = 0;
    er = r; ec = c;
    return 1;
}

static int solved(void)
{
    int i;
    for (i = 0; i < N * N - 1; i++)
        if (t[i / N][i % N] != i + 1) return 0;
    return t[N-1][N-1] == 0;
}

static void render(int moves, const char *msg)
{
    int r, c, w = (N * N > 25) ? 4 : 3, left = 40 - (N * w) / 2;
    char sub[90];
    snprintf(sub, sizeof sub, "%dx%d - arrows slide tiles into the gap, Q quits", N, N);
    draw_title("SLIDING PUZZLE", sub);
    for (r = 0; r < N; r++) {
        scr_move(7 + r, left);
        for (c = 0; c < N; c++) {
            if (t[r][c]) printf("%s%s%*d %s", BG_BLUE, C_WHITE, w - 1, t[r][c], C_RESET);
            else         printf("%s%*s%s", C_GREY, w, "", C_RESET);
        }
    }
    draw_textf(N + 9, left, "Moves: %d   ", moves);
    draw_text(N + 11, 20, "                                        ");
    if (msg) draw_textf(N + 11, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_fifteen(const GParams *p)
{
    N = gp_int(p->size, 4);
    if (N < 3) N = 3;
    if (N > MAXN) N = MAXN;

    for (;;) {
        int i, moves = 0;
        for (i = 0; i < N * N; i++) t[i / N][i % N] = (i + 1) % (N * N);
        er = ec = N - 1;
        for (i = 0; i < N * N * 30; i++) {
            int d = rnd(4);
            slide(d == 0 ? -1 : d == 1 ? 1 : 0, d == 2 ? -1 : d == 3 ? 1 : 0);
        }
        if (solved()) slide(-1, 0);

        while (!solved()) {
            int k;
            render(moves, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP)    { if (slide(1, 0))  moves++; }
            if (k == KEY_DOWN)  { if (slide(-1, 0)) moves++; }
            if (k == KEY_LEFT)  { if (slide(0, 1))  moves++; }
            if (k == KEY_RIGHT) { if (slide(0, -1)) moves++; }
        }
        render(moves, "Solved!");
        score_report(p->title ? p->title : "sliding", N * N * 200 / (moves + 1));
        if (!confirm("\n  Play again?")) return;
    }
}
