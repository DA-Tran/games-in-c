/* GIC:PARAMETERISED sudoku
 * sudoku.c - Latin-square puzzles across five sizes and six rule variants.
 *
 * params: size    = 4, 6, 9, 12 or 16 (box shape derived from the size)
 *         variant = 0 classic    1 diagonal     2 even-odd
 *                   3 consecutive 4 anti-knight 5 windoku
 *         difficulty = 1..5, drives how many cells are removed
 *
 * Diagonal, anti-knight and windoku are true generation constraints and are
 * enforced during solving. Even-odd and consecutive are presented as extra
 * clues derived from the finished grid, which is how they are normally set.
 */
#include "engine.h"
#include "games.h"

#define MAXN 16

static int N, BW, BH, VARIANT;
static int sol[MAXN][MAXN], grid[MAXN][MAXN], given[MAXN][MAXN];

/* Box shape for each supported size. */
static void box_dims(int n, int *bw, int *bh)
{
    switch (n) {
        case 4:  *bw = 2; *bh = 2; break;
        case 6:  *bw = 3; *bh = 2; break;
        case 12: *bw = 4; *bh = 3; break;
        case 16: *bw = 4; *bh = 4; break;
        default: *bw = 3; *bh = 3; break;      /* 9 */
    }
}

/* The four shaded windoku regions, expressed for a 9x9 board. */
static int windoku_region(int r, int c)
{
    if (N != 9) return -1;
    if (r >= 1 && r <= 3 && c >= 1 && c <= 3) return 0;
    if (r >= 1 && r <= 3 && c >= 5 && c <= 7) return 1;
    if (r >= 5 && r <= 7 && c >= 1 && c <= 3) return 2;
    if (r >= 5 && r <= 7 && c >= 5 && c <= 7) return 3;
    return -1;
}

static int knight_clash(int g[MAXN][MAXN], int r, int c, int v)
{
    static const int DR[8] = {-2,-2,-1,-1,1,1,2,2};
    static const int DC[8] = {-1,1,-2,2,-2,2,-1,1};
    int i;
    for (i = 0; i < 8; i++) {
        int a = r + DR[i], b = c + DC[i];
        if (a < 0 || a >= N || b < 0 || b >= N) continue;
        if (g[a][b] == v) return 1;
    }
    return 0;
}

static int ok_at(int g[MAXN][MAXN], int r, int c, int v)
{
    int i, j, br, bc;
    for (i = 0; i < N; i++) {
        if (i != c && g[r][i] == v) return 0;
        if (i != r && g[i][c] == v) return 0;
    }
    br = (r / BH) * BH;
    bc = (c / BW) * BW;
    for (i = br; i < br + BH; i++)
        for (j = bc; j < bc + BW; j++)
            if ((i != r || j != c) && g[i][j] == v) return 0;

    if (VARIANT == 1) {                       /* diagonal */
        if (r == c)
            for (i = 0; i < N; i++) if (i != r && g[i][i] == v) return 0;
        if (r + c == N - 1)
            for (i = 0; i < N; i++) if (i != r && g[i][N - 1 - i] == v) return 0;
    }
    if (VARIANT == 4 && knight_clash(g, r, c, v)) return 0;
    if (VARIANT == 5) {                       /* windoku */
        int reg = windoku_region(r, c);
        if (reg >= 0)
            for (i = 0; i < N; i++) for (j = 0; j < N; j++)
                if ((i != r || j != c) && windoku_region(i, j) == reg && g[i][j] == v)
                    return 0;
    }
    return 1;
}

/* Backtracking fill that always expands the most constrained empty cell
 * first (minimum remaining values). Plain positional order sends the extra
 * constraints - diagonal especially - down enormous dead ends on 12x12 and
 * 16x16; MRV prunes those immediately and makes every variant fast. */
static long fill_budget;

