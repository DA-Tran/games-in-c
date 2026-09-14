/* GIC:PARAMETERISED reversi
 * reversi.c - Othello on any even board from 4x4 to 12x12.
 * params: size = board edge (even, 4..12)
 * Corner and edge weights are derived from the board size rather than
 * hard-coded, so the positional AI stays sensible at every size.
 */
#include "engine.h"
#include "games.h"

#define MAXN 12
static int N;
static char bd[MAXN][MAXN];
static const int DX[8] = {-1,-1,-1,0,0,1,1,1};
static const int DY[8] = {-1,0,1,-1,1,-1,0,1};

/* Corners are worth far more than the squares that give them away, and the
 * squares diagonally inside a corner are the worst on the board. */
static int weight_at(int r, int c)
{
    int er = (r == 0 || r == N - 1);
    int ec = (c == 0 || c == N - 1);
    int nr = (r == 1 || r == N - 2);
    int nc = (c == 1 || c == N - 2);
    if (er && ec) return 120;
    if ((er && nc) || (nr && ec)) return -20;
    if (nr && nc) return -40;
    if (er || ec) return 20;
    if (nr || nc) return -5;
    return 3;
}

static int flips(int r, int c, char p, int apply)
{
    char o = (p == 'X') ? 'O' : 'X';
    int total = 0, d;
    if (bd[r][c] != ' ') return 0;
    for (d = 0; d < 8; d++) {
        int i = r + DX[d], j = c + DY[d], n = 0;
        while (i >= 0 && i < N && j >= 0 && j < N && bd[i][j] == o) {
            i += DX[d]; j += DY[d]; n++;
        }
        if (n && i >= 0 && i < N && j >= 0 && j < N && bd[i][j] == p) {
            total += n;
            if (apply) {
                i = r + DX[d]; j = c + DY[d];
                while (bd[i][j] == o) { bd[i][j] = p; i += DX[d]; j += DY[d]; }
            }
        }
    }
    if (apply && total) bd[r][c] = p;
    return total;
}

static int has_move(char p)
{
    int r, c;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++)
        if (flips(r, c, p, 0)) return 1;
    return 0;
}

static void counts(int *x, int *o)
{
    int r, c;
    *x = *o = 0;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (bd[r][c] == 'X') (*x)++;
        else if (bd[r][c] == 'O') (*o)++;
    }
}

static void ai_move(void)
{
    int r, c, best = -100000, br = -1, bc = -1;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        int f = flips(r, c, 'O', 0), v;
        if (!f) continue;
        v = f + weight_at(r, c);
        if (v > best) { best = v; br = r; bc = c; }
    }
    if (br >= 0) flips(br, bc, 'O', 1);
}

static void render(int cr, int cc, const char *msg)
{
    int r, c, x, o, left = 40 - (N * 3) / 2;
    counts(&x, &o);
    {
        char sub[80];
        snprintf(sub, sizeof sub, "%dx%d - arrows move, Enter places, Q quits", N, N);
        draw_title("REVERSI", sub);
    }
    {
        int cc2;
        scr_move(6, left);
        printf("  ");
        for (cc2 = 0; cc2 < N; cc2++) printf(" %c ", 'a' + cc2);
    }
    for (r = 0; r < N; r++) {
        scr_move(7 + r, left);
        printf("%2d", r + 1);
        for (c = 0; c < N; c++) {
            int sel = (r == cr && c == cc);
            const char *bgc = sel ? BG_BLUE : "";
            if (bd[r][c] == 'X')      printf("%s%s ● %s", bgc, C_CYAN, C_RESET);
            else if (bd[r][c] == 'O') printf("%s%s ● %s", bgc, C_YELLOW, C_RESET);
            else if (flips(r, c, 'X', 0)) printf("%s%s ⁘ %s", bgc, C_GREEN, C_RESET);
            else                      printf("%s%s · %s", bgc, C_GREY, C_RESET);
        }
    }
    draw_textf(N + 9, left, "%sYou %d%s   %sCPU %d%s        ", C_CYAN, x, C_RESET, C_YELLOW, o, C_RESET);
    draw_text(N + 11, 20, "                                        ");
    if (msg) draw_textf(N + 11, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_reversi(const GParams *p)
{
    N = gp_int(p->size, 8);
    if (N < 4) N = 4;
    if (N > MAXN) N = MAXN;
    if (N % 2) N++;                       /* the opening needs an even board */

    for (;;) {
        int cr = N / 2 - 1, cc = N / 2 - 1, r, c, over = 0, h = N / 2;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) bd[r][c] = ' ';
        bd[h-1][h-1] = bd[h][h] = 'O';
        bd[h-1][h]   = bd[h][h-1] = 'X';

        while (!over) {
            int k;
            if (!has_move('X') && !has_move('O')) break;
            if (!has_move('X')) {
                render(cr, cc, "No moves - you pass.");
                sleep_ms(900);
                ai_move();
                continue;
            }
            render(cr, cc, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < N - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < N - 1) cc++;
            if (k != KEY_ENTER && k != ' ') continue;
            if (!flips(cr, cc, 'X', 0)) continue;
            flips(cr, cc, 'X', 1);
            while (has_move('O') && !has_move('X')) ai_move();
            if (has_move('O')) ai_move();
        }
        {
            int x, o;
            counts(&x, &o);
            render(cr, cc, x > o ? "You win!" : x < o ? "Computer wins." : "Draw.");
            score_report(p->title ? p->title : "reversi", x);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
