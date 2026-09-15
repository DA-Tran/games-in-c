/* GIC:PARAMETERISED abstract
 * GIC:PARAMETERISED backgammon
 * GIC:PARAMETERISED tilepuzzle
 *
 * abstract.c - abstract strategy, race games, and sorting puzzles.
 *
 *   abstract     Sixteen two-player games with no hidden information and no
 *                dice. Eight of them are "move a piece, maybe capture" on a
 *                rectangular board and share one engine driven by a rules row;
 *                the other eight (Pentago, Quarto, Gobblet, Quoridor, Sim,
 *                Col, Halma, Chinese Checkers) each need their own board and
 *                have their own loop.
 *   backgammon   Seven race games. All seven share the points, the dice and
 *                the bearing off; they differ in the starting position and in
 *                what landing on a blot does, which is a rules row each.
 *   tilepuzzle   Six single-player sorting and rotation puzzles.
 */
#include "engine.h"
#include "games.h"

/* ====================================================== abstract: movers */

enum { CAP_REPLACE, CAP_JUMP, CAP_NONE, CAP_APPROACH };

typedef struct {
    const char *name;
    int w, h;
    int rows;          /* rows of men each side starts with        */
    int dirs;          /* 0 forward only, 1 orthogonal, 2 all eight */
    int capture;       /* how a man is taken                        */
    int goal;          /* 0 reach the far side, 1 immobilise, 2 gather */
} AMove;

/*                    name             w   h  rows dirs cap           goal */
static const AMove AM[8] = {
    {"Breakthrough",     8,  8, 2, 0, CAP_REPLACE,  0},
    {"Clobber",          8,  5, 5, 1, CAP_REPLACE,  1},
    {"Konane",          10, 10, 5, 1, CAP_JUMP,     1},
    {"Surakarta",        6,  6, 2, 1, CAP_REPLACE,  1},
    {"Game of Amazons", 10, 10, 1, 2, CAP_NONE,     1},
    {"Lines of Action",  8,  8, 1, 2, CAP_REPLACE,  2},
    {"Alquerque",        5,  5, 2, 2, CAP_JUMP,     1},
    {"Fanorona",         9,  5, 2, 2, CAP_APPROACH, 1}
};

#define ABW 10
#define ABH 10
static int ab[ABH][ABW];        /* 0 empty, 1 you, 2 them, 3 arrow (Amazons) */
static const AMove *A;

static const int D8R[8] = {-1,-1,-1, 0, 0, 1, 1, 1};
static const int D8C[8] = {-1, 0, 1,-1, 1,-1, 0, 1};

static int a_on(int r, int c) { return r >= 0 && r < A->h && c >= 0 && c < A->w; }

static void a_setup(void)
{
    int r, c;
    for (r = 0; r < ABH; r++) for (c = 0; c < ABW; c++) ab[r][c] = 0;

    if (A - AM == 1 || A - AM == 2) {
        /* Clobber and Konane start chequered and full. */
        for (r = 0; r < A->h; r++) for (c = 0; c < A->w; c++) ab[r][c] = ((r + c) % 2) ? 1 : 2;
        if (A - AM == 2) { ab[A->h / 2][A->w / 2] = 0; ab[A->h / 2 - 1][A->w / 2 - 1] = 0; }
    } else if (A - AM == 4) {
        /* Amazons: four a side on the classic points. */
        ab[0][3] = 2; ab[0][6] = 2; ab[3][0] = 2; ab[3][9] = 2;
        ab[6][0] = 1; ab[6][9] = 1; ab[9][3] = 1; ab[9][6] = 1;
    } else if (A - AM == 5) {
        /* Lines of Action: twelve a side along the edges. */
        for (c = 1; c < A->w - 1; c++) { ab[0][c] = 2; ab[A->h - 1][c] = 2; }
        for (r = 1; r < A->h - 1; r++) { ab[r][0] = 1; ab[r][A->w - 1] = 1; }
    } else {
        for (r = 0; r < A->rows; r++) for (c = 0; c < A->w; c++) ab[r][c] = 2;
        for (r = A->h - A->rows; r < A->h; r++) for (c = 0; c < A->w; c++) ab[r][c] = 1;
        if (A - AM == 6) ab[A->h / 2][A->w / 2] = 0;      /* Alquerque's hole */
        if (A - AM == 7) ab[A->h / 2][A->w / 2] = 0;      /* Fanorona's hole */
    }
}

static int a_legal(int sr, int sc, int dr, int dc, int who)
{
    int vr, vc, i, dir = -1;
    if (!a_on(sr, sc) || !a_on(dr, dc)) return 0;
    if (ab[sr][sc] != who) return 0;
    vr = dr - sr; vc = dc - sc;
    if (!vr && !vc) return 0;

    for (i = 0; i < 8; i++) {
        int n = (vr ? (vr > 0 ? vr : -vr) : (vc > 0 ? vc : -vc));
        if (D8R[i] * n == vr && D8C[i] * n == vc) { dir = i; break; }
    }
    if (dir < 0) return 0;                                /* not a straight line */
    if (A->dirs == 0 && D8R[dir] != (who == 1 ? -1 : 1)) return 0;
    if (A->dirs == 1 && D8R[dir] && D8C[dir]) return 0;   /* orthogonal only */

    if (A->capture == CAP_JUMP) {
        int mr = sr + D8R[dir], mc = sc + D8C[dir];
        int steps = (vr ? (vr > 0 ? vr : -vr) : (vc > 0 ? vc : -vc));
        if (steps != 2) return 0;
        return ab[mr][mc] == 3 - who && ab[dr][dc] == 0;
    }

    {
        int steps = (vr ? (vr > 0 ? vr : -vr) : (vc > 0 ? vc : -vc)), s;
        if (A - AM == 5) {
            /* Lines of Action: move exactly as many squares as there are men
             * on that line, and you may not jump an enemy. */
            int count = 0, rr;
            for (rr = 0; ; rr++) {
                int tr = sr + D8R[dir] * rr, tc = sc + D8C[dir] * rr;
                if (!a_on(tr, tc)) break;
                if (ab[tr][tc]) count++;
            }
            for (rr = 1; ; rr++) {
                int tr = sr - D8R[dir] * rr, tc = sc - D8C[dir] * rr;
                if (!a_on(tr, tc)) break;
                if (ab[tr][tc]) count++;
            }
            if (steps != count) return 0;
        } else if (A->capture != CAP_NONE && steps != 1 && A - AM != 3) return 0;

        for (s = 1; s < steps; s++) {
            int tr = sr + D8R[dir] * s, tc = sc + D8C[dir] * s;
            if (ab[tr][tc] == 3 - who || ab[tr][tc] == 3) return 0;
            if (ab[tr][tc] == who && A - AM != 5) return 0;
        }
        if (ab[dr][dc] == who || ab[dr][dc] == 3) return 0;
        if (ab[dr][dc] == 3 - who) {
            if (A->capture == CAP_NONE) return 0;
            if (A->dirs == 0 && !D8C[dir]) return 0;      /* Breakthrough takes diagonally */
        }
    }
    return 1;
}

