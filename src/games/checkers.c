/* checkers.c - 8x8 draughts: forced jumps, multi-captures, kings. */
#include "engine.h"
#include "games.h"

#define N 8
/* '.' empty, 'x'/'X' human man/king, 'o'/'O' computer man/king */
static char bd[N][N];

static int is_mine(char c)  { return c == 'x' || c == 'X'; }
static int is_theirs(char c){ return c == 'o' || c == 'O'; }
static int is_king(char c)  { return c == 'X' || c == 'O'; }

typedef struct { int r1, c1, r2, c2, jump; } Move;

static int gen_moves(int human, Move *out, int max)
{
    int r, c, d, n = 0, jumps = 0;
    static const int DR[4] = {-1,-1,1,1};
    static const int DC[4] = {-1,1,-1,1};

    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        char p = bd[r][c];
        if (human ? !is_mine(p) : !is_theirs(p)) continue;
        for (d = 0; d < 4; d++) {
            int r1 = r + DR[d], c1 = c + DC[d];
            int r2 = r + 2*DR[d], c2 = c + 2*DC[d];
            /* Men only move forward unless crowned. */
            if (!is_king(p)) {
                if (human && DR[d] > 0) continue;
                if (!human && DR[d] < 0) continue;
            }
            if (r2 >= 0 && r2 < N && c2 >= 0 && c2 < N && bd[r2][c2] == '.' &&
                (human ? is_theirs(bd[r1][c1]) : is_mine(bd[r1][c1]))) {
                if (n < max) { out[n].r1=r; out[n].c1=c; out[n].r2=r2; out[n].c2=c2; out[n].jump=1; n++; }
                jumps = 1;
            } else if (r1 >= 0 && r1 < N && c1 >= 0 && c1 < N && bd[r1][c1] == '.') {
                if (n < max) { out[n].r1=r; out[n].c1=c; out[n].r2=r1; out[n].c2=c1; out[n].jump=0; n++; }
            }
        }
    }
    if (jumps) {                       /* captures are compulsory */
        int i, k = 0;
        for (i = 0; i < n; i++) if (out[i].jump) out[k++] = out[i];
        n = k;
    }
    return n;
}

/* Apply a move; returns 1 if the same piece can jump again. */
static int apply_move(Move m, int human)
{
    char p = bd[m.r1][m.c1];
    bd[m.r1][m.c1] = '.';
    if (m.jump) bd[(m.r1 + m.r2) / 2][(m.c1 + m.c2) / 2] = '.';
    if (human  && m.r2 == 0)     p = 'X';
    if (!human && m.r2 == N - 1) p = 'O';
    bd[m.r2][m.c2] = p;

    if (m.jump) {
        Move buf[64];
        int n = gen_moves(human, buf, 64), i;
        for (i = 0; i < n; i++)
            if (buf[i].jump && buf[i].r1 == m.r2 && buf[i].c1 == m.c2) return 1;
    }
    return 0;
}

static int material(void)
{
    int r, c, s = 0;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        char p = bd[r][c];
        if (p == 'o') s += 10 + (r);       /* advancement bonus */
        else if (p == 'O') s += 25;
        else if (p == 'x') s -= 10 + (N - 1 - r);
        else if (p == 'X') s -= 25;
    }
    return s;
}

static void ai_turn(void)
{
    for (;;) {
        Move buf[64];
        int n = gen_moves(0, buf, 64), i, best = -100000, bi = 0;
        if (n == 0) return;
        for (i = 0; i < n; i++) {
            char save[N][N];
            int v;
            memcpy(save, bd, sizeof bd);
            apply_move(buf[i], 0);
            v = material() + (buf[i].jump ? 40 : 0) + rnd(3);
            memcpy(bd, save, sizeof bd);
            if (v > best) { best = v; bi = i; }
        }
        if (!apply_move(buf[bi], 0)) return;
    }
}

static void render(int cr, int cc, int sr, int sc, const char *msg)
{
    int r, c;
    draw_title("CHECKERS", "Arrows move, Enter selects then targets, Q quits");
    draw_text(6, 26, "   a  b  c  d  e  f  g  h");
    for (r = 0; r < N; r++) {
        scr_move(7 + r, 26);
        printf("%d ", N - r);
        for (c = 0; c < N; c++) {
            char p = bd[r][c];
            int dark = ((r + c) % 2) == 1;
            const char *bg = (r == cr && c == cc) ? BG_BLUE
                           : (r == sr && c == sc) ? BG_GREEN
                           : (dark ? BG_BLACK : "");
            const char *fg = is_mine(p) ? C_CYAN : is_theirs(p) ? C_RED : C_GREY;
            char g = (p == '.') ? (dark ? '.' : ' ')
                   : (is_king(p) ? 'K' : 'o');
            printf("%s%s %c %s", bg, fg, g, C_RESET);
        }
    }
    draw_text(17, 20, "                                                        ");
    if (msg) draw_textf(17, 24, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_checkers(const GParams *p)
{
    (void)p;
    for (;;) {
        int cr = 5, cc = 0, sr = -1, sc = -1, r, c, over = 0;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
            if ((r + c) % 2 == 0) { bd[r][c] = '.'; continue; }
            if (r < 3)      bd[r][c] = 'o';
            else if (r > 4) bd[r][c] = 'x';
            else            bd[r][c] = '.';
        }

        while (!over) {
            Move buf[64];
            int n = gen_moves(1, buf, 64), k;
            if (n == 0) { render(cr, cc, sr, sc, "You have no moves - computer wins."); break; }
            render(cr, cc, sr, sc, buf[0].jump ? "Capture available - you must jump." : NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < N - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < N - 1) cc++;
            if (k != KEY_ENTER && k != ' ') continue;

            if (sr < 0) {
                if (is_mine(bd[cr][cc])) { sr = cr; sc = cc; }
                continue;
            }
            if (sr == cr && sc == cc) { sr = sc = -1; continue; }
            {
                int i, found = -1;
                for (i = 0; i < n; i++)
                    if (buf[i].r1 == sr && buf[i].c1 == sc && buf[i].r2 == cr && buf[i].c2 == cc)
                        found = i;
                if (found < 0) { render(cr, cc, sr, sc, "Illegal move."); sleep_ms(600); continue; }
                if (apply_move(buf[found], 1)) { sr = cr; sc = cc; continue; }
                sr = sc = -1;
            }
            ai_turn();
            {
                Move t[64];
                int mine = 0, theirs = 0;
                for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
                    if (is_mine(bd[r][c])) mine++;
                    if (is_theirs(bd[r][c])) theirs++;
                }
                if (theirs == 0 || gen_moves(0, t, 64) == 0) { render(cr, cc, -1, -1, "You win!"); over = 1; }
                else if (mine == 0) { render(cr, cc, -1, -1, "Computer wins."); over = 1; }
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
