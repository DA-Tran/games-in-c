/* GIC:PARAMETERISED tictactoe
 * tictactoe.c - the noughts-and-crosses family.
 *
 * params: size    = board edge (3-6)
 *         count   = how many in a row wins
 *         variant = 0 plain      1 misere     2 wild       3 order and chaos
 *                   4 toroidal   5 notakto    6 nine-board 7 ultimate
 *                   8 numerical
 *
 * The variants are not skins. Misere inverts the goal, wild and order-and-chaos
 * let either player place either mark, notakto gives both players the same
 * mark, numerical wins on three cells summing to fifteen, and the two
 * nine-board games are played on nine sub-boards with a send-your-opponent
 * mechanic. Each needs its own win test, so the engine carries all of them.
 */
#include "engine.h"
#include "games.h"

#define MAXN 6
#define CELLS (MAXN * MAXN)

static int N, K, VAR;
static char bd[CELLS];
static const char *p_title_cache;

static const char *p_title_or(const char *dflt)
{
    return p_title_cache ? p_title_cache : dflt;
}

static const int DR[4] = {0, 1, 1, 1};
static const int DC[4] = {1, 0, 1, -1};

/* ------------------------------------------------------------ single board */

static char cell_at(int r, int c)
{
    if (VAR == 4) {                       /* toroidal: lines wrap the edges */
        r = (r % N + N) % N;
        c = (c % N + N) % N;
    } else if (r < 0 || r >= N || c < 0 || c >= N) {
        return '\0';
    }
    return bd[r * N + c];
}

/* Length of the run through (r,c) along direction d. */
static int run_len(int r, int c, int d, char who)
{
    int n = 1, i;
    for (i = 1; i < K; i++) {
        if (cell_at(r + DR[d] * i, c + DC[d] * i) != who) break;
        n++;
    }
    for (i = 1; i < K; i++) {
        if (cell_at(r - DR[d] * i, c - DC[d] * i) != who) break;
        n++;
    }
    return n;
}

static int made_line(int idx)
{
    int r = idx / N, c = idx % N, d;
    char who = bd[idx];
    if (who == ' ') return 0;
    for (d = 0; d < 4; d++) if (run_len(r, c, d, who) >= K) return 1;
    return 0;
}

/* Numerical noughts and crosses: any three cells in a line summing to 15. */
static int made_fifteen(void)
{
    static const int L[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},
                                {1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    int i;
    for (i = 0; i < 8; i++) {
        int a = bd[L[i][0]], b = bd[L[i][1]], c = bd[L[i][2]];
        if (a == ' ' || b == ' ' || c == ' ') continue;
        if ((a - '0') + (b - '0') + (c - '0') == 15) return 1;
    }
    return 0;
}

static int board_full(void)
{
    int i;
    for (i = 0; i < N * N; i++) if (bd[i] == ' ') return 0;
    return 1;
}

/* How good the position looks for `me`, counting runs that can still grow. */
static int heur(char me, char you)
{
    int r, c, d, s = 0;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        char w = bd[r * N + c];
        if (w == ' ') continue;
        for (d = 0; d < 4; d++) {
            int n = run_len(r, c, d, w);
            int v = n * n;
            s += (w == me) ? v : (w == you) ? -v : 0;
        }
    }
    return s;
}