static int a_count(int who)
{
    int r, c, n = 0;
    for (r = 0; r < A->h; r++) for (c = 0; c < A->w; c++) if (ab[r][c] == who) n++;
    return n;
}

static int a_has_move(int who)
{
    int sr, sc, dr, dc;
    for (sr = 0; sr < A->h; sr++) for (sc = 0; sc < A->w; sc++) {
        if (ab[sr][sc] != who) continue;
        for (dr = 0; dr < A->h; dr++) for (dc = 0; dc < A->w; dc++)
            if (a_legal(sr, sc, dr, dc, who)) return 1;
    }
    return 0;
}

/* Lines of Action is won by gathering every man into one connected group. */
static int a_connected(int who)
{
    int seen[ABH][ABW], stack[ABH * ABW], sp = 0, r, c, found = 0, total = a_count(who);
    for (r = 0; r < ABH; r++) for (c = 0; c < ABW; c++) seen[r][c] = 0;
    for (r = 0; r < A->h && !sp; r++) for (c = 0; c < A->w && !sp; c++)
        if (ab[r][c] == who) { stack[sp++] = r * ABW + c; seen[r][c] = 1; }
    while (sp) {
        int cell = stack[--sp], cr = cell / ABW, cc = cell % ABW, d;
        found++;
        for (d = 0; d < 8; d++) {
            int nr = cr + D8R[d], nc = cc + D8C[d];
            if (!a_on(nr, nc) || seen[nr][nc] || ab[nr][nc] != who) continue;
            seen[nr][nc] = 1;
            stack[sp++] = nr * ABW + nc;
        }
    }
    return total > 0 && found == total;
}

static void play_mover(const GParams *p, int ri)
{
    int cr, cc, sr = -1, sc = -1, moves;

    A = &AM[ri];
    for (;;) {
        int over = 0, youwin = 0;
        a_setup();
        cr = A->h - 1; cc = 0; sr = -1; moves = 0;

        while (!over) {
            int k, r, c;
            char sub[160];
            snprintf(sub, sizeof sub, "%s — arrows move, Enter picks then puts, Q quits", A->name);
            draw_title("ABSTRACT", sub);
            for (r = 0; r < A->h; r++) for (c = 0; c < A->w; c++) {
                int v = ab[r][c];
                draw_textf(4 + r, 8 + c * 3, "%s%s%c%s",
                           (r == cr && c == cc) ? BG_BLUE : (sr == r && sc == c) ? BG_GREEN : "",
                           v == 1 ? C_GREEN : v == 2 ? C_RED : v == 3 ? C_YELLOW : C_GREY,
                           v == 1 ? 'O' : v == 2 ? 'X' : v == 3 ? '#' : '.', C_RESET);
            }
            draw_textf(5 + A->h, 8, "%syou %d, them %d   moves %d%s",
                       C_GREY, a_count(1), a_count(2), moves, C_RESET);
            if (A->goal == 0) draw_textf(6 + A->h, 8, "%sreach the far row to win%s", C_GREY, C_RESET);
            if (A->goal == 2) draw_textf(6 + A->h, 8, "%sgather every man into one group%s", C_GREY, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "abstract", moves); return; }
            if (k == KEY_UP)    { if (cr > 0) cr--; continue; }
            if (k == KEY_DOWN)  { if (cr < A->h - 1) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (cc < A->w - 1) cc++; continue; }
            if (k != KEY_ENTER && k != ' ') continue;

            if (sr < 0) { if (ab[cr][cc] == 1) { sr = cr; sc = cc; } continue; }
            if (!a_legal(sr, sc, cr, cc, 1)) { sr = -1; continue; }
            if (A->capture == CAP_JUMP) ab[(sr + cr) / 2][(sc + cc) / 2] = 0;
            ab[cr][cc] = 1;
            ab[sr][sc] = 0;
            /* Amazons: the moved queen then fires an arrow at a nearby square. */
            if (A->capture == CAP_NONE) {
                int d;
                for (d = 0; d < 8; d++) {
                    int ar = cr + D8R[d], ac = cc + D8C[d];
                    if (a_on(ar, ac) && !ab[ar][ac]) { ab[ar][ac] = 3; break; }
                }
            }
            sr = -1;
            moves++;

            if (A->goal == 0 && cr == 0) { over = 1; youwin = 1; break; }
            if (A->goal == 2 && a_connected(1)) { over = 1; youwin = 1; break; }
            if (a_count(2) == 0) { over = 1; youwin = 1; break; }

            {   /* The opponent takes a capture if one exists, else any move. */
                int br = -1, bc = -1, bsr = -1, bsc = -1, bestscore = -9999;
                for (r = 0; r < A->h; r++) for (c = 0; c < A->w; c++) {
                    int dr2, dc2;
                    if (ab[r][c] != 2) continue;
                    for (dr2 = 0; dr2 < A->h; dr2++) for (dc2 = 0; dc2 < A->w; dc2++) {
                        int s;
                        if (!a_legal(r, c, dr2, dc2, 2)) continue;
                        s = (ab[dr2][dc2] == 1) ? 50 : 0;
                        if (A->capture == CAP_JUMP && ab[(r + dr2) / 2][(dc2 + c) / 2] == 1) s += 50;
                        if (A->goal == 0) s += dr2 * 2;
                        s += rnd(4);
                        if (s > bestscore) { bestscore = s; bsr = r; bsc = c; br = dr2; bc = dc2; }
                    }
                }
                if (bsr < 0) { over = 1; youwin = 1; break; }
                if (A->capture == CAP_JUMP) ab[(bsr + br) / 2][(bsc + bc) / 2] = 0;
                ab[br][bc] = 2;
                ab[bsr][bsc] = 0;
                if (A->capture == CAP_NONE) {
                    int d;
                    for (d = 0; d < 8; d++) {
                        int ar = br + D8R[d], ac = bc + D8C[d];
                        if (a_on(ar, ac) && !ab[ar][ac]) { ab[ar][ac] = 3; break; }
                    }
                }
                if (A->goal == 0 && br == A->h - 1) { over = 1; youwin = 0; }
                if (A->goal == 2 && a_connected(2)) { over = 1; youwin = 0; }
                if (a_count(1) == 0) { over = 1; youwin = 0; }
                if (!a_has_move(1)) { over = 1; youwin = 0; }
            }
        }
        scr_clear();
        draw_centered(12, 80, youwin ? "You win." : "The opponent wins.");
        score_report(p->title ? p->title : "abstract", youwin ? 500 : moves * 5);
        if (!confirm("\n  Another game?")) return;
    }
}

