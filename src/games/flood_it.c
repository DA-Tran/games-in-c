/* GIC:PARAMETERISED floodit
 * flood_it.c - claim the board by recolouring from the top-left corner.
 * params: size = board edge, count = palette size, level = move budget
 */
#include "engine.h"
#include "games.h"

#define MAXN 18
static int N, COLOURS, MOVES;
static int g[MAXN][MAXN];
static const char *CCOL[8] = {C_RED, C_GREEN, C_BLUE, C_YELLOW,
                              C_MAGENTA, C_CYAN, C_WHITE, C_GREY};

static void flood(int r, int c, int from, int to)
{
    int stack[MAXN * MAXN][2], sp = 0;
    stack[sp][0] = r; stack[sp][1] = c; sp++;
    while (sp > 0) {
        int y, x;
        sp--;
        y = stack[sp][0]; x = stack[sp][1];
        if (y < 0 || y >= N || x < 0 || x >= N || g[y][x] != from) continue;
        g[y][x] = to;
        stack[sp][0] = y+1; stack[sp][1] = x; sp++;
        stack[sp][0] = y-1; stack[sp][1] = x; sp++;
        stack[sp][0] = y; stack[sp][1] = x+1; sp++;
        stack[sp][0] = y; stack[sp][1] = x-1; sp++;
    }
}

static int owned(void)
{
    static int seen[MAXN][MAXN];
    int stack[MAXN * MAXN][2], sp = 0, n = 0, col = g[0][0];
    static const int DR[4] = {1,-1,0,0}, DC[4] = {0,0,1,-1};
    memset(seen, 0, sizeof seen);
    stack[sp][0] = 0; stack[sp][1] = 0; sp++;
    seen[0][0] = 1;
    while (sp > 0) {
        int r, c, d;
        sp--;
        r = stack[sp][0]; c = stack[sp][1];
        n++;
        for (d = 0; d < 4; d++) {
            int a = r + DR[d], b = c + DC[d];
            if (a < 0 || a >= N || b < 0 || b >= N) continue;
            if (seen[a][b] || g[a][b] != col) continue;
            seen[a][b] = 1;
            stack[sp][0] = a; stack[sp][1] = b; sp++;
        }
    }
    return n;
}

static void render(int cur, int left, const char *msg)
{
    int r, c, x0 = 40 - N;
    char sub[90];
    snprintf(sub, sizeof sub, "%dx%d, %d colours, %d moves - Left/Right pick, Enter floods",
             N, N, COLOURS, MOVES);
    draw_title("FLOOD IT", sub);
    for (r = 0; r < N; r++) {
        scr_move(6 + r, x0);
        for (c = 0; c < N; c++) printf("%s##%s", CCOL[g[r][c]], C_RESET);
    }
    scr_move(N + 8, x0);
    printf("Colour: ");
    for (c = 0; c < COLOURS; c++)
        printf("%s%s##%s", c == cur ? C_REV : "", CCOL[c], C_RESET);
    draw_textf(N + 9, x0, "Moves left: %2d   Filled: %d/%d   ", left, owned(), N * N);
    draw_text(N + 11, 18, "                                              ");
    if (msg) draw_textf(N + 11, x0, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_flood_it(const GParams *p)
{
    N = gp_int(p->size, 12);
    if (N < 4) N = 4;
    if (N > MAXN) N = MAXN;
    COLOURS = gp_int(p->count, 6);
    if (COLOURS < 3) COLOURS = 3;
    if (COLOURS > 8) COLOURS = 8;
    MOVES = gp_int(p->level, 22);

    for (;;) {
        int r, c, cur = 0, left = MOVES, over = 0;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) g[r][c] = rnd(COLOURS);

        while (!over) {
            int k;
            render(cur, left, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  cur = (cur + COLOURS - 1) % COLOURS;
            if (k == KEY_RIGHT) cur = (cur + 1) % COLOURS;
            if (k != KEY_ENTER && k != ' ') continue;
            if (cur == g[0][0]) continue;
            flood(0, 0, g[0][0], cur);
            left--;
            if (owned() == N * N) { render(cur, left, "Board flooded - you win!"); over = 1; }
            else if (left == 0)   { render(cur, left, "Out of moves.");            over = 1; }
        }
        if (owned() == N * N) score_report(p->title ? p->title : "flood-it", left * 50);
        if (!confirm("\n  Play again?")) return;
    }
}
