/* connect4.c - Connect Four with a minimax AI over a scored board. */
#include "engine.h"
#include "games.h"

#define W 7
#define H 6

static char bd[H][W];

static int drop(int col, char p)
{
    int r;
    for (r = H - 1; r >= 0; r--)
        if (bd[r][col] == ' ') { bd[r][col] = p; return r; }
    return -1;
}

static int wins(char p)
{
    int r, c;
    for (r = 0; r < H; r++)
        for (c = 0; c < W; c++) {
            if (c + 3 < W && bd[r][c] == p && bd[r][c+1] == p && bd[r][c+2] == p && bd[r][c+3] == p) return 1;
            if (r + 3 < H && bd[r][c] == p && bd[r+1][c] == p && bd[r+2][c] == p && bd[r+3][c] == p) return 1;
            if (r + 3 < H && c + 3 < W && bd[r][c] == p && bd[r+1][c+1] == p && bd[r+2][c+2] == p && bd[r+3][c+3] == p) return 1;
            if (r + 3 < H && c - 3 >= 0 && bd[r][c] == p && bd[r+1][c-1] == p && bd[r+2][c-2] == p && bd[r+3][c-3] == p) return 1;
        }
    return 0;
}

static int full(void)
{
    int c;
    for (c = 0; c < W; c++) if (bd[0][c] == ' ') return 0;
    return 1;
}

/* Score a window of four by how close each side is to completing it. */
static int score_window(char a, char b, char c, char d)
{
    int me = 0, op = 0, sp = 0;
    char v[4]; int i;
    v[0]=a; v[1]=b; v[2]=c; v[3]=d;
    for (i = 0; i < 4; i++) {
        if (v[i] == 'O') me++;
        else if (v[i] == 'X') op++;
        else sp++;
    }
    if (me && op) return 0;
    if (me == 3 && sp == 1) return 50;
    if (me == 2 && sp == 2) return 10;
    if (op == 3 && sp == 1) return -60;
    if (op == 2 && sp == 2) return -8;
    return 0;
}

static int evaluate(void)
{
    int r, c, s = 0;
    for (r = 0; r < H; r++)
        for (c = 0; c < W; c++) {
            if (c + 3 < W) s += score_window(bd[r][c], bd[r][c+1], bd[r][c+2], bd[r][c+3]);
            if (r + 3 < H) s += score_window(bd[r][c], bd[r+1][c], bd[r+2][c], bd[r+3][c]);
            if (r + 3 < H && c + 3 < W) s += score_window(bd[r][c], bd[r+1][c+1], bd[r+2][c+2], bd[r+3][c+3]);
            if (r + 3 < H && c >= 3) s += score_window(bd[r][c], bd[r+1][c-1], bd[r+2][c-2], bd[r+3][c-3]);
        }
    for (r = 0; r < H; r++) s += (bd[r][W/2] == 'O') ? 6 : (bd[r][W/2] == 'X' ? -6 : 0);
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
        if (ai) {
            if (v > best) best = v;
            if (best > alpha) alpha = best;
        } else {
            if (v < best) best = v;
            if (best < beta) beta = best;
        }
        if (alpha >= beta) break;
    }
    return best;
}

static int ai_col(void)
{
    int c, best = -1000000, mv = -1;
    static const int order[W] = {3, 2, 4, 1, 5, 0, 6};
    for (c = 0; c < W; c++) {
        int col = order[c], r, v;
        if (bd[0][col] != ' ') continue;
        r = drop(col, 'O');
        v = negamax(5, -1000000, 1000000, 0);
        bd[r][col] = ' ';
        if (v > best) { best = v; mv = col; }
    }
    return mv;
}

static void render(int cur, const char *msg)
{
    int r, c;
    draw_title("CONNECT FOUR", "Left/Right to aim, Enter to drop, Q to quit");
    draw_text(6, 30, "                   ");
    draw_textf(6, 30 + cur * 3, "%s v %s", C_CYAN, C_RESET);
    for (r = 0; r < H; r++) {
        scr_move(7 + r, 29);
        for (c = 0; c < W; c++) {
            char p = bd[r][c];
            if (p == 'X')      printf("%s ● %s", C_CYAN, C_RESET);
            else if (p == 'O') printf("%s ● %s", C_YELLOW, C_RESET);
            else               printf("%s · %s", C_GREY, C_RESET);
        }
    }
    draw_text(15, 24, "                                        ");
    if (msg) draw_textf(15, 28, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_connect4(const GParams *p)
{
    (void)p;
    for (;;) {
        int cur = 3, over = 0, r, c;
        for (r = 0; r < H; r++) for (c = 0; c < W; c++) bd[r][c] = ' ';

        while (!over) {
            int k;
            render(cur, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT  && cur > 0)     cur--;
            if (k == KEY_RIGHT && cur < W - 1) cur++;
            if (k != KEY_ENTER && k != ' ') continue;
            if (bd[0][cur] != ' ') continue;

            drop(cur, 'X');
            if (wins('X'))      { render(cur, "You win!"); over = 1; }
            else if (full())    { render(cur, "Draw.");    over = 1; }
            else {
                int m = ai_col();
                if (m >= 0) drop(m, 'O');
                if (wins('O'))   { render(cur, "Computer wins."); over = 1; }
                else if (full()) { render(cur, "Draw.");          over = 1; }
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