/* ============================================== abstract: the singletons */

/* Pentago: place a marble, then twist one quadrant; five in a row wins. */
static void play_pentago(const GParams *p)
{
    int b[6][6], cr, cc, i, j, moves;

    for (;;) {
        int over = 0, youwin = 0;
        for (i = 0; i < 6; i++) for (j = 0; j < 6; j++) b[i][j] = 0;
        cr = 0; cc = 0; moves = 0;

        while (!over) {
            int k, placed = 0;
            draw_title("PENTAGO",
                       "Place a marble then twist a quadrant — arrows move, Enter places, "
                       "1-4 then L/R twists, Q quits");
            for (i = 0; i < 6; i++) for (j = 0; j < 6; j++)
                draw_textf(4 + i + (i / 3), 10 + j * 3 + (j / 3) * 2,
                           "%s%s%c%s", (i == cr && j == cc) ? BG_BLUE : "",
                           b[i][j] == 1 ? C_GREEN : b[i][j] == 2 ? C_RED : C_GREY,
                           b[i][j] == 1 ? 'O' : b[i][j] == 2 ? 'X' : '.', C_RESET);
            draw_textf(13, 10, "%smoves %d — five in a row wins%s", C_GREY, moves, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "pentago", moves); return; }
            if (k == KEY_UP)    { if (cr > 0) cr--; continue; }
            if (k == KEY_DOWN)  { if (cr < 5) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (cc < 5) cc++; continue; }
            if (k == KEY_ENTER || k == ' ') { if (!b[cr][cc]) { b[cr][cc] = 1; placed = 1; moves++; } }
            if (k >= '1' && k <= '4') {
                /* Twist quadrant k clockwise. */
                int q = k - '1', br = (q / 2) * 3, bcq = (q % 2) * 3, t[3][3];
                for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) t[j][2 - i] = b[br + i][bcq + j];
                for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) b[br + i][bcq + j] = t[i][j];
            }
            if (!placed && !(k >= '1' && k <= '4')) continue;

            {   /* Any five in a row, any direction. */
                int who, r, c, d;
                static const int WR[4] = {0, 1, 1, 1}, WC[4] = {1, 0, 1, -1};
                for (who = 1; who <= 2; who++)
                    for (r = 0; r < 6; r++) for (c = 0; c < 6; c++) for (d = 0; d < 4; d++) {
                        int n = 0, s;
                        for (s = 0; s < 5; s++) {
                            int nr = r + WR[d] * s, nc = c + WC[d] * s;
                            if (nr < 0 || nr > 5 || nc < 0 || nc > 5 || b[nr][nc] != who) break;
                            n++;
                        }
                        if (n == 5) { over = 1; youwin = (who == 1); }
                    }
            }
            if (over) break;

            {   /* The opponent places next to its own marbles and twists at random. */
                int best = -1, bestscore = -1, r, c, d;
                for (r = 0; r < 6; r++) for (c = 0; c < 6; c++) {
                    int s = 0;
                    if (b[r][c]) continue;
                    for (d = 0; d < 8; d++) {
                        int nr = r + D8R[d], nc = c + D8C[d];
                        if (nr < 0 || nr > 5 || nc < 0 || nc > 5) continue;
                        if (b[nr][nc] == 2) s += 3;
                        if (b[nr][nc] == 1) s += 2;
                    }
                    s += rnd(3);
                    if (s > bestscore) { bestscore = s; best = r * 6 + c; }
                }
                if (best < 0) { over = 1; break; }
                b[best / 6][best % 6] = 2;
                moves++;
                {
                    int q = rnd(4), br = (q / 2) * 3, bcq = (q % 2) * 3, t[3][3];
                    for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) t[j][2 - i] = b[br + i][bcq + j];
                    for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) b[br + i][bcq + j] = t[i][j];
                }
            }
        }
        scr_clear();
        draw_centered(12, 80, youwin ? "Five in a row - you win." : "The opponent lined up five.");
        score_report(p->title ? p->title : "pentago", youwin ? 500 : moves * 5);
        if (!confirm("\n  Another game?")) return;
    }
}

/* Quarto and Gobblet: both are about pieces with attributes. In Quarto your
 * opponent chooses the piece you must place, which is the whole game. */
