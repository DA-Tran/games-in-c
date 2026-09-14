/* GIC:PARAMETERISED checkers
 * checkers.c - the draughts family.
 *
 * params: size    = board edge (8, 10 or 12)
 *         variant = which national rules
 *
 * The national variants are not cosmetic. They differ on five axes that each
 * change play: whether men may capture backwards, whether kings fly along a
 * diagonal or step one square, whether you must take the longest available
 * sequence, whether movement is diagonal or orthogonal, and whether a man may
 * take a king. Giveaway inverts the goal entirely. Those flags are what the
 * table below encodes.
 */
#include "engine.h"
#include "games.h"

#define MAXN 12

/* '.' empty, 'x'/'X' human man/king, 'o'/'O' computer man/king */
static char bd[MAXN][MAXN];
static int N;

typedef struct {
    int back_cap;    /* men may capture backwards                      */
    int fly;         /* kings slide any distance                       */
    int maxcap;      /* must take the longest capture sequence         */
    int dirs;        /* 0 diagonal, 1 orthogonal, 2 both               */
    int side_move;   /* men may also move sideways (orthogonal games)  */
    int men_take_k;  /* men may capture kings                          */
    int giveaway;    /* lose every piece to win                        */
    int prom_cont;   /* promoting mid-capture continues as a king      */
    const char *name;
} Rules;

static Rules R;

static const Rules RULESET[13] = {
  /* back fly max dirs side mtk give pcont  name                  */
    {0, 0, 0, 0, 0, 1, 0, 0, "Checkers"},
    {1, 1, 1, 0, 0, 1, 0, 0, "International Draughts"},
    {1, 1, 0, 0, 0, 1, 0, 1, "Russian Draughts"},
    {1, 1, 1, 0, 0, 1, 0, 0, "Brazilian Draughts"},
    {0, 1, 1, 1, 1, 1, 0, 0, "Turkish Draughts"},
    {0, 0, 1, 0, 0, 0, 0, 0, "Italian Draughts"},
    {0, 1, 1, 0, 0, 1, 0, 0, "Spanish Draughts"},
    {1, 1, 0, 0, 0, 1, 0, 0, "Pool Checkers"},
    {1, 0, 0, 0, 0, 1, 1, 0, "Suicide Checkers"},
    {1, 1, 1, 2, 0, 1, 0, 0, "Frisian Draughts"},
    {1, 1, 1, 1, 1, 1, 0, 0, "Armenian Draughts"},
    {1, 1, 1, 0, 0, 1, 0, 0, "Canadian Checkers"},
    {0, 1, 1, 1, 0, 1, 0, 0, "Dameo"}
};

/* Direction tables: diagonal first, then orthogonal. */
static const int DR8[8] = {-1, -1, 1, 1, -1, 1, 0, 0};
static const int DC8[8] = {-1,  1,-1, 1,  0, 0,-1, 1};

static int dir_lo(void) { return R.dirs == 1 ? 4 : 0; }
static int dir_hi(void) { return R.dirs == 0 ? 4 : 8; }

static int is_mine(char c)   { return c == 'x' || c == 'X'; }
static int is_theirs(char c) { return c == 'o' || c == 'O'; }
static int is_king(char c)   { return c == 'X' || c == 'O'; }
static int on(int r, int c)  { return r >= 0 && r < N && c >= 0 && c < N; }

typedef struct { int r1, c1, r2, c2, jump, cr, cc; } Move;

/* Can this piece legally travel in direction d? */
static int man_may(int human, int d, int capturing)
{
    int dr = DR8[d];
    int fwd = human ? (dr < 0) : (dr > 0);
    int back = human ? (dr > 0) : (dr < 0);
    if (fwd) return 1;
    if (dr == 0) return R.side_move;              /* sideways */
    if (back) return capturing ? R.back_cap : 0;
    return 0;
}

static int enemy_at(int human, int r, int c)
{
    char v = bd[r][c];
    if (human ? !is_theirs(v) : !is_mine(v)) return 0;
    if (!R.men_take_k && is_king(v)) return 0;    /* Italian: men spare kings */
    return 1;
}

