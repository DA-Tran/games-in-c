/* GIC:PARAMETERISED merge
 * g2048.c - sliding merge puzzles.
 * params: size    = grid edge (3..8)
 *         variant = 0 doubling (2048)  1 tripling  2 Fibonacci  3 Threes
 */
#include "engine.h"
#include "games.h"

#define MAXN 8
static int N, VARIANT, score;
static int g[MAXN][MAXN];

/* Can two tiles combine, and into what? Rules differ per variant. */
static int combine(int a, int b, int *out)
{
    if (!a || !b) return 0;
    switch (VARIANT) {
        case 1:                                   /* tripling: needs 3 in a row */
            return 0;                             /* handled in slide_line */
        case 2: {                                 /* Fibonacci neighbours merge */
            int x = a < b ? a : b, y = a < b ? b : a;
            int f0 = 1, f1 = 1;
            while (f1 < x) { int t = f0 + f1; f0 = f1; f1 = t; }
            if (f1 != x) return 0;
            if (f0 + f1 == y || (x == y && x == 1)) { *out = x + y; return 1; }
            return 0;
        }
        case 3:                                   /* Threes: 1+2, then equals */
            if ((a == 1 && b == 2) || (a == 2 && b == 1)) { *out = 3; return 1; }
            if (a == b && a >= 3) { *out = a + b; return 1; }
            return 0;
        default:
            if (a == b) { *out = a + b; return 1; }
            return 0;
    }
}

static int slide_line(int *line)
{
    int tmp[MAXN], n = 0, i, moved = 0;
    for (i = 0; i < N; i++) if (line[i]) tmp[n++] = line[i];
    for (i = n; i < N; i++) tmp[i] = 0;

    if (VARIANT == 1) {                           /* three identical tiles merge */
        for (i = 0; i + 2 < N; i++) {
            if (tmp[i] && tmp[i] == tmp[i+1] && tmp[i+1] == tmp[i+2]) {
                int j;
                tmp[i] *= 3;
                score += tmp[i];
                for (j = i + 1; j < N - 2; j++) tmp[j] = tmp[j + 2];
                tmp[N-1] = tmp[N-2] = 0;
            }
        }
    } else {
        for (i = 0; i + 1 < N; i++) {
            int merged;
            if (combine(tmp[i], tmp[i+1], &merged)) {
                int j;
                tmp[i] = merged;
                score += merged;
                for (j = i + 1; j < N - 1; j++) tmp[j] = tmp[j + 1];
                tmp[N-1] = 0;
            }
        }
    }
    for (i = 0; i < N; i++) {
        if (line[i] != tmp[i]) moved = 1;
        line[i] = tmp[i];
    }
    return moved;
}

static int move_dir(int dir)
{
    int i, j, moved = 0, line[MAXN];
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            switch (dir) {
                case 0: line[j] = g[i][j];        break;
                case 1: line[j] = g[i][N-1-j];    break;
                case 2: line[j] = g[j][i];        break;
                default: line[j] = g[N-1-j][i];   break;
            }
        }
        if (slide_line(line)) moved = 1;
        for (j = 0; j < N; j++) {
            switch (dir) {
                case 0: g[i][j]      = line[j]; break;
                case 1: g[i][N-1-j]  = line[j]; break;
                case 2: g[j][i]      = line[j]; break;
                default: g[N-1-j][i] = line[j]; break;
            }
        }
    }
    return moved;
}

static void spawn(void)
{
    int free_cells[MAXN * MAXN][2], n = 0, r, c;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++)
        if (!g[r][c]) { free_cells[n][0] = r; free_cells[n][1] = c; n++; }
    if (!n) return;
    r = rnd(n);
    if (VARIANT == 3)      g[free_cells[r][0]][free_cells[r][1]] = rnd(3) + 1;
    else if (VARIANT == 2) g[free_cells[r][0]][free_cells[r][1]] = 1;
    else if (VARIANT == 1) g[free_cells[r][0]][free_cells[r][1]] = 3;
    else                   g[free_cells[r][0]][free_cells[r][1]] = (rnd(10) == 0) ? 4 : 2;
}

static int can_move(void)
{
    int r, c, save[MAXN][MAXN], d;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (!g[r][c]) return 1;
    memcpy(save, g, sizeof g);
    for (d = 0; d < 4; d++) {
        int m;
        memcpy(g, save, sizeof g);
        m = move_dir(d);
        memcpy(g, save, sizeof g);
        if (m) return 1;
    }
    return 0;
}

static const char *tile_colour(int v)
{
    switch (v) {
        case 1: case 2:  return C_WHITE;
        case 3: case 4:  return C_CYAN;
        case 6: case 8:  return C_GREEN;
        case 12: case 16:return C_YELLOW;
        case 24: case 32:return C_MAGENTA;
        case 48: case 64:return C_RED;
        default:         return C_BOLD C_YELLOW;
    }
}

void fam_2048(const GParams *p)
{
    static const char *VNAME[4] = {"doubling", "tripling", "Fibonacci", "Threes"};
    char sub[90];

    N = gp_int(p->size, 4);
    if (N < 3) N = 3;
    if (N > MAXN) N = MAXN;
    VARIANT = (p->variant >= 0 && p->variant <= 3) ? p->variant : 0;
    snprintf(sub, sizeof sub, "%dx%d, %s merges - arrows slide, Q quits", N, N, VNAME[VARIANT]);

    for (;;) {
        int r, c;
        memset(g, 0, sizeof g);
        score = 0;
        spawn(); spawn();

        for (;;) {
            int k, moved = 0, left = 40 - (N * 6) / 2;
            draw_title("MERGE", sub);
            for (r = 0; r < N; r++) {
                scr_move(7 + r * 2, left);
                for (c = 0; c < N; c++) {
                    if (g[r][c]) printf("%s%6d%s", tile_colour(g[r][c]), g[r][c], C_RESET);
                    else         printf("%s     .%s", C_GREY, C_RESET);
                }
            }
            draw_textf(N * 2 + 8, left, "Score: %-8d Best: %d   ",
                       score, score_load(p->title ? p->title : "merge"));
            scr_flush();
            if (!can_move()) break;
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) {
                score_save(p->title ? p->title : "merge", score);
                return;
            }
            if (k == KEY_LEFT)  moved = move_dir(0);
            if (k == KEY_RIGHT) moved = move_dir(1);
            if (k == KEY_UP)    moved = move_dir(2);
            if (k == KEY_DOWN)  moved = move_dir(3);
            if (moved) spawn();
        }
        draw_centered(N * 2 + 10, 80, C_BOLD C_RED "No moves left." C_RESET);
        score_report(p->title ? p->title : "merge", score);
        if (!confirm("\n  Play again?")) return;
    }
}