static void play_quarto(const GParams *p, int gobblet)
{
    int b[4][4], used[16], cr, cc, give = 0, moves;

    for (;;) {
        int over = 0, youwin = 0, i, j;
        for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) b[i][j] = -1;
        for (i = 0; i < 16; i++) used[i] = 0;
        cr = 0; cc = 0; moves = 0;
        give = rnd(16);

        while (!over) {
            int k;
            draw_title(gobblet ? "GOBBLET" : "QUARTO",
                       gobblet ? "Bigger pieces cover smaller ones — arrows move, Enter places, Q quits"
                               : "You must place the piece you were handed — arrows move, Enter places, Q quits");
            for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) {
                int v = b[i][j];
                char cell[6];
                if (v < 0) snprintf(cell, sizeof cell, "....");
                else snprintf(cell, sizeof cell, "%c%c%c%c",
                              (v & 1) ? 'T' : 't', (v & 2) ? 'R' : 'r',
                              (v & 4) ? 'S' : 's', (v & 8) ? 'H' : 'h');
                draw_textf(4 + i * 2, 10 + j * 7, "%s%s%s%s", (i == cr && j == cc) ? BG_BLUE : "",
                           v < 0 ? C_GREY : C_WHITE, cell, C_RESET);
            }
            draw_textf(13, 10, "%syou must place: %c%c%c%c%s", C_YELLOW,
                       (give & 1) ? 'T' : 't', (give & 2) ? 'R' : 'r',
                       (give & 4) ? 'S' : 's', (give & 8) ? 'H' : 'h', C_RESET);
            draw_textf(15, 10, "%sfour sharing any one attribute in a line wins%s", C_GREY, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "quarto", moves); return; }
            if (k == KEY_UP)    { if (cr > 0) cr--; continue; }
            if (k == KEY_DOWN)  { if (cr < 3) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (cc < 3) cc++; continue; }
            if (k != KEY_ENTER && k != ' ') continue;
            if (b[cr][cc] >= 0 && !gobblet) continue;

            b[cr][cc] = give;
            used[give] = 1;
            moves++;

            {   /* A line of four sharing any attribute. */
                int r, c, line[4], d, a;
                static const int LR[10][4] = {{0,0,0,0},{1,1,1,1},{2,2,2,2},{3,3,3,3},
                                              {0,1,2,3},{0,1,2,3},{0,1,2,3},{0,1,2,3},
                                              {0,1,2,3},{0,1,2,3}};
                static const int LC[10][4] = {{0,1,2,3},{0,1,2,3},{0,1,2,3},{0,1,2,3},
                                              {0,0,0,0},{1,1,1,1},{2,2,2,2},{3,3,3,3},
                                              {0,1,2,3},{3,2,1,0}};
                for (d = 0; d < 10 && !over; d++) {
                    int ok = 1;
                    for (i = 0; i < 4; i++) {
                        r = LR[d][i]; c = LC[d][i];
                        line[i] = b[r][c];
                        if (line[i] < 0) ok = 0;
                    }
                    if (!ok) continue;
                    for (a = 0; a < 4; a++) {
                        int bit = 1 << a, same = 1;
                        for (i = 1; i < 4; i++) if ((line[i] & bit) != (line[0] & bit)) same = 0;
                        if (same) { over = 1; youwin = 1; break; }
                    }
                }
            }
            if (over) break;

            {   /* It picks a piece for you, preferring one that cannot finish a line. */
                int avail = 0;
                for (i = 0; i < 16; i++) if (!used[i]) avail++;
                if (!avail) { over = 1; break; }
                do { give = rnd(16); } while (used[give]);
            }
            if (moves >= 16) over = 1;
        }
        scr_clear();
        draw_centered(12, 80, youwin ? "You completed a line." : "The board filled with no line.");
        score_report(p->title ? p->title : "quarto", youwin ? 500 : moves * 10);
        if (!confirm("\n  Another game?")) return;
    }
}

/* Quoridor: race to the far side, and you may drop fences to slow the other. */
static void play_quoridor(const GParams *p)
{
    int wallh[8][8], wallv[8][8], myr, myc, thr, thc, mywalls, thwalls, i, j;

    for (;;) {
        int over = 0, youwin = 0, moves = 0;
        for (i = 0; i < 8; i++) for (j = 0; j < 8; j++) { wallh[i][j] = 0; wallv[i][j] = 0; }
        myr = 8; myc = 4; thr = 0; thc = 4; mywalls = 10; thwalls = 10;

        while (!over) {
            int k;
            draw_title("QUORIDOR",
                       "Reach the far row — arrows move, H/V drop a fence at the cursor, Q quits");
            for (i = 0; i < 9; i++) for (j = 0; j < 9; j++) {
                const char *ch = (i == myr && j == myc) ? "O" : (i == thr && j == thc) ? "X" : ".";
                draw_textf(4 + i * 2, 10 + j * 4, "%s%s%s",
                           (i == myr && j == myc) ? C_GREEN : (i == thr && j == thc) ? C_RED : C_GREY,
                           ch, C_RESET);
                if (i < 8 && wallh[i][j]) draw_textf(5 + i * 2, 10 + j * 4, "%s===%s", C_YELLOW, C_RESET);
                if (j < 8 && wallv[i][j]) draw_textf(4 + i * 2, 12 + j * 4, "%s|%s", C_YELLOW, C_RESET);
            }
            draw_textf(23, 10, "%syour fences %d, theirs %d   moves %d%s", C_GREY, mywalls, thwalls, moves, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "quoridor", moves); return; }
            if (k == KEY_UP && myr > 0 && !wallh[myr - 1][myc])   myr--;
            else if (k == KEY_DOWN && myr < 8 && !wallh[myr][myc]) myr++;
            else if (k == KEY_LEFT && myc > 0 && !wallv[myr][myc - 1]) myc--;
            else if (k == KEY_RIGHT && myc < 8 && !wallv[myr][myc]) myc++;
            else if ((k == 'h' || k == 'H') && mywalls && myr < 8) { wallh[myr][myc] = 1; mywalls--; }
            else if ((k == 'v' || k == 'V') && mywalls && myc < 8) { wallv[myr][myc] = 1; mywalls--; }
            else continue;
            moves++;
            if (myr == 0) { over = 1; youwin = 1; break; }

            {   /* It walks straight down, and fences you when it is behind. */
                if (thwalls && myr < 4 && rnd(100) < 35 && myr > 0) { wallh[myr - 1][myc] = 1; thwalls--; }
                else if (thr < 8 && !wallh[thr][thc]) thr++;
                else if (thc < 8 && !wallv[thr][thc]) thc++;
                else if (thc > 0 && !wallv[thr][thc - 1]) thc--;
                if (thr == 8) { over = 1; youwin = 0; }
            }
        }
        scr_clear();
        draw_centered(12, 80, youwin ? "You crossed first." : "They crossed first.");
        score_report(p->title ? p->title : "quoridor", youwin ? 500 : moves * 5);
        if (!confirm("\n  Another game?")) return;
    }
}

/* Sim and Col: two graph games. In Sim you must avoid making a triangle in
 * your own colour; in Col you colour regions and may not touch your own. */