/* Negamax over the plain, misere and toroidal rules. */
static int search(int aiturn, int depth, int lim, int alpha, int beta)
{
    int i, best;
    char me = 'O', you = 'X';
    if (depth >= lim || board_full()) return heur(me, you);
    best = aiturn ? -100000 : 100000;
    for (i = 0; i < N * N; i++) {
        int v;
        if (bd[i] != ' ') continue;
        bd[i] = aiturn ? me : you;
        if (made_line(i)) {
            /* Misere and notakto invert who benefits from completing a line. */
            int win = (VAR == 1 || VAR == 5) ? !aiturn : aiturn;
            v = win ? 9000 - depth : depth - 9000;
        } else {
            v = search(!aiturn, depth + 1, lim, alpha, beta);
        }
        bd[i] = ' ';
        if (aiturn) {
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

/* Wild and order-and-chaos let a player choose which mark to place, so the
 * AI has to search both choices per empty cell. */
static void ai_simple(int *mv, char *mark)
{
    int i, best = -1000000, lim;
    char cand[2];
    int ncand = 1, ci;

    lim = (N <= 3) ? 9 : (N == 4) ? 5 : 4;
    cand[0] = 'O';
    if (VAR == 2 || VAR == 3) { cand[0] = 'X'; cand[1] = 'O'; ncand = 2; }
    if (VAR == 5) cand[0] = 'X';              /* notakto: both play X */

    *mv = -1; *mark = cand[0];
    for (i = 0; i < N * N; i++) {
        if (bd[i] != ' ') continue;
        for (ci = 0; ci < ncand; ci++) {
            int v;
            bd[i] = cand[ci];
            if (made_line(i)) {
                /* Chaos wants no line at all; everyone else wants the line
                 * unless the rules are misere. */
                if (VAR == 3)      v = -9000;
                else if (VAR == 1 || VAR == 5) v = -9000;
                else               v = 9000;
            } else if (VAR == 2 || VAR == 3) {
                v = heur('O', 'X') + rnd(3);
            } else {
                v = search(0, 1, lim, -1000000, 1000000);
            }
            bd[i] = ' ';
            if (v > best) { best = v; *mv = i; *mark = cand[ci]; }
        }
    }
}

static int ai_numerical(void)
{
    int i, best = -1000000, mv = -1, n;
    for (n = 2; n <= 8; n += 2) {
        int used = 0;
        for (i = 0; i < 9; i++) if (bd[i] == '0' + n) used = 1;
        if (used) continue;
        for (i = 0; i < 9; i++) {
            int v;
            if (bd[i] != ' ') continue;
            bd[i] = '0' + n;
            v = made_fifteen() ? 9000 : rnd(50);
            bd[i] = ' ';
            if (v > best) { best = v; mv = i * 16 + n; }
        }
    }
    return mv;
}

static void draw_board(int cur, char pending, const char *msg, const char *sub)
{
    int r, c;
    draw_title(p_title_or("TIC TAC TOE"), sub);
    for (r = 0; r < N; r++) {
        for (c = 0; c < N; c++) {
            int i = r * N + c;
            const char *col = bd[i] == 'X' ? C_CYAN
                            : bd[i] == 'O' ? C_YELLOW
                            : bd[i] == ' ' ? C_GREY : C_GREEN;
            draw_textf(6 + r * 2, 30 + c * 4, "%s%s %c %s",
                       i == cur ? C_REV : "", col,
                       bd[i] == ' ' ? '.' : bd[i], C_RESET);
        }
    }
    if (VAR == 2 || VAR == 3)
        draw_textf(8 + N * 2, 24, "Placing: %s%c%s   (Tab switches)   ",
                   C_BOLD, pending, C_RESET);
    draw_text(10 + N * 2, 24, "                                          ");
    if (msg) draw_textf(10 + N * 2, 26, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

static int play_simple(void)
{
    static const char *SUB[9] = {
        "Arrows move, Enter places, Q quits",
        "MISERE: making a line LOSES",
        "WILD: place either mark; any line wins",
        "ORDER: make a line of either mark. Chaos stops you",
        "TOROIDAL: lines wrap around the edges",
        "NOTAKTO: both play X; making a line LOSES",
        "", "",
        "NUMERICAL: you have odds; three in a line summing to 15 wins"};
    int cur = (N * N) / 2, i;
    char pending = 'X';
    int mynum = 1;

    for (i = 0; i < N * N; i++) bd[i] = ' ';
    if (VAR == 5) pending = 'X';

    for (;;) {
        int k;
        draw_board(cur, pending, NULL, SUB[VAR]);
        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) return 0;
        if (k == KEY_LEFT)  cur = (cur % N == 0) ? cur + N - 1 : cur - 1;
        if (k == KEY_RIGHT) cur = (cur % N == N - 1) ? cur - N + 1 : cur + 1;
        if (k == KEY_UP)    cur = (cur < N) ? cur + N * (N - 1) : cur - N;
        if (k == KEY_DOWN)  cur = (cur >= N * (N - 1)) ? cur - N * (N - 1) : cur + N;
        if (k == '\t' && (VAR == 2 || VAR == 3)) pending = pending == 'X' ? 'O' : 'X';
        if (VAR == 8 && k >= '1' && k <= '9' && ((k - '0') & 1)) mynum = k - '0';
        if (k != KEY_ENTER && k != ' ') continue;
        if (bd[cur] != ' ') continue;

        if (VAR == 8) {
            int used = 0;
            for (i = 0; i < 9; i++) if (bd[i] == '0' + mynum) used = 1;
            if (used) {
                draw_board(cur, pending, "You have already used that number.", SUB[VAR]);
                key_get();
                continue;
            }
            bd[cur] = '0' + mynum;
            if (made_fifteen()) {
                draw_board(cur, pending, "Fifteen! You win.", SUB[VAR]);
                key_get(); return 1;
            }
        } else {
            bd[cur] = (VAR == 5) ? 'X' : (VAR == 2 || VAR == 3) ? pending : 'X';
            if (made_line(cur)) {
                int lose = (VAR == 1 || VAR == 5);
                draw_board(cur, pending,
                           lose ? "You made a line — you lose." : "You win!", SUB[VAR]);
                key_get(); return !lose;
            }
        }

        if (board_full()) {
            /* Chaos wins a full board; everyone else calls it a draw. */
            draw_board(cur, pending, VAR == 3 ? "Board full — Chaos wins." : "Draw.", SUB[VAR]);
            key_get(); return 0;
        }

        if (VAR == 8) {
            int m = ai_numerical();
            if (m >= 0) {
                bd[m / 16] = '0' + (m % 16);
                if (made_fifteen()) {
                    draw_board(cur, pending, "Computer makes fifteen.", SUB[VAR]);
                    key_get(); return 0;
                }
            }
        } else {
            int mv; char mk;
            ai_simple(&mv, &mk);
            if (mv >= 0) {
                bd[mv] = mk;
                if (made_line(mv)) {
                    int ailose = (VAR == 1 || VAR == 5);
                    draw_board(cur, pending,
                               ailose ? "Computer made a line — you win!"
                                      : "Computer wins.", SUB[VAR]);
                    key_get(); return ailose;
                }
            }
        }
        if (board_full()) {
            draw_board(cur, pending, VAR == 3 ? "Board full — Chaos wins." : "Draw.", SUB[VAR]);
            key_get(); return 0;
        }
    }
}

/* ------------------------------------------------------- nine-board games */

static char nb[9][9];
static char meta[9];

static int small_winner(const char *b)
{
    static const int L[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},
                                {1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    int i;
    for (i = 0; i < 8; i++)
        if (b[L[i][0]] != ' ' && b[L[i][0]] == b[L[i][1]] && b[L[i][1]] == b[L[i][2]])
            return b[L[i][0]];
    for (i = 0; i < 9; i++) if (b[i] == ' ') return 0;
    return 'D';
}

static int small_full(const char *b)
{
    int i;
    for (i = 0; i < 9; i++) if (b[i] == ' ') return 0;
    return 1;
}

static void draw_nine(int board, int cell, int forced, const char *msg)
{
    int B, r, c;
    draw_title(p_title_or("NINE BOARD"),
               VAR == 7 ? "ULTIMATE: win small boards to claim the big one"
                        : "NINE-BOARD: a line on any board wins");
    for (B = 0; B < 9; B++) {
        int br = (B / 3) * 8 + 4, bc = (B % 3) * 16 + 16;
        int active = (forced < 0 || forced == B);
        draw_textf(br - 1, bc, "%s%s#%d%s      ",
                   B == board ? C_REV : "", active ? C_WHITE : C_GREY, B + 1, C_RESET);
        for (r = 0; r < 3; r++) for (c = 0; c < 3; c++) {
            int i = r * 3 + c;
            char ch = nb[B][i];
            const char *col;
            if (VAR == 7 && meta[B] && meta[B] != 'D')
                col = meta[B] == 'X' ? C_CYAN : C_YELLOW;
            else col = ch == 'X' ? C_CYAN : ch == 'O' ? C_YELLOW : C_GREY;
            draw_textf(br + r, bc + c * 2,
                       "%s%s%c%s", (B == board && i == cell) ? C_REV : "",
                       col, ch == ' ' ? '.' : ch, C_RESET);
        }
    }
    draw_text(30, 20, "                                                   ");
    if (msg) draw_textf(30, 22, "%s%s%s", C_BOLD, msg, C_RESET);
    else if (forced >= 0) draw_textf(30, 22, "You must play in board #%d.   ", forced + 1);
    else draw_text(30, 22, "Play in any open board.        ");
    scr_flush();
}

static int nine_over(char who, const char *msg, int board, int cell, int forced)
{
    draw_nine(board, cell, forced, msg);
    key_get();
    return who == 'X';
}

static int play_nine(void)
{
    int B, i, board = 4, cell = 4, forced = -1;
    for (B = 0; B < 9; B++) { for (i = 0; i < 9; i++) nb[B][i] = ' '; meta[B] = 0; }

    for (;;) {
        int k, w;
        draw_nine(board, cell, forced, NULL);
        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) return 0;
        if (k == KEY_LEFT)  cell = (cell % 3 == 0) ? cell + 2 : cell - 1;
        if (k == KEY_RIGHT) cell = (cell % 3 == 2) ? cell - 2 : cell + 1;
        if (k == KEY_UP)    cell = (cell < 3) ? cell + 6 : cell - 3;
        if (k == KEY_DOWN)  cell = (cell > 5) ? cell - 6 : cell + 3;
        if (k == '\t') { do board = (board + 1) % 9; while (forced >= 0 && board != forced); }
        if (k != KEY_ENTER && k != ' ') continue;
        if (forced >= 0 && board != forced) continue;
        if (nb[board][cell] != ' ') continue;
        if (VAR == 7 && meta[board]) continue;

        nb[board][cell] = 'X';
        w = small_winner(nb[board]);
        if (VAR == 6 && w == 'X')
            return nine_over('X', "Line on board — you win!", board, cell, forced);
        if (VAR == 7) {
            if (w) meta[board] = (char)w;
            if (small_winner(meta) == 'X')
                return nine_over('X', "You win the big board!", board, cell, forced);
        }

        /* Your cell decides where the opponent must answer. */
        forced = cell;
        if (small_full(nb[forced]) || (VAR == 7 && meta[forced])) forced = -1;

        /* Reply: take a win, else block, else a central cell. */
        {
            int tb = forced, best = -1, bc2 = -1, s;
            if (tb < 0) {
                for (B = 0; B < 9; B++)
                    if (!small_full(nb[B]) && !(VAR == 7 && meta[B])) { tb = B; break; }
            }
            if (tb < 0) return nine_over('D', "All boards full — draw.", board, cell, forced);
            for (i = 0; i < 9; i++) {
                if (nb[tb][i] != ' ') continue;
                nb[tb][i] = 'O'; s = (small_winner(nb[tb]) == 'O') ? 1000 : 0;
                nb[tb][i] = 'X'; if (small_winner(nb[tb]) == 'X') s += 500;
                nb[tb][i] = ' ';
                s += (i == 4) ? 8 : (i % 2 == 0) ? 4 : 1;
                s += rnd(3);
                if (s > best) { best = s; bc2 = i; }
            }
            if (bc2 < 0) return nine_over('D', "No moves left — draw.", board, cell, forced);
            nb[tb][bc2] = 'O';
            w = small_winner(nb[tb]);
            if (VAR == 6 && w == 'O')
                return nine_over('O', "Computer made a line.", board, cell, forced);
            if (VAR == 7) {
                if (w) meta[tb] = (char)w;
                if (small_winner(meta) == 'O')
                    return nine_over('O', "Computer wins the big board.", board, cell, forced);
            }
            forced = bc2;
            if (small_full(nb[forced]) || (VAR == 7 && meta[forced])) forced = -1;
            board = (forced >= 0) ? forced : tb;
        }
    }
}

/* -------------------------------------------------------------------- run */

void fam_tictactoe(const GParams *p)
{
    int wins = 0;
    p_title_cache = p->title;
    VAR = gp_int(p->variant, 0);
    N = gp_int(p->size, 3);
    K = gp_int(p->count, 3);
    if (N < 3) N = 3;
    if (N > MAXN) N = MAXN;
    if (K < 3) K = 3;
    if (K > N) K = N;
    if (VAR == 8) { N = 3; K = 3; }

    for (;;) {
        int won = (VAR == 6 || VAR == 7) ? play_nine() : play_simple();
        if (won) { wins++; score_report(p->title ? p->title : "tictactoe", wins); }
        if (!confirm("\n  Play again?")) return;
    }
}
