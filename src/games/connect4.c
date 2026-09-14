/* GIC:PARAMETERISED connect
 * connect4.c - the drop-and-connect family.
 *
 * params: width, height = board size
 *         count         = how many in a row wins
 *         variant       = 0 plain, 1 pop-out
 *
 * Pop-out is a real rules change rather than a bigger board: on your turn you
 * may instead remove one of your own discs from the bottom row, dropping the
 * whole column by one. That can complete a line for either player at once, so
 * the win test has to scan the whole board after such a move.
 */
#include "engine.h"
#include "games.h"

#define MAXW 13
#define MAXH 10

static int W, H, K, POPOUT;
static char bd[MAXH][MAXW];

static int drop(int col, char p)
{
    int r;
    for (r = H - 1; r >= 0; r--)
        if (bd[r][col] == ' ') { bd[r][col] = p; return r; }
    return -1;
}

/* Remove the bottom disc of a column; everything above slides down one. */
static void popout(int col)
{
    int r;
    for (r = H - 1; r > 0; r--) bd[r][col] = bd[r - 1][col];
    bd[0][col] = ' ';
}

static int wins(char p)
{
    static const int DR[4] = {0, 1, 1, 1};
    static const int DC[4] = {1, 0, 1, -1};
    int r, c, d, i;
    for (r = 0; r < H; r++)
        for (c = 0; c < W; c++) {
            if (bd[r][c] != p) continue;
            for (d = 0; d < 4; d++) {
                int rr = r + DR[d] * (K - 1), cc = c + DC[d] * (K - 1);
                if (rr < 0 || rr >= H || cc < 0 || cc >= W) continue;
                for (i = 1; i < K; i++)
                    if (bd[r + DR[d] * i][c + DC[d] * i] != p) break;
                if (i == K) return 1;
            }
        }
    return 0;
}

static int full(void)
{
    int c;
    for (c = 0; c < W; c++) if (bd[0][c] == ' ') return 0;
    return 1;
}

/* Score one K-long window by how close each side is to filling it. */
static int score_window(int r, int c, int dr, int dc)
{
    int me = 0, op = 0, sp = 0, i;
    for (i = 0; i < K; i++) {
        char v = bd[r + dr * i][c + dc * i];
        if (v == 'O') me++; else if (v == 'X') op++; else sp++;
    }
    if (me && op) return 0;
    if (me && sp) return me * me * 4;
    if (op && sp) return -(op * op * 5);
    return 0;
}

static int evaluate(void)
{
    static const int DR[4] = {0, 1, 1, 1};
    static const int DC[4] = {1, 0, 1, -1};
    int r, c, d, s = 0;
    for (r = 0; r < H; r++)
        for (c = 0; c < W; c++)
            for (d = 0; d < 4; d++) {
                int rr = r + DR[d] * (K - 1), cc = c + DC[d] * (K - 1);
                if (rr < 0 || rr >= H || cc < 0 || cc >= W) continue;
                s += score_window(r, c, DR[d], DC[d]);
            }
    for (r = 0; r < H; r++)
        s += (bd[r][W / 2] == 'O') ? 6 : (bd[r][W / 2] == 'X' ? -6 : 0);
    return s;
}

static int negamax(int depth, int alpha, int beta, int ai)
{
    int c, best;
    if (wins('O')) return 100000 + depth;
    if (wins('X')) return -100000 - depth;
    if (full() || depth == 0) return evaluate();

    best = ai ? -1000000 : 1000000;
    for (c = 0; c < W; c++) {
        int r, v;
        if (bd[0][c] != ' ') continue;
        r = drop(c, ai ? 'O' : 'X');
        v = negamax(depth - 1, alpha, beta, !ai);
        bd[r][c] = ' ';
        if (ai) { if (v > best) best = v; if (best > alpha) alpha = best; }
        else    { if (v < best) best = v; if (best < beta)  beta  = best; }
        if (alpha >= beta) break;
    }
    return best;
}