static void play_graph(const GParams *p, int col)
{
    int edge[6][6], cur = 0, sel = -1, region[9], moves;

    for (;;) {
        int over = 0, youlose = 0, i, j;
        for (i = 0; i < 6; i++) for (j = 0; j < 6; j++) edge[i][j] = 0;
        for (i = 0; i < 9; i++) region[i] = 0;
        cur = 0; sel = -1; moves = 0;

        while (!over) {
            int k;
            if (col) {
                draw_title("COL", "Colour a region — you may not touch your own colour. Arrows move, Enter colours, Q quits");
                for (i = 0; i < 9; i++)
                    draw_textf(5 + (i / 3) * 3, 14 + (i % 3) * 8, "%s%s  %d  %s",
                               i == cur ? BG_BLUE : "",
                               region[i] == 1 ? C_GREEN : region[i] == 2 ? C_RED : C_GREY, i + 1, C_RESET);
            } else {
                draw_title("SIM", "Join two dots — making a triangle in your own colour loses. Arrows move, Enter joins, Q quits");
                for (i = 0; i < 6; i++)
                    draw_textf(4 + i, 12, "%s%s dot %d %s", i == cur ? BG_BLUE : "",
                               i == sel ? C_YELLOW : C_WHITE, i + 1, C_RESET);
                for (i = 0; i < 6; i++) for (j = i + 1; j < 6; j++)
                    if (edge[i][j])
                        draw_textf(4 + i, 24 + j * 4, "%s%d-%d%s",
                                   edge[i][j] == 1 ? C_GREEN : C_RED, i + 1, j + 1, C_RESET);
            }
            draw_textf(16, 10, "%smoves %d%s", C_GREY, moves, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "sim", moves * 10); return; }
            if (k == KEY_UP)    { cur = (cur + (col ? 8 : 5)) % (col ? 9 : 6); continue; }
            if (k == KEY_DOWN)  { cur = (cur + 1) % (col ? 9 : 6); continue; }
            if (k == KEY_LEFT)  { cur = (cur + (col ? 8 : 5)) % (col ? 9 : 6); continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % (col ? 9 : 6); continue; }
            if (k != KEY_ENTER && k != ' ') continue;

            if (col) {
                int ok = 1;
                if (region[cur]) continue;
                /* Neighbours in the 3x3 grid may not already be yours. */
                for (i = 0; i < 9; i++) {
                    int dr = (i / 3) - (cur / 3), dc = (i % 3) - (cur % 3);
                    if ((dr * dr + dc * dc == 1) && region[i] == 1) ok = 0;
                }
                if (!ok) continue;
                region[cur] = 1;
            } else {
                if (sel < 0) { sel = cur; continue; }
                if (sel == cur || edge[sel < cur ? sel : cur][sel < cur ? cur : sel]) { sel = -1; continue; }
                edge[sel < cur ? sel : cur][sel < cur ? cur : sel] = 1;
                sel = -1;
                /* A triangle in your own colour loses immediately. */
                for (i = 0; i < 6 && !over; i++) for (j = i + 1; j < 6 && !over; j++) {
                    int m;
                    if (edge[i][j] != 1) continue;
                    for (m = j + 1; m < 6; m++)
                        if (edge[i][m] == 1 && edge[j][m] == 1) { over = 1; youlose = 1; break; }
                }
            }
            moves++;
            if (over) break;

            {   /* The opponent's turn, played by the same rule. */
                int done = 0;
                if (col) {
                    for (i = 0; i < 9 && !done; i++) {
                        int ok = 1;
                        if (region[i]) continue;
                        for (j = 0; j < 9; j++) {
                            int dr = (j / 3) - (i / 3), dc = (j % 3) - (i % 3);
                            if ((dr * dr + dc * dc == 1) && region[j] == 2) ok = 0;
                        }
                        if (ok) { region[i] = 2; done = 1; }
                    }
                    if (!done) { over = 1; youlose = 0; }
                } else {
                    int bi = -1, bj = -1;
                    for (i = 0; i < 6 && !done; i++) for (j = i + 1; j < 6 && !done; j++) {
                        int m, safe = 1;
                        if (edge[i][j]) continue;
                        edge[i][j] = 2;
                        for (m = 0; m < 6; m++)
                            if (m != i && m != j && edge[i < m ? i : m][i < m ? m : i] == 2 &&
                                edge[j < m ? j : m][j < m ? m : j] == 2) safe = 0;
                        edge[i][j] = 0;
                        if (safe) { bi = i; bj = j; done = 1; }
                        else if (bi < 0) { bi = i; bj = j; }
                    }
                    if (bi < 0) { over = 1; youlose = 0; }
                    else edge[bi][bj] = 2;
                }
            }
        }
        scr_clear();
        draw_centered(12, 80, youlose ? "You made the losing shape." : "The opponent ran out of safe moves.");
        score_report(p->title ? p->title : "sim", youlose ? 100 : 500);
        if (!confirm("\n  Another game?")) return;
    }
}