/* Captures available to the single piece at (r,c). */
static int caps_at(int r, int c, int human, Move *out, int max)
{
    char p = bd[r][c];
    int d, n = 0, king = is_king(p);
    if (human ? !is_mine(p) : !is_theirs(p)) return 0;

    for (d = dir_lo(); d < dir_hi(); d++) {
        int dr = DR8[d], dc = DC8[d];
        if (!king && !man_may(human, d, 1)) continue;
        if (king && R.fly) {
            int i = 1, tr, tc;
            /* slide over empties, take the first enemy, land beyond it */
            while (on(r + dr * i, c + dc * i) && bd[r + dr * i][c + dc * i] == '.') i++;
            tr = r + dr * i; tc = c + dc * i;
            if (!on(tr, tc) || !enemy_at(human, tr, tc)) continue;
            i++;
            while (on(r + dr * i, c + dc * i) && bd[r + dr * i][c + dc * i] == '.') {
                if (n < max) {
                    out[n].r1 = r; out[n].c1 = c;
                    out[n].r2 = r + dr * i; out[n].c2 = c + dc * i;
                    out[n].cr = tr; out[n].cc = tc; out[n].jump = 1; n++;
                }
                i++;
            }
        } else {
            int r1 = r + dr, c1 = c + dc, r2 = r + 2 * dr, c2 = c + 2 * dc;
            if (!on(r2, c2) || bd[r2][c2] != '.') continue;
            if (!enemy_at(human, r1, c1)) continue;
            if (n < max) {
                out[n].r1 = r; out[n].c1 = c; out[n].r2 = r2; out[n].c2 = c2;
                out[n].cr = r1; out[n].cc = c1; out[n].jump = 1; n++;
            }
        }
    }
    return n;
}

static void do_move(Move m, int human, int allow_promote)
{
    char p = bd[m.r1][m.c1];
    bd[m.r1][m.c1] = '.';
    if (m.jump) bd[m.cr][m.cc] = '.';
    if (allow_promote) {
        if (human  && m.r2 == 0)     p = 'X';
        if (!human && m.r2 == N - 1) p = 'O';
    }
    bd[m.r2][m.c2] = p;
}

/* Longest capture chain continuing from (r,c). */
static int chain_from(int r, int c, int human, int depth)
{
    Move buf[64];
    int n, i, best = 0;
    if (depth > 12) return 0;
    n = caps_at(r, c, human, buf, 64);
    for (i = 0; i < n; i++) {
        char save[MAXN][MAXN];
        int v;
        memcpy(save, bd, sizeof bd);
        do_move(buf[i], human, 0);
        v = 1 + chain_from(buf[i].r2, buf[i].c2, human, depth + 1);
        memcpy(bd, save, sizeof bd);
        if (v > best) best = v;
    }
    return best;
}

static int gen_moves(int human, Move *out, int max)
{
    int r, c, d, n = 0, jumps = 0;

    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        char p = bd[r][c];
        if (human ? !is_mine(p) : !is_theirs(p)) continue;
        {
            int got = caps_at(r, c, human, out + n, max - n);
            if (got > 0) jumps = 1;
            n += got;
        }
    }
    if (jumps) {
        if (R.maxcap) {                     /* keep only the longest sequences */
            int i, k = 0, best = 0;
            int len[128];
            for (i = 0; i < n && i < 128; i++) {
                char save[MAXN][MAXN];
                memcpy(save, bd, sizeof bd);
                do_move(out[i], human, 0);
                len[i] = 1 + chain_from(out[i].r2, out[i].c2, human, 0);
                memcpy(bd, save, sizeof bd);
                if (len[i] > best) best = len[i];
            }
            for (i = 0; i < n && i < 128; i++) if (len[i] == best) out[k++] = out[i];
            n = k;
        }
        return n;                            /* captures are compulsory */
    }

    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        char p = bd[r][c];
        int king = is_king(p);
        if (human ? !is_mine(p) : !is_theirs(p)) continue;
        for (d = dir_lo(); d < dir_hi(); d++) {
            int dr = DR8[d], dc = DC8[d], i;
            if (!king && !man_may(human, d, 0)) continue;
            if (king && R.fly) {
                for (i = 1; on(r + dr * i, c + dc * i) && bd[r + dr * i][c + dc * i] == '.'; i++)
                    if (n < max) {
                        out[n].r1 = r; out[n].c1 = c;
                        out[n].r2 = r + dr * i; out[n].c2 = c + dc * i;
                        out[n].jump = 0; n++;
                    }
            } else {
                int r1 = r + dr, c1 = c + dc;
                if (on(r1, c1) && bd[r1][c1] == '.' && n < max) {
                    out[n].r1 = r; out[n].c1 = c; out[n].r2 = r1; out[n].c2 = c1;
                    out[n].jump = 0; n++;
                }
            }
        }
    }
    return n;
}

/* Apply a player move; returns 1 when the same piece must jump again. */
static int apply_move(Move m, int human)
{
    int promoted = 0;
    char before = bd[m.r1][m.c1];
    do_move(m, human, 1);
    promoted = !is_king(before) && is_king(bd[m.r2][m.c2]);
    if (m.jump) {
        Move buf[64];
        /* In most rules promotion ends the turn; Russian lets it continue. */
        if (promoted && !R.prom_cont) return 0;
        if (caps_at(m.r2, m.c2, human, buf, 64) > 0) return 1;
    }
    return 0;
}