static int fill(int g[MAXN][MAXN])
{
    int r, c, br = -1, bc = -1, bestn = MAXN + 1;
    int cand[MAXN], n = 0, i, v;

    if (--fill_budget <= 0) return 0;

    for (r = 0; r < N && bestn > 1; r++)
        for (c = 0; c < N; c++) {
            int cnt = 0;
            if (g[r][c]) continue;
            for (v = 1; v <= N; v++) if (ok_at(g, r, c, v)) cnt++;
            if (cnt == 0) return 0;            /* dead end, back out now */
            if (cnt < bestn) { bestn = cnt; br = r; bc = c; }
            if (cnt == 1) break;
        }
    if (br < 0) return 1;                      /* nothing empty: solved */

    for (v = 1; v <= N; v++) if (ok_at(g, br, bc, v)) cand[n++] = v;
    shuffle_int(cand, n);
    for (i = 0; i < n; i++) {
        g[br][bc] = cand[i];
        if (fill(g)) return 1;
        g[br][bc] = 0;
        if (fill_budget <= 0) return 0;
    }
    return 0;
}

static void fill_restarting(int g[MAXN][MAXN])
{
    int attempt;
    for (attempt = 0; attempt < 60; attempt++) {
        memset(g, 0, sizeof(int) * MAXN * MAXN);
        fill_budget = 200000;
        if (fill(g)) return;
    }
    /* Fall back to the plain Latin-square constraints, which always fill. */
    VARIANT = 0;
    memset(g, 0, sizeof(int) * MAXN * MAXN);
    fill_budget = 4000000;
    fill(g);
}

/* Count solutions with a node budget so large boards stay responsive.
 * Returns 2 for "several", or -1 if the budget ran out (treated as
 * not-provably-unique, which keeps the removal step conservative). */
static int count_solutions(int g[MAXN][MAXN], int pos, int found, long *budget)
{
    int r, c, v;
    if (--(*budget) <= 0) return -1;
    if (found >= 2) return found;
    if (pos == N * N) return found + 1;
    r = pos / N; c = pos % N;
    if (g[r][c]) return count_solutions(g, pos + 1, found, budget);
    for (v = 1; v <= N; v++) {
        if (!ok_at(g, r, c, v)) continue;
        g[r][c] = v;
        found = count_solutions(g, pos + 1, found, budget);
        g[r][c] = 0;
        if (found < 0 || found >= 2) break;
    }
    return found;
}

static void generate(int holes)
{
    int order[MAXN * MAXN], i, removed = 0;
    fill_restarting(sol);
    memcpy(grid, sol, sizeof sol);

    for (i = 0; i < N * N; i++) order[i] = i;
    shuffle_int(order, N * N);

    for (i = 0; i < N * N && removed < holes; i++) {
        int r = order[i] / N, c = order[i] % N, keep = grid[r][c];
        int work[MAXN][MAXN];
        /* Uniqueness proving is exponential, so give the big boards a
         * smaller budget: an inconclusive answer keeps the clue, which is
         * the safe direction. */
        long budget = (N > 9) ? 12000 : 60000;
        grid[r][c] = 0;
        memcpy(work, grid, sizeof grid);
        if (count_solutions(work, 0, 0, &budget) != 1) grid[r][c] = keep;
        else removed++;
    }
    for (i = 0; i < N * N; i++)
        given[i / N][i % N] = grid[i / N][i % N] != 0;
}

static int complete(void)
{
    int r, c;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++)
        if (grid[r][c] != sol[r][c]) return 0;
    return 1;
}

static char digit_char(int v)
{
    if (v <= 0) return '.';
    return (v <= 9) ? (char)('0' + v) : (char)('A' + v - 10);
}