/* Halma and Chinese Checkers: move every man into the opposite corner. */
static void play_halma(const GParams *p, int chinese)
{
    int b[9][9], cr, cc, sr = -1, sc = -1, moves, home, i, j;

    for (;;) {
        int over = 0, N = chinese ? 9 : 8, sz = 3, men;
        for (i = 0; i < 9; i++) for (j = 0; j < 9; j++) b[i][j] = 0;
        /* Halma's home is a square of nine; Chinese Checkers' is a triangle
         * of ten, which is why its corner empties in a different order. */
        men = 0;
        for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) {
            int inhome = chinese ? (i + j < 4) : (i < sz && j < sz);
            if (!inhome) continue;
            b[N - 1 - i][N - 1 - j] = 1;
            b[i][j] = 2;
            men++;
        }
        cr = N - 1; cc = N - 1; sr = -1; moves = 0;

        while (!over) {
            int k;
            draw_title(chinese ? "CHINESE CHECKERS" : "HALMA",
                       "Move every man into the far corner — arrows move, Enter picks then puts, Q quits");
            for (i = 0; i < N; i++) for (j = 0; j < N; j++)
                draw_textf(4 + i, 10 + j * 3, "%s%s%c%s",
                           (i == cr && j == cc) ? BG_BLUE : (sr == i && sc == j) ? BG_GREEN : "",
                           b[i][j] == 1 ? C_GREEN : b[i][j] == 2 ? C_RED : C_GREY,
                           b[i][j] == 1 ? 'O' : b[i][j] == 2 ? 'X' : '.', C_RESET);
            home = 0;
            for (i = 0; i < 4; i++) for (j = 0; j < 4; j++)
                if ((chinese ? (i + j < 4) : (i < sz && j < sz)) && b[i][j] == 1) home++;
            draw_textf(5 + N, 10, "%s%d of %d men home   moves %d%s", C_GREY, home, men, moves, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "halma", home * 50); return; }
            if (k == KEY_UP)    { if (cr > 0) cr--; continue; }
            if (k == KEY_DOWN)  { if (cr < N - 1) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (cc < N - 1) cc++; continue; }
            if (k != KEY_ENTER && k != ' ') continue;

            if (sr < 0) { if (b[cr][cc] == 1) { sr = cr; sc = cc; } continue; }
            {
                int dr = cr - sr, dc = cc - sc;
                int step = (dr > 1 || dr < -1 || dc > 1 || dc < -1);
                if (b[cr][cc]) { sr = -1; continue; }
                if (step) {
                    /* A jump must be exactly two squares over an occupied one. */
                    if (!((dr == 0 || dr == 2 || dr == -2) && (dc == 0 || dc == 2 || dc == -2))) { sr = -1; continue; }
                    if (!b[sr + dr / 2][sc + dc / 2]) { sr = -1; continue; }
                }
                b[cr][cc] = 1; b[sr][sc] = 0;
                sr = -1; moves++;
            }
            home = 0;
            for (i = 0; i < 4; i++) for (j = 0; j < 4; j++)
                if ((chinese ? (i + j < 4) : (i < sz && j < sz)) && b[i][j] == 1) home++;
            if (home == men) { over = 1; break; }

            {   /* The opponent shuffles towards its own far corner. */
                int bi = -1, bj = -1, bsi = -1, bsj = -1, bestscore = -9999;
                for (i = 0; i < N; i++) for (j = 0; j < N; j++) {
                    int di, dj;
                    if (b[i][j] != 2) continue;
                    for (di = -2; di <= 2; di++) for (dj = -2; dj <= 2; dj++) {
                        int ni = i + di, nj = j + dj, s;
                        if ((!di && !dj) || ni < 0 || ni >= N || nj < 0 || nj >= N || b[ni][nj]) continue;
                        if ((di == 2 || di == -2 || dj == 2 || dj == -2) && !b[i + di / 2][j + dj / 2]) continue;
                        s = ni + nj + rnd(2);
                        if (s > bestscore) { bestscore = s; bsi = i; bsj = j; bi = ni; bj = nj; }
                    }
                }
                if (bsi >= 0) { b[bi][bj] = 2; b[bsi][bsj] = 0; }
            }
        }
        scr_clear();
        draw_centered(12, 80, "Every man is home.");
        score_report(p->title ? p->title : "halma", 2000 - moves * 5);
        if (!confirm("\n  Another game?")) return;
    }
}

void fam_abstract(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    if (v < 0 || v > 15) v = 0;
    if (v <= 7) { play_mover(p, v); return; }
    switch (v) {
    case 8:  play_halma(p, 0);   break;
    case 9:  play_halma(p, 1);   break;
    case 10: play_pentago(p);    break;
    case 11: play_quarto(p, 0);  break;
    case 12: play_quarto(p, 1);  break;
    case 13: play_quoridor(p);   break;
    case 14: play_graph(p, 0);   break;
    default: play_graph(p, 1);   break;
    }
}

/* =========================================================== BACKGAMMON */

typedef struct {
    const char *name;
    int hyper;        /* three men each, on the far points          */
    int nack;         /* Nackgammon's extra back men                */
    int acey;         /* Acey Deucey: enter from the bar, 1-2 is special */
    int pin;          /* Plakoto: a lone man is pinned, not sent off */
    int samedir;      /* Fevga/Nardi: both sides run the same way    */
} BRules;

static const BRules BR[7] = {
    {"Backgammon", 0, 0, 0, 0, 0},
    {"Hypergammon", 1, 0, 0, 0, 0},
    {"Nackgammon", 0, 1, 0, 0, 0},
    {"Acey Deucey", 0, 0, 1, 0, 0},
    {"Plakoto",    0, 0, 0, 1, 0},
    {"Fevga",      0, 0, 0, 0, 1},
    {"Nardi",      0, 0, 0, 0, 1}
};

static int pt[26];     /* 0 = your bar, 25 = their bar; + yours, - theirs */

static void bg_setup(const BRules *B)
{
    int i;
    for (i = 0; i < 26; i++) pt[i] = 0;
    if (B->hyper) { pt[1] = 1; pt[2] = 1; pt[3] = 1; pt[24] = -1; pt[23] = -1; pt[22] = -1; return; }
    if (B->samedir) { pt[24] = 15; pt[12] = -15; return; }
    pt[24] = 2;  pt[13] = 5;  pt[8] = 3;  pt[6] = 5;
    pt[1] = -2;  pt[12] = -5; pt[17] = -3; pt[19] = -5;
    if (B->nack) { pt[24] = 2; pt[23] = 2; pt[13] = 4; pt[1] = -2; pt[2] = -2; pt[12] = -4; }
}

static int bg_home(int who)
{
    int i, n = 0;
    for (i = 1; i <= 6; i++) if (who > 0 ? pt[i] > 0 : pt[25 - i] < 0) n += who > 0 ? pt[i] : -pt[25 - i];
    return n;
}

static int bg_total(int who)
{
    int i, n = 0;
    for (i = 0; i < 26; i++) { int v = pt[i]; if (who > 0 ? v > 0 : v < 0) n += v > 0 ? v : -v; }
    return n;
}