/* Returns the column, or -(col+1) to mean "pop that column out". */
static int ai_move(void)
{
    int c, best = -1000000, mv = 0, depth;
    char save[MAXH][MAXW];

    /* A wider board with a longer target needs a shallower search to stay
     * responsive; 7x6 connect-4 can afford to look further. */
    depth = (W * H <= 42) ? 5 : (W * H <= 72) ? 4 : 3;

    for (c = 0; c < W; c++) {
        int r, v;
        if (bd[0][c] != ' ') continue;
        r = drop(c, 'O');
        v = negamax(depth - 1, -1000000, 1000000, 0) - (c == W / 2 ? 0 : 1);
        bd[r][c] = ' ';
        if (v > best) { best = v; mv = c; }
    }
    if (POPOUT) {
        for (c = 0; c < W; c++) {
            int v;
            if (bd[H - 1][c] != 'O') continue;
            memcpy(save, bd, sizeof bd);
            popout(c);
            v = wins('O') ? 200000 : wins('X') ? -200000
                : negamax(depth - 1, -1000000, 1000000, 0);
            memcpy(bd, save, sizeof bd);
            if (v > best) { best = v; mv = -(c + 1); }
        }
    }
    return mv;
}

static void render(int cur, int popmode, const char *msg)
{
    int r, c, left = 40 - (W * 3) / 2;
    char sub[96];
    snprintf(sub, sizeof sub, "Connect %d%s — Left/Right aim, Enter %s%s",
             K, POPOUT ? ", pop-out allowed" : "",
             popmode ? "POPS" : "drops",
             POPOUT ? ", Tab toggles" : ", Q quits");
    draw_title("CONNECT", sub);
    draw_text(6, left - 2, "                                          ");
    draw_textf(6, left + cur * 3, "%s %c %s", popmode ? C_RED : C_CYAN,
               popmode ? '^' : 'v', C_RESET);
    for (r = 0; r < H; r++) {
        scr_move(7 + r, left - 1);
        for (c = 0; c < W; c++) {
            char p = bd[r][c];
            if (p == 'X')      printf("%s O %s", C_CYAN, C_RESET);
            else if (p == 'O') printf("%s O %s", C_YELLOW, C_RESET);
            else               printf("%s . %s", C_GREY, C_RESET);
        }
    }
    draw_text(9 + H, left - 4, "                                        ");
    if (msg) draw_textf(9 + H, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_connect4(const GParams *p)
{
    W = gp_int(p->width, 7);
    H = gp_int(p->height, 6);
    K = gp_int(p->count, 4);
    POPOUT = gp_int(p->variant, 0) == 1;
    if (W < 4) W = 4;
    if (W > MAXW) W = MAXW;
    if (H < 4) H = 4;
    if (H > MAXH) H = MAXH;
    if (K < 3) K = 3;
    if (K > W && K > H) K = W < H ? W : H;

    for (;;) {
        int cur = W / 2, over = 0, r, c, popmode = 0;
        for (r = 0; r < H; r++) for (c = 0; c < W; c++) bd[r][c] = ' ';

        while (!over) {
            int k, m;
            render(cur, popmode, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT  && cur > 0)     cur--;
            if (k == KEY_RIGHT && cur < W - 1) cur++;
            if (k == '\t' && POPOUT) popmode = !popmode;
            if (k != KEY_ENTER && k != ' ') continue;

            if (popmode) {
                if (bd[H - 1][cur] != 'X') continue;
                popout(cur);
            } else {
                if (bd[0][cur] != ' ') continue;
                drop(cur, 'X');
            }

            /* A pop-out can complete lines for both sides at once. */
            if (wins('X') && wins('O')) { render(cur, popmode, "Both complete — draw."); over = 1; }
            else if (wins('X'))         { render(cur, popmode, "You win!");    over = 1; }
            else if (wins('O'))         { render(cur, popmode, "Computer wins."); over = 1; }
            else if (full() && !POPOUT) { render(cur, popmode, "Draw.");       over = 1; }
            else {
                m = ai_move();
                if (m < 0) popout(-m - 1); else drop(m, 'O');
                if (wins('O') && wins('X')) { render(cur, popmode, "Both complete — draw."); over = 1; }
                else if (wins('O'))         { render(cur, popmode, "Computer wins."); over = 1; }
                else if (wins('X'))         { render(cur, popmode, "You win!"); over = 1; }
                else if (full() && !POPOUT) { render(cur, popmode, "Draw."); over = 1; }
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