static void render(int cr, int cc, int hints, const char *msg)
{
    static const char *VNAME[6] = {"Classic", "Diagonal", "Even-Odd",
                                   "Consecutive", "Anti-Knight", "Windoku"};
    int r, c, cw = (N > 9) ? 3 : 2;
    int left = 40 - (N * cw) / 2;
    char sub[120];

    snprintf(sub, sizeof sub, "%dx%d %s - arrows move, %s enters, 0 clears, H hints, Q quits",
             N, N, VNAME[VARIANT], (N > 9) ? "1-9 and A-G" : "1-9");
    draw_title("SUDOKU", sub);

    for (r = 0; r < N; r++) {
        int y = 6 + r + (r / BH);
        for (c = 0; c < N; c++) {
            int x = left + c * cw + (c / BW);
            int v = grid[r][c];
            const char *fg;
            const char *bg = (r == cr && c == cc) ? BG_BLUE : "";
            if (given[r][c])                       fg = C_WHITE;
            else if (v && !ok_at(grid, r, c, v))   fg = C_RED;
            else                                   fg = C_CYAN;
            /* Even-odd shading and windoku regions are drawn as clue tints. */
            if (!bg[0] && VARIANT == 2 && sol[r][c] % 2 == 0) bg = BG_BLACK;
            if (!bg[0] && VARIANT == 5 && windoku_region(r, c) >= 0) bg = BG_BLACK;
            scr_move(y, x);
            printf("%s%s %c %s", bg, v ? fg : C_GREY, digit_char(v), C_RESET);
        }
        /* Consecutive markers sit between horizontally adjacent cells. */
        if (VARIANT == 3)
            for (c = 0; c + 1 < N; c++) {
                int d = sol[r][c] - sol[r][c + 1];
                if (d == 1 || d == -1)
                    draw_textf(y, left + c * cw + (c / BW) + cw, "%s.%s", C_YELLOW, C_RESET);
            }
    }
    draw_textf(6 + N + BH + 1, left, "Hints used: %d    ", hints);
    draw_text(6 + N + BH + 3, 16, "                                                        ");
    if (msg) draw_textf(6 + N + BH + 3, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_sudoku(const GParams *p)
{
    static const int HOLE_PCT[6] = {0, 35, 44, 50, 56, 62};
    int tier = p->difficulty >= 1 && p->difficulty <= 5 ? p->difficulty : 3;

    N = gp_int(p->size, 9);
    if (N != 4 && N != 6 && N != 9 && N != 12 && N != 16) N = 9;
    VARIANT = (p->variant >= 0 && p->variant <= 5) ? p->variant : 0;
    box_dims(N, &BW, &BH);
    /* Windoku only has its extra regions defined on the 9x9 board. */
    if (VARIANT == 5 && N != 9) VARIANT = 0;

    for (;;) {
        int cr = 0, cc = 0, hints = 0;
        long start;

        draw_title("SUDOKU", "Generating a puzzle with a unique solution...");
        scr_flush();
        generate(N * N * HOLE_PCT[tier] / 100);
        start = now_ms();

        while (!complete()) {
            int k;
            render(cr, cc, hints, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < N - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < N - 1) cc++;
            if ((k == 'h' || k == 'H') && !given[cr][cc]) {
                grid[cr][cc] = sol[cr][cc];
                hints++;
                continue;
            }
            if (given[cr][cc]) continue;
            if (k >= '1' && k <= '9' && k - '0' <= N) grid[cr][cc] = k - '0';
            if (N > 9) {
                int up = toupper(k);
                if (up >= 'A' && up <= 'A' + N - 11) grid[cr][cc] = up - 'A' + 10;
            }
            if (k == '0' || k == ' ') grid[cr][cc] = 0;
        }
        {
            int secs = (int)((now_ms() - start) / 1000);
            char m[96];
            snprintf(m, sizeof m, "Solved in %d:%02d with %d hint%s.",
                     secs / 60, secs % 60, hints, hints == 1 ? "" : "s");
            render(cr, cc, hints, m);
            score_report(p->title ? p->title : "sudoku", tier * 1000 - secs - hints * 50);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