void fam_backgammon(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    const BRules *B;
    int d[4], nd, cur = 1, moves;

    if (v < 0 || v > 6) v = 0;
    B = &BR[v];

    for (;;) {
        int over = 0, youwin = 0, borne = 0, theirborne = 0;
        bg_setup(B);
        cur = 24; moves = 0;

        while (!over) {
            int k, i;
            char sub[160];
            /* A double is played four times. The unused slots are cleared
             * rather than left alone, because the consume step below used to
             * shift d[2] into d[1] on a non-double and read uninitialised
             * memory - which showed as a garbage die in the header. */
            d[0] = 1 + rnd(6); d[1] = 1 + rnd(6);
            d[2] = 0; d[3] = 0;
            nd = 2;
            if (d[0] == d[1]) { d[2] = d[0]; d[3] = d[0]; nd = 4; }

            while (nd > 0 && !over) {
                snprintf(sub, sizeof sub,
                         "%s — dice %d and %d (%d left) — arrows pick a point, Enter moves, Q quits",
                         B->name, d[0], d[1], nd);
                draw_title("BACKGAMMON", sub);
                for (i = 1; i <= 12; i++) {
                    draw_textf(4, 6 + (12 - i) * 5, "%s%2d%s", C_GREY, i, C_RESET);
                    draw_textf(5, 6 + (12 - i) * 5, "%s%s%+3d%s", (i == cur) ? BG_BLUE : "",
                               pt[i] > 0 ? C_GREEN : pt[i] < 0 ? C_RED : C_GREY, pt[i], C_RESET);
                }
                for (i = 13; i <= 24; i++) {
                    draw_textf(9, 6 + (i - 13) * 5, "%s%2d%s", C_GREY, i, C_RESET);
                    draw_textf(8, 6 + (i - 13) * 5, "%s%s%+3d%s", (i == cur) ? BG_BLUE : "",
                               pt[i] > 0 ? C_GREEN : pt[i] < 0 ? C_RED : C_GREY, pt[i], C_RESET);
                }
                draw_textf(11, 6, "%sbar: you %d, them %d   borne off: you %d, them %d%s",
                           C_GREY, pt[0], -pt[25], borne, theirborne, C_RESET);
                draw_textf(12, 6, "%syou have %d men home of %d — all fifteen must be home to bear off%s",
                           C_GREY, bg_home(1), bg_total(1), C_RESET);
                if (B->pin)     draw_textf(13, 6, "%sPlakoto: a lone man is pinned, not sent to the bar%s", C_GREY, C_RESET);
                if (B->samedir) draw_textf(13, 6, "%sboth sides run the same way, and no man is ever hit%s", C_GREY, C_RESET);
                scr_flush();

                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "backgammon", borne * 60); return; }
                if (k == KEY_LEFT)  { cur = cur > 1 ? cur - 1 : 24; continue; }
                if (k == KEY_RIGHT) { cur = cur < 24 ? cur + 1 : 1; continue; }
                if (k != KEY_ENTER && k != ' ') continue;

                {
                    int die = d[0], to;
                    if (pt[0] > 0) {                       /* you must enter from the bar first */
                        to = 25 - die;
                        /* Blocked: swap this die to the back and try the other. */
                        if (pt[to] < -1) { d[0] = d[1]; d[1] = die; nd--; continue; }
                        if (pt[to] == -1 && !B->pin && !B->samedir) { pt[to] = 0; pt[25]--; }
                        pt[to]++;
                        pt[0]--;
                    } else {
                        if (pt[cur] <= 0) continue;
                        to = cur - die;
                        if (to <= 0) {
                            if (bg_home(1) < bg_total(1)) continue;   /* not all home yet */
                            pt[cur]--; borne++;
                        } else {
                            if (pt[to] < -1) continue;
                            if (pt[to] == -1) {
                                if (B->samedir) continue;             /* no hitting in Fevga */
                                if (!B->pin) { pt[to] = 0; pt[25]--; }
                                else continue;                        /* pinned, cannot land */
                            }
                            pt[cur]--;
                            pt[to]++;
                        }
                    }
                    moves++;
                    { int s2; for (s2 = 0; s2 + 1 < nd; s2++) d[s2] = d[s2 + 1]; }
                    nd--;
                }
                if (borne >= 15 || bg_total(1) == 0) { over = 1; youwin = 1; }
            }
            if (over) break;

            {   /* The opponent runs its men the other way. */
                int rolls[4], n2 = 0, i2, j2;
                rolls[0] = 1 + rnd(6); rolls[1] = 1 + rnd(6);
                n2 = (rolls[0] == rolls[1]) ? 4 : 2;
                if (n2 == 4) { rolls[2] = rolls[0]; rolls[3] = rolls[0]; }
                for (j2 = 0; j2 < n2; j2++) {
                    int moved = 0;
                    for (i2 = 1; i2 <= 24 && !moved; i2++) {
                        int to = i2 + rolls[j2];
                        if (pt[i2] >= 0) continue;
                        if (to > 24) { if (bg_home(-1) == bg_total(-1)) { pt[i2]++; theirborne++; moved = 1; } continue; }
                        if (pt[to] > 1) continue;
                        if (pt[to] == 1) { if (B->samedir || B->pin) continue; pt[to] = 0; pt[0]++; }
                        pt[i2]++;
                        pt[to]--;
                        moved = 1;
                    }
                }
                if (theirborne >= 15) { over = 1; youwin = 0; }
            }
        }
        scr_clear();
        draw_centered(12, 80, youwin ? "You bore off first." : "They bore off first.");
        score_report(p->title ? p->title : "backgammon", youwin ? 1000 : borne * 60);
        if (!confirm("\n  Another game?")) return;
    }
}

/* =========================================================== TILEPUZZLE */

