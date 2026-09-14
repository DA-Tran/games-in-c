/* GIC:PARAMETERISED gomoku
 * gomoku.c - five-in-a-row on 9x9 to 19x19, free-style or Renju.
 *
 * params: size    = board edge (9, 13, 15 or 19)
 *         variant = 0 free-style, 1 Renju
 *
 * Renju restricts black only: an overline (six or more) and a double-four
 * are forbidden and lose the game immediately. Those are the two rules that
 * actually bite in casual play; the full professional rule set also bans
 * double-three, which is not enforced here.
 */
#include "engine.h"
#include "games.h"

#define MAXN 19
static int N, VARIANT;
static char bd[MAXN][MAXN];

static int count_dir(int r, int c, int dr, int dc, char p)
{
    int n = 0;
    r += dr; c += dc;
    while (r >= 0 && r < N && c >= 0 && c < N && bd[r][c] == p) {
        n++; r += dr; c += dc;
    }
    return n;
}

static int run_len(int r, int c, int i, char p)
{
    static const int D[4][2] = {{0,1},{1,0},{1,1},{1,-1}};
    return 1 + count_dir(r, c, D[i][0], D[i][1], p)
             + count_dir(r, c, -D[i][0], -D[i][1], p);
}

static int wins_at(int r, int c, char p)
{
    int i;
    for (i = 0; i < 4; i++) {
        int n = run_len(r, c, i, p);
        if (VARIANT == 1 && p == 'X') { if (n == 5) return 1; }
        else if (n >= 5) return 1;
    }
    return 0;
}

/* Renju forbids black an overline, or two fours made by one stone. */
static int renju_forbidden(int r, int c)
{
    int i, fours = 0;
    if (VARIANT != 1) return 0;
    for (i = 0; i < 4; i++) {
        int n = run_len(r, c, i, 'X');
        if (n >= 6) return 1;                  /* overline */
        if (n == 4) fours++;
    }
    return fours >= 2;                         /* double four */
}

/* Score a candidate square by the runs it would extend, for both sides. */
static long square_score(int r, int c, char p)
{
    static const int D[4][2] = {{0,1},{1,0},{1,1},{1,-1}};
    static const long VAL[6] = {0, 1, 30, 700, 12000, 500000};
    long s = 0;
    int i;
    for (i = 0; i < 4; i++) {
        int a = count_dir(r, c, D[i][0], D[i][1], p);
        int b = count_dir(r, c, -D[i][0], -D[i][1], p);
        int n = a + b + 1;
        int openA, openB, ra, ca, rb, cb;
        if (n > 5) n = 5;
        ra = r + D[i][0] * (a + 1); ca = c + D[i][1] * (a + 1);
        rb = r - D[i][0] * (b + 1); cb = c - D[i][1] * (b + 1);
        openA = (ra >= 0 && ra < N && ca >= 0 && ca < N && bd[ra][ca] == ' ');
        openB = (rb >= 0 && rb < N && cb >= 0 && cb < N && bd[rb][cb] == ' ');
        if (!openA && !openB && n < 5) continue;
        s += VAL[n] * (openA && openB ? 2 : 1);
    }
    return s;
}

static int near_stone(int r, int c)
{
    int i, j;
    for (i = -2; i <= 2; i++) for (j = -2; j <= 2; j++) {
        int a = r + i, b = c + j;
        if (a >= 0 && a < N && b >= 0 && b < N && bd[a][b] != ' ') return 1;
    }
    return 0;
}

static void ai_move(int *orow, int *ocol)
{
    int r, c, br = N/2, bc = N/2;
    long best = -1;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        long v;
        if (bd[r][c] != ' ' || !near_stone(r, c)) continue;
        /* Attack value plus a slight premium on blocking the human. */
        v = square_score(r, c, 'O') + (long)(square_score(r, c, 'X') * 1.1);
        if (v > best) { best = v; br = r; bc = c; }
    }
    bd[br][bc] = 'O';
    *orow = br; *ocol = bc;
}

static void render(int cr, int cc, const char *msg)
{
    int r, c, left = 40 - N;
    char sub[90];
    snprintf(sub, sizeof sub, "%dx%d %s - arrows move, Enter places, Q quits",
             N, N, VARIANT ? "Renju (black restricted)" : "free-style");
    draw_title("GOMOKU", sub);
    for (r = 0; r < N; r++) {
        scr_move(5 + r, left);
        for (c = 0; c < N; c++) {
            int sel = (r == cr && c == cc);
            const char *bg = sel ? BG_BLUE : "";
            if (bd[r][c] == 'X')      printf("%s%s● %s", bg, C_CYAN, C_RESET);
            else if (bd[r][c] == 'O') printf("%s%s● %s", bg, C_YELLOW, C_RESET);
            else                      printf("%s%s· %s", bg, C_GREY, C_RESET);
        }
    }
    draw_text(N + 7, 18, "                                                        ");
    if (msg) draw_textf(N + 7, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_gomoku(const GParams *p)
{
    N = gp_int(p->size, 15);
    if (N < 9) N = 9;
    if (N > MAXN) N = MAXN;
    VARIANT = p->variant ? 1 : 0;

    for (;;) {
        int cr = N/2, cc = N/2, r, c, over = 0, moves = 0;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) bd[r][c] = ' ';

        while (!over) {
            int k;
            render(cr, cc, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < N - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < N - 1) cc++;
            if (k != KEY_ENTER && k != ' ') continue;
            if (bd[cr][cc] != ' ') continue;

            bd[cr][cc] = 'X';
            moves++;
            if (renju_forbidden(cr, cc)) {
                render(cr, cc, "Forbidden move under Renju - you lose.");
                over = 1;
                break;
            }
            if (wins_at(cr, cc, 'X')) {
                render(cr, cc, "You win!");
                score_report(p->title ? p->title : "gomoku", 1000 / (moves + 1) * 10);
                over = 1;
                break;
            }
            if (moves * 2 >= N * N)   { render(cr, cc, "Draw.");    over = 1; break; }
            {
                int ar, ac;
                ai_move(&ar, &ac);
                moves++;
                if (wins_at(ar, ac, 'O')) { render(cr, cc, "Computer wins."); over = 1; }
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