static int material(void)
{
    int r, c, s = 0;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        char p = bd[r][c];
        if (p == 'o')      s += 10 + r;
        else if (p == 'O') s += 28;
        else if (p == 'x') s -= 10 + (N - 1 - r);
        else if (p == 'X') s -= 28;
    }
    return R.giveaway ? -s : s;             /* giveaway wants to be captured */
}

static void ai_turn(void)
{
    for (;;) {
        Move buf[128];
        int n = gen_moves(0, buf, 128), i, best = -1000000, bi = 0;
        if (n == 0) return;
        for (i = 0; i < n; i++) {
            char save[MAXN][MAXN];
            int v;
            memcpy(save, bd, sizeof bd);
            apply_move(buf[i], 0);
            v = material() + (buf[i].jump ? (R.giveaway ? -30 : 40) : 0) + rnd(3);
            memcpy(bd, save, sizeof bd);
            if (v > best) { best = v; bi = i; }
        }
        if (!apply_move(buf[bi], 0)) return;
    }
}

static void render(int cr, int cc, int sr, int sc, const char *msg)
{
    int r, c, left = 40 - (N * 3) / 2;
    char sub[110];
    snprintf(sub, sizeof sub, "%s — %s%s%s%s", R.name,
             R.dirs == 1 ? "orthogonal" : R.dirs == 2 ? "orthogonal+diagonal" : "diagonal",
             R.fly ? ", flying kings" : "",
             R.maxcap ? ", must take the most" : "",
             R.giveaway ? ", LOSE everything to win" : "");
    draw_title("DRAUGHTS", sub);
    for (r = 0; r < N; r++) {
        scr_move(5 + r, left - 3);
        printf("%2d ", N - r);
        for (c = 0; c < N; c++) {
            char p = bd[r][c];
            int dark = ((r + c) % 2) == 1;
            const char *bg = (r == cr && c == cc) ? BG_BLUE
                           : (r == sr && c == sc) ? BG_GREEN
                           : (dark ? BG_BLACK : "");
            const char *fg = is_mine(p) ? C_CYAN : is_theirs(p) ? C_RED : C_GREY;
            char g = (p == '.') ? (dark ? '.' : ' ') : (is_king(p) ? 'K' : 'o');
            printf("%s%s %c %s", bg, fg, g, C_RESET);
        }
    }
    draw_text(7 + N, left - 6, "                                                        ");
    if (msg) draw_textf(7 + N, left - 4, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_checkers(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    if (v < 0 || v > 12) v = 0;
    R = RULESET[v];
    N = gp_int(p->size, 8);
    if (N < 8) N = 8;
    if (N > MAXN) N = MAXN;
    if (N & 1) N++;

    for (;;) {
        int cr = N - 3, cc = 0, sr = -1, sc = -1, r, c, over = 0;
        int rows = (N == 8) ? 3 : (N == 10) ? 4 : 5;

        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
            /* Turkish-style games fill whole rows; diagonal games use the
             * dark squares only. */
            int usable = (R.dirs == 1) ? (r != 0 && r != N - 1) : ((r + c) % 2 == 1);
            if (!usable)                 bd[r][c] = '.';
            else if (r < rows)           bd[r][c] = 'o';
            else if (r >= N - rows)      bd[r][c] = 'x';
            else                         bd[r][c] = '.';
        }

        while (!over) {
            Move buf[128];
            int n = gen_moves(1, buf, 128), k;
            if (n == 0) {
                render(cr, cc, sr, sc, R.giveaway ? "No moves left — you win!"
                                                  : "You have no moves — computer wins.");
                key_get(); break;
            }
            render(cr, cc, sr, sc, buf[0].jump ? "Capture available — you must take." : NULL);
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
                    if (buf[i].r1 == sr && buf[i].c1 == sc &&
                        buf[i].r2 == cr && buf[i].c2 == cc) found = i;
                if (found < 0) { render(cr, cc, sr, sc, "Illegal move."); sleep_ms(500); continue; }
                if (apply_move(buf[found], 1)) { sr = cr; sc = cc; continue; }
                sr = sc = -1;
            }
            ai_turn();
            {
                Move t[128];
                int mine = 0, theirs = 0;
                for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
                    if (is_mine(bd[r][c])) mine++;
                    if (is_theirs(bd[r][c])) theirs++;
                }
                if (R.giveaway) {
                    if (mine == 0)        { render(cr, cc, -1, -1, "All gone — you win!"); over = 1; }
                    else if (theirs == 0) { render(cr, cc, -1, -1, "Computer shed everything first."); over = 1; }
                } else {
                    if (theirs == 0 || gen_moves(0, t, 128) == 0) { render(cr, cc, -1, -1, "You win!"); over = 1; }
                    else if (mine == 0) { render(cr, cc, -1, -1, "Computer wins."); over = 1; }
                }
                if (over) key_get();
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