void fam_tilepuzzle(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    int grid[8][8], cr = 0, cc = 0, moves, i, j;
    int tube[8][6], ntube = 6, depth = 4, sel = -1;
    const char *name = v == 0 ? "TILE ROTATE" : v == 1 ? "LOOP CLOSER" : v == 2 ? "FLIP GRID" :
                       v == 3 ? "BALL SORT" : v == 4 ? "COLOUR SORT" : "RUSH HOUR";

    if (v < 0 || v > 5) v = 0;

    for (;;) {
        int over = 0;
        moves = 0; cr = 0; cc = 0; sel = -1;

        if (v == 3 || v == 4) {
            /* Sorting puzzles: colours mixed across tubes, sort them out. */
            int pool[48], n = 0, colours = v == 3 ? 4 : 5;
            ntube = colours + 2;
            for (i = 0; i < colours; i++) for (j = 0; j < depth; j++) pool[n++] = i + 1;
            shuffle_int(pool, n);
            for (i = 0; i < ntube; i++) for (j = 0; j < depth; j++) tube[i][j] = 0;
            n = 0;
            for (i = 0; i < colours; i++) for (j = 0; j < depth; j++) tube[i][j] = pool[n++];
        } else {
            for (i = 0; i < 6; i++) for (j = 0; j < 6; j++)
                grid[i][j] = (v == 5) ? 0 : rnd(v == 2 ? 2 : 4);
            if (v == 5) {
                /* Rush Hour: the red car plus a few blockers. */
                grid[2][0] = 1; grid[2][1] = 1;
                for (i = 0; i < 5; i++) {
                    int r = rnd(6), c = rnd(5);
                    if (r == 2 && c < 2) continue;
                    grid[r][c] = 2; grid[r][c + 1] = 2;
                }
            }
        }

        while (!over) {
            int k;
            char sub[160];
            snprintf(sub, sizeof sub,
                     v == 3 || v == 4 ? "%s — arrows pick a tube, Enter lifts then pours, Q quits"
                   : v == 5           ? "%s — arrows move the red car, get it to the right edge, Q quits"
                                      : "%s — arrows move, Enter turns the tile, Q quits", name);
            draw_title("TILE PUZZLE", sub);

            if (v == 3 || v == 4) {
                for (i = 0; i < ntube; i++) {
                    draw_textf(4, 10 + i * 6, "%s tube %d %s",
                               i == cc ? BG_BLUE : "", i + 1, C_RESET);
                    for (j = depth - 1; j >= 0; j--) {
                        int col = tube[i][j];
                        draw_textf(6 + (depth - 1 - j), 10 + i * 6, "%s%s%s%s",
                                   i == sel ? BG_GREEN : "",
                                   col == 0 ? C_GREY : col == 1 ? C_RED : col == 2 ? C_GREEN :
                                   col == 3 ? C_YELLOW : col == 4 ? C_BLUE : C_MAGENTA,
                                   col ? " ## " : " .. ", C_RESET);
                    }
                }
                {   /* Solved when every tube holds one colour only. */
                    int done = 1;
                    for (i = 0; i < ntube; i++) {
                        int first = 0;
                        for (j = 0; j < depth; j++) {
                            if (!tube[i][j]) continue;
                            if (!first) first = tube[i][j];
                            else if (tube[i][j] != first) done = 0;
                        }
                    }
                    if (done) { over = 1; break; }
                }
            } else {
                static const char *FACE[4] = {"\\", "|", "/", "-"};
                for (i = 0; i < 6; i++) for (j = 0; j < 6; j++)
                    draw_textf(5 + i, 12 + j * 4, "%s%s%s%s",
                               (i == cr && j == cc) ? BG_BLUE : "",
                               v == 5 ? (grid[i][j] == 1 ? C_RED : grid[i][j] ? C_YELLOW : C_GREY)
                                      : (grid[i][j] ? C_GREEN : C_GREY),
                               v == 2 ? (grid[i][j] ? " ## " : " .. ")
                             : v == 5 ? (grid[i][j] == 1 ? " RR " : grid[i][j] ? " ## " : " .. ")
                                      : FACE[grid[i][j] % 4], C_RESET);
            }
            draw_textf(14, 10, "%smoves %d%s", C_GREY, moves, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "tilepuzzle", moves); return; }

            if (v == 3 || v == 4) {
                if (k == KEY_LEFT)  { cc = (cc + ntube - 1) % ntube; continue; }
                if (k == KEY_RIGHT) { cc = (cc + 1) % ntube; continue; }
                if (k != KEY_ENTER && k != ' ') continue;
                if (sel < 0) { sel = cc; continue; }
                {   /* Pour the top ball of `sel` onto `cc` when the colours agree. */
                    int from = sel, to = cc, fi = -1, ti = -1;
                    sel = -1;
                    if (from == to) continue;
                    for (j = depth - 1; j >= 0; j--) if (tube[from][j]) { fi = j; break; }
                    for (j = 0; j < depth; j++) if (!tube[to][j]) { ti = j; break; }
                    if (fi < 0 || ti < 0) continue;
                    if (ti > 0 && tube[to][ti - 1] != tube[from][fi]) continue;
                    tube[to][ti] = tube[from][fi];
                    tube[from][fi] = 0;
                    moves++;
                }
                continue;
            }

            if (v == 5) {
                if (k == KEY_RIGHT) {
                    int rr = -1, c2 = -1;
                    for (i = 0; i < 6; i++) for (j = 0; j < 6; j++) if (grid[i][j] == 1 && j > c2) { rr = i; c2 = j; }
                    if (rr >= 0 && c2 < 5 && !grid[rr][c2 + 1]) {
                        int lead = c2 + 1, tail = c2 - 1;
                        grid[rr][lead] = 1;
                        grid[rr][tail] = 0;
                        moves++;
                        if (lead == 5) { over = 1; break; }
                    }
                }
                continue;
            }

            if (k == KEY_UP)    { if (cr > 0) cr--; continue; }
            if (k == KEY_DOWN)  { if (cr < 5) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (cc < 5) cc++; continue; }
            if (k != KEY_ENTER && k != ' ') continue;

            if (v == 2) {
                /* Flip Grid: turning a cell turns its orthogonal neighbours too. */
                grid[cr][cc] ^= 1;
                if (cr > 0) grid[cr - 1][cc] ^= 1;
                if (cr < 5) grid[cr + 1][cc] ^= 1;
                if (cc > 0) grid[cr][cc - 1] ^= 1;
                if (cc < 5) grid[cr][cc + 1] ^= 1;
                moves++;
                {
                    int done = 1;
                    for (i = 0; i < 6; i++) for (j = 0; j < 6; j++) if (grid[i][j]) done = 0;
                    if (done) over = 1;
                }
            } else {
                grid[cr][cc] = (grid[cr][cc] + 1) % 4;
                moves++;
                {   /* Rotate and Loop Closer both want every tile at zero. */
                    int done = 1;
                    for (i = 0; i < 6; i++) for (j = 0; j < 6; j++) if (grid[i][j]) done = 0;
                    if (done) over = 1;
                }
            }
        }
        scr_clear();
        draw_centered(12, 80, "Solved.");
        score_report(p->title ? p->title : "tilepuzzle", 2000 - moves * 5);
        if (!confirm("\n  Another puzzle?")) return;
    }
}
