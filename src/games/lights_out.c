/* GIC:PARAMETERISED lightsout
 * lights_out.c - toggle puzzles across board sizes and toggle patterns.
 *
 * params: size    = board edge, 3..8
 *         variant = 0 plus-shaped  1 diagonal  2 whole row and column
 *
 * Boards are scrambled from the solved state using legal presses, so every
 * puzzle generated is guaranteed solvable.
 */
#include "engine.h"
#include "games.h"

#define MAXN 8

static int N, VARIANT;
static int on[MAXN][MAXN];

static void toggle(int r, int c)
{
    int i, j;
    switch (VARIANT) {
        case 1: {                               /* diagonal cross */
            static const int DR[5] = {0,-1,-1,1,1};
            static const int DC[5] = {0,-1,1,-1,1};
            for (i = 0; i < 5; i++) {
                int a = r + DR[i], b = c + DC[i];
                if (a >= 0 && a < N && b >= 0 && b < N) on[a][b] = !on[a][b];
            }
            break;
        }
        case 2:                                 /* whole row and column */
            for (i = 0; i < N; i++) { on[r][i] = !on[r][i]; }
            for (j = 0; j < N; j++) { if (j != r) on[j][c] = !on[j][c]; }
            break;
        default: {                              /* plus */
            static const int DR[5] = {0,-1,1,0,0};
            static const int DC[5] = {0,0,0,-1,1};
            for (i = 0; i < 5; i++) {
                int a = r + DR[i], b = c + DC[i];
                if (a >= 0 && a < N && b >= 0 && b < N) on[a][b] = !on[a][b];
            }
            break;
        }
    }
}

static int all_off(void)
{
    int r, c;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (on[r][c]) return 0;
    return 1;
}

static void render(int cr, int cc, int moves, const char *msg)
{
    static const char *VNAME[3] = {"plus", "diagonal", "row and column"};
    int r, c, left = 40 - (N * 3) / 2;
    char sub[110];
    snprintf(sub, sizeof sub, "%dx%d, %s toggle - arrows move, Enter presses, Q quits",
             N, N, VNAME[VARIANT]);
    draw_title("LIGHTS OUT", sub);

    for (r = 0; r < N; r++) {
        scr_move(7 + r, left);
        for (c = 0; c < N; c++) {
            int sel = (r == cr && c == cc);
            if (on[r][c]) printf("%s%s%s O %s", sel ? C_REV : "", BG_YELLOW, C_BLACK, C_RESET);
            else          printf("%s%s . %s", sel ? C_REV : "", C_GREY, C_RESET);
        }
    }
    draw_textf(N + 9, left, "Moves: %d   ", moves);
    draw_text(N + 11, 16, "                                              ");
    if (msg) draw_textf(N + 11, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_lights_out(const GParams *p)
{
    N = gp_int(p->size, 5);
    if (N < 3) N = 3;
    if (N > MAXN) N = MAXN;
    VARIANT = (p->variant >= 0 && p->variant <= 2) ? p->variant : 0;

    for (;;) {
        int cr = N / 2, cc = N / 2, moves = 0, i;
        memset(on, 0, sizeof on);
        /* Scramble with legal presses so the puzzle is always solvable. */
        for (i = 0; i < N * N / 2 + 4; i++) toggle(rnd(N), rnd(N));
        if (all_off()) toggle(rnd(N), rnd(N));

        while (!all_off()) {
            int k;
            render(cr, cc, moves, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < N - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < N - 1) cc++;
            if (k == KEY_ENTER || k == ' ') { toggle(cr, cc); moves++; }
        }
        render(cr, cc, moves, "All lights out!");
        score_report(p->title ? p->title : "lights-out", N * N * 20 / (moves + 1));
        if (!confirm("\n  Play again?")) return;
    }
}
