/* GIC:PARAMETERISED go
 * GIC:PARAMETERISED morris
 * GIC:PARAMETERISED tafl
 *
 * classic.c - three old board families that share a grid and a capture idea.
 *
 * They are in one file because the capture test is the same shape in all
 * three: find a connected group, count what surrounds it, remove it if it is
 * fully enclosed. What differs is what counts as a group and what counts as
 * enclosure.
 *
 *   go      params: size = board, variant 0 area scoring, 1 first capture wins
 *           A group is a set of orthogonally-joined stones; it dies when it
 *           has no liberties. Suicide is illegal and the simple ko rule is
 *           enforced by remembering the previous position.
 *   morris  params: count = men per player (3, 6, 9 or 12)
 *           Points and lines rather than a grid; closing a mill removes an
 *           enemy man. With three men left a player may fly anywhere.
 *   tafl    params: size = board, variant = the setup
 *           Asymmetric: the king escapes to a corner or edge, the besiegers
 *           capture by sandwiching. Nothing else in the suite is asymmetric.
 */
#include "engine.h"
#include "games.h"

/* =================================================================== GO */

#define GMAX 19
#define GCELL (GMAX * GMAX)

static int gb[GCELL], gprev[GCELL], GN;
static int gcap[3];

static int gidx(int r, int c) { return r * GN + c; }
static int gon(int r, int c) { return r >= 0 && r < GN && c >= 0 && c < GN; }

/* Collect the group at (r,c) and count its liberties. */
static int group_at(int r, int c, int *cells, int *ncells)
{
    int stack[GCELL], sp = 0, seen[GCELL], i, libs = 0, who = gb[gidx(r, c)];
    static const int DR[4] = {-1, 1, 0, 0}, DC[4] = {0, 0, -1, 1};
    for (i = 0; i < GCELL; i++) seen[i] = 0;
    *ncells = 0;
    if (!who) return 0;
    stack[sp++] = gidx(r, c);
    seen[gidx(r, c)] = 1;
    while (sp) {
        int cell = stack[--sp], cr = cell / GN, cc = cell % GN, d;
        cells[(*ncells)++] = cell;
        for (d = 0; d < 4; d++) {
            int nr = cr + DR[d], nc = cc + DC[d], ni;
            if (!gon(nr, nc)) continue;
            ni = gidx(nr, nc);
            if (gb[ni] == 0) { if (!seen[ni]) { seen[ni] = 1; libs++; } continue; }
            if (gb[ni] == who && !seen[ni]) { seen[ni] = 1; stack[sp++] = ni; }
        }
    }
    return libs;
}

static int capture_around(int r, int c, int who)
{
    static const int DR[4] = {-1, 1, 0, 0}, DC[4] = {0, 0, -1, 1};
    int cells[GCELL], n, d, taken = 0;
    for (d = 0; d < 4; d++) {
        int nr = r + DR[d], nc = c + DC[d], i;
        if (!gon(nr, nc) || gb[gidx(nr, nc)] != 3 - who) continue;
        if (group_at(nr, nc, cells, &n) == 0) {
            for (i = 0; i < n; i++) gb[cells[i]] = 0;
            taken += n;
        }
    }
    return taken;
}

/* Area scoring: stones on the board plus empty points reached only by one
 * colour. */
static void go_score(int *black, int *white)
{
    int seen[GCELL], stack[GCELL], sp, i, r, c;
    static const int DR[4] = {-1, 1, 0, 0}, DC[4] = {0, 0, -1, 1};
    *black = 0; *white = 0;
    for (i = 0; i < GCELL; i++) seen[i] = 0;
    for (i = 0; i < GN * GN; i++) { if (gb[i] == 1) (*black)++; if (gb[i] == 2) (*white)++; }
    for (r = 0; r < GN; r++) for (c = 0; c < GN; c++) {
        int nreg = 0, touch = 0;
        if (gb[gidx(r, c)] || seen[gidx(r, c)]) continue;
        sp = 0;
        stack[sp++] = gidx(r, c);
        seen[gidx(r, c)] = 1;
        while (sp) {
            int cell = stack[--sp], cr = cell / GN, cc = cell % GN, d;
            nreg++;
            for (d = 0; d < 4; d++) {
                int nr = cr + DR[d], nc = cc + DC[d], ni;
                if (!gon(nr, nc)) continue;
                ni = gidx(nr, nc);
                if (gb[ni]) { touch |= gb[ni]; continue; }
                if (!seen[ni]) { seen[ni] = 1; stack[sp++] = ni; }
            }
        }
        if (touch == 1) *black += nreg;
        else if (touch == 2) *white += nreg;
    }
}

/* The opponent plays a plain but sane game: capture when it can, else take a
 * point with the most friendly neighbours that is not self-atari. */
static int go_ai(void)
{
    int r, c, best = -1, bestscore = -9999;
    static const int DR[4] = {-1, 1, 0, 0}, DC[4] = {0, 0, -1, 1};
    for (r = 0; r < GN; r++) for (c = 0; c < GN; c++) {
        int cells[GCELL], n, score = 0, d, save[GCELL], i;
        if (gb[gidx(r, c)]) continue;
        for (i = 0; i < GN * GN; i++) save[i] = gb[i];
        gb[gidx(r, c)] = 2;
        score += capture_around(r, c, 2) * 12;
        if (group_at(r, c, cells, &n) == 0) score -= 50;       /* suicide */
        for (d = 0; d < 4; d++) {
            int nr = r + DR[d], nc = c + DC[d];
            if (!gon(nr, nc)) { score += 1; continue; }
            if (gb[gidx(nr, nc)] == 2) score += 2;
            if (gb[gidx(nr, nc)] == 1) score += 3;             /* contact play */
        }
        score += rnd(3);
        for (i = 0; i < GN * GN; i++) gb[i] = save[i];
        if (score > bestscore) { bestscore = score; best = gidx(r, c); }
    }
    return best;
}

void fam_go(const GParams *p)
{
    int atari = gp_int(p->variant, 0) == 1;
    int cr, cc, passes, i;

    GN = gp_int(p->size, 9);
    if (GN < 5) GN = 5;
    if (GN > GMAX) GN = GMAX;

    for (;;) {
        int over = 0;
        for (i = 0; i < GCELL; i++) { gb[i] = 0; gprev[i] = 0; }
        gcap[1] = gcap[2] = 0;
        cr = GN / 2; cc = GN / 2; passes = 0;

        while (!over) {
            int k, r, c, black, white;
            char sub[150];
            snprintf(sub, sizeof sub,
                     "%s %dx%d — arrows move, Enter places, P passes, Q quits",
                     atari ? "Atari Go (first capture wins)" : "Go", GN, GN);
            draw_title("GO", sub);
            for (r = 0; r < GN; r++) for (c = 0; c < GN; c++) {
                int v = gb[gidx(r, c)];
                draw_textf(4 + r, 6 + c * 2, "%s%s%c%s", (r == cr && c == cc) ? BG_BLUE : "",
                           v == 1 ? C_GREEN : v == 2 ? C_RED : C_GREY,
                           v == 1 ? 'O' : v == 2 ? 'X' : '.', C_RESET);
            }
            go_score(&black, &white);
            draw_textf(5 + GN, 6, "%scaptures: you %d, them %d%s", C_GREY, gcap[1], gcap[2], C_RESET);
            if (!atari)
                draw_textf(6 + GN, 6, "%sarea: you %d, them %d%s", C_GREY, black, white, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "go", gcap[1] * 10); return; }
            if (k == KEY_UP)    { if (cr > 0) cr--; continue; }
            if (k == KEY_DOWN)  { if (cr < GN - 1) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (cc < GN - 1) cc++; continue; }
            if (k == 'p' || k == 'P') { passes++; if (passes >= 2) over = 1; }
            else if (k == KEY_ENTER || k == ' ') {
                int cells[GCELL], n, save[GCELL], taken;
                if (gb[gidx(cr, cc)]) continue;
                for (i = 0; i < GN * GN; i++) save[i] = gb[i];
                gb[gidx(cr, cc)] = 1;
                taken = capture_around(cr, cc, 1);
                if (group_at(cr, cc, cells, &n) == 0 && taken == 0) {
                    for (i = 0; i < GN * GN; i++) gb[i] = save[i];   /* suicide */
                    continue;
                }
                /* Simple ko: the position may not repeat the previous one. */
                {
                    int same = 1;
                    for (i = 0; i < GN * GN; i++) if (gb[i] != gprev[i]) { same = 0; break; }
                    if (same) { for (i = 0; i < GN * GN; i++) gb[i] = save[i]; continue; }
                }
                for (i = 0; i < GN * GN; i++) gprev[i] = save[i];
                gcap[1] += taken;
                passes = 0;
                if (atari && taken) { over = 1; break; }
            } else continue;

            if (!over) {
                int m = go_ai(), taken;
                if (m < 0) { over = 1; break; }
                gb[m] = 2;
                taken = capture_around(m / GN, m % GN, 2);
                gcap[2] += taken;
                if (atari && taken) { over = 1; break; }
            }
        }
        {
            int black, white;
            go_score(&black, &white);
            scr_clear();
            if (atari)
                draw_centered(12, 80, gcap[1] ? "You made the first capture." : "They made the first capture.");
            else
                draw_textf(12, 20, "%sArea: you %d, them %d — %s%s", C_BOLD, black, white,
                           black > white ? "you win" : black == white ? "a draw" : "they win", C_RESET);
            score_report(p->title ? p->title : "go", atari ? (gcap[1] ? 500 : 100) : black * 10);
            scr_flush();
        }
        if (!confirm("\n  Another game?")) return;
    }
}

/* =============================================================== MORRIS */

/* Each board is a list of points and the lines (mills) joining them. */
typedef struct { int npts; int nmill; const int (*mill)[3]; const char *adj; int men; } MBoard;

static const int M3[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}
};
static const int M9[16][3] = {
    {0,1,2},{3,4,5},{6,7,8},{9,10,11},{12,13,14},{15,16,17},{18,19,20},{21,22,23},
    {0,9,21},{3,10,18},{6,11,15},{1,4,7},{16,19,22},{8,12,17},{5,13,20},{2,14,23}
};
static const int M6[8][3] = {
    {0,1,2},{3,4,5},{10,11,12},{13,14,15},{0,3,10},{2,5,12},{6,7,8},{9,9,9}
};
static const int M12[20][3] = {
    {0,1,2},{3,4,5},{6,7,8},{9,10,11},{12,13,14},{15,16,17},{18,19,20},{21,22,23},
    {0,9,21},{3,10,18},{6,11,15},{1,4,7},{16,19,22},{8,12,17},{5,13,20},{2,14,23},
    {0,3,6},{2,5,8},{15,18,21},{17,20,23}
};

/* Point coordinates for drawing the classic nine-men board. */
static const int MX[24] = {0,3,6,1,3,5,2,3,4,0,1,2,4,5,6,2,3,4,1,3,5,0,3,6};
static const int MY[24] = {0,0,0,1,1,1,2,2,2,3,3,3,3,3,3,4,4,4,5,5,5,6,6,6};

static int mb[24];

static int in_mill(int pt, int who, const int (*mills)[3], int nm)
{
    int i;
    for (i = 0; i < nm; i++) {
        if (mills[i][0] == mills[i][2]) continue;                /* padding row */
        if (mills[i][0] != pt && mills[i][1] != pt && mills[i][2] != pt) continue;
        if (mb[mills[i][0]] == who && mb[mills[i][1]] == who && mb[mills[i][2]] == who) return 1;
    }
    return 0;
}

static int adjacent(int a, int b, const int (*mills)[3], int nm)
{
    int i, j;
    for (i = 0; i < nm; i++) {
        if (mills[i][0] == mills[i][2]) continue;
        for (j = 0; j < 2; j++)
            if ((mills[i][j] == a && mills[i][j + 1] == b) ||
                (mills[i][j] == b && mills[i][j + 1] == a)) return 1;
    }
    return 0;
}

void fam_morris(const GParams *p)
{
    int men = gp_int(p->count, 9);
    const int (*mills)[3];
    int npts, nm, i, cur = 0, sel = -1;
    int placed[3], onboard[3], turn = 1;

    if (men != 3 && men != 6 && men != 9 && men != 12) men = 9;
    if (men == 3)       { mills = M3;  nm = 8;  npts = 9;  }
    else if (men == 6)  { mills = M6;  nm = 7;  npts = 16; }
    else if (men == 12) { mills = M12; nm = 20; npts = 24; }
    else                { mills = M9;  nm = 16; npts = 24; }

    for (;;) {
        int over = 0, winner = 0;
        for (i = 0; i < 24; i++) mb[i] = 0;
        placed[1] = placed[2] = 0;
        onboard[1] = onboard[2] = 0;
        turn = 1; cur = 0; sel = -1;

        while (!over) {
            int k, phase = (placed[turn] < men) ? 0 : 1;
            int flying = (onboard[turn] == 3 && placed[turn] >= men);
            char sub[160];

            snprintf(sub, sizeof sub,
                     "%d men's morris — %s, arrows move, Enter %s, Q quits",
                     men, phase == 0 ? "placing" : flying ? "flying (any empty point)" : "moving",
                     phase == 0 ? "places" : "picks then puts");
            draw_title("MORRIS", sub);
            for (i = 0; i < npts && i < 24; i++) {
                int v = mb[i];
                draw_textf(4 + MY[i] * 2, 10 + MX[i] * 5,
                           "%s%s%c%s", (i == cur) ? BG_BLUE : (i == sel ? BG_GREEN : ""),
                           v == 1 ? C_GREEN : v == 2 ? C_RED : C_GREY,
                           v == 1 ? 'O' : v == 2 ? 'X' : '+', C_RESET);
            }
            draw_textf(18, 6, "%syou: %d placed, %d on board   them: %d placed, %d on board%s",
                       C_GREY, placed[1], onboard[1], placed[2], onboard[2], C_RESET);
            draw_textf(19, 6, "%s%s to move%s", C_WHITE, turn == 1 ? "You" : "They", C_RESET);
            scr_flush();

            if (turn == 2) {
                /* The opponent places or moves to complete or block a mill. */
                int done = 0;
                sleep_ms(300);
                if (placed[2] < men) {
                    for (i = 0; i < npts && !done; i++) {
                        if (mb[i]) continue;
                        mb[i] = 2;
                        if (in_mill(i, 2, mills, nm)) { done = 1; }
                        else { mb[i] = 1; if (in_mill(i, 1, mills, nm)) { mb[i] = 2; done = 1; } else mb[i] = 0; }
                    }
                    if (!done) for (i = 0; i < npts; i++) if (!mb[i]) { mb[i] = 2; break; }
                    placed[2]++; onboard[2]++;
                } else {
                    int from, to, moved = 0;
                    for (from = 0; from < npts && !moved; from++) {
                        if (mb[from] != 2) continue;
                        for (to = 0; to < npts && !moved; to++) {
                            if (mb[to]) continue;
                            if (onboard[2] > 3 && !adjacent(from, to, mills, nm)) continue;
                            mb[from] = 0; mb[to] = 2; moved = 1;
                        }
                    }
                    if (!moved) { over = 1; winner = 1; break; }
                }
                /* A completed mill removes one of your men. */
                for (i = 0; i < npts; i++)
                    if (mb[i] == 2 && in_mill(i, 2, mills, nm)) {
                        int j;
                        for (j = 0; j < npts; j++)
                            if (mb[j] == 1 && !in_mill(j, 1, mills, nm)) { mb[j] = 0; onboard[1]--; break; }
                        break;
                    }
                if (onboard[1] < 3 && placed[1] >= men) { over = 1; winner = 2; }
                turn = 1;
                continue;
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "morris", onboard[1] * 50); return; }
            if (k == KEY_LEFT)  { cur = (cur + npts - 1) % npts; continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % npts; continue; }
            if (k == KEY_UP)    { cur = (cur + npts - 3) % npts; continue; }
            if (k == KEY_DOWN)  { cur = (cur + 3) % npts; continue; }
            if (k != KEY_ENTER && k != ' ') continue;

            if (phase == 0) {
                if (mb[cur]) continue;
                mb[cur] = 1;
                placed[1]++; onboard[1]++;
            } else if (sel < 0) {
                if (mb[cur] != 1) continue;
                sel = cur;
                continue;
            } else {
                if (mb[cur]) { sel = -1; continue; }
                if (!flying && !adjacent(sel, cur, mills, nm)) { sel = -1; continue; }
                mb[sel] = 0; mb[cur] = 1;
                sel = -1;
            }
            /* Your mill takes one of theirs. */
            if (in_mill(cur, 1, mills, nm)) {
                for (i = 0; i < npts; i++)
                    if (mb[i] == 2 && !in_mill(i, 2, mills, nm)) { mb[i] = 0; onboard[2]--; break; }
            }
            if (onboard[2] < 3 && placed[2] >= men) { over = 1; winner = 1; }
            turn = 2;
        }
        scr_clear();
        draw_centered(12, 80, winner == 1 ? "You reduced them below three." : "They reduced you below three.");
        score_report(p->title ? p->title : "morris", winner == 1 ? 500 : 100);
        if (!confirm("\n  Another game?")) return;
    }
}

/* ================================================================= TAFL */

#define TMAX 19

static int tb[TMAX * TMAX], TN;      /* 0 empty, 1 defender, 2 attacker, 3 king */

static int tidx(int r, int c) { return r * TN + c; }
static int ton(int r, int c) { return r >= 0 && r < TN && c >= 0 && c < TN; }
static int corner(int r, int c)
{
    return (r == 0 || r == TN - 1) && (c == 0 || c == TN - 1);
}

static void tafl_setup(int variant)
{
    int r, c, mid = TN / 2, i;
    for (i = 0; i < TN * TN; i++) tb[i] = 0;
    tb[tidx(mid, mid)] = 3;
    /* Defenders in a cross around the king, attackers in four groups on the
     * edges. The arm length scales with the board, which is what makes
     * Brandubh, Tablut, Hnefatafl and Alea Evangelii different games. */
    {
        int arm = TN <= 7 ? 1 : TN <= 9 ? 2 : TN <= 11 ? 2 : 3;
        int att = TN <= 7 ? 2 : TN <= 9 ? 3 : TN <= 11 ? 4 : 6;
        for (i = 1; i <= arm; i++) {
            tb[tidx(mid - i, mid)] = 1; tb[tidx(mid + i, mid)] = 1;
            tb[tidx(mid, mid - i)] = 1; tb[tidx(mid, mid + i)] = 1;
        }
        if (TN >= 11) { tb[tidx(mid-1, mid-1)] = 1; tb[tidx(mid+1, mid+1)] = 1;
                        tb[tidx(mid-1, mid+1)] = 1; tb[tidx(mid+1, mid-1)] = 1; }
        for (i = 0; i < att; i++) {
            int off = i - att / 2;
            if (ton(0, mid + off))      tb[tidx(0, mid + off)] = 2;
            if (ton(TN - 1, mid + off)) tb[tidx(TN - 1, mid + off)] = 2;
            if (ton(mid + off, 0))      tb[tidx(mid + off, 0)] = 2;
            if (ton(mid + off, TN - 1)) tb[tidx(mid + off, TN - 1)] = 2;
        }
        if (variant != 1) {
            if (ton(1, mid))      tb[tidx(1, mid)] = 2;
            if (ton(TN - 2, mid)) tb[tidx(TN - 2, mid)] = 2;
            if (ton(mid, 1))      tb[tidx(mid, 1)] = 2;
            if (ton(mid, TN - 2)) tb[tidx(mid, TN - 2)] = 2;
        }
    }
    (void)r; (void)c;
}

static int is_attacker(int v) { return v == 2; }
static int is_defender(int v) { return v == 1 || v == 3; }

/* A man is taken when sandwiched between two enemies; the king needs four. */
static void tafl_captures(int r, int c, int mover)
{
    static const int DR[4] = {-1, 1, 0, 0}, DC[4] = {0, 0, -1, 1};
    int d;
    for (d = 0; d < 4; d++) {
        int mr = r + DR[d], mc = c + DC[d];
        int fr = r + DR[d] * 2, fc = c + DC[d] * 2;
        int victim, beyond;
        if (!ton(mr, mc) || !ton(fr, fc)) continue;
        victim = tb[tidx(mr, mc)];
        beyond = tb[tidx(fr, fc)];
        if (!victim) continue;
        if (mover == 2 && is_defender(victim)) {
            if (victim == 3) continue;                    /* the king needs four */
            if (is_attacker(beyond) || corner(fr, fc)) tb[tidx(mr, mc)] = 0;
        } else if (mover == 1 && is_attacker(victim)) {
            if (is_defender(beyond) || corner(fr, fc)) tb[tidx(mr, mc)] = 0;
        }
    }
}

static int king_taken(void)
{
    static const int DR[4] = {-1, 1, 0, 0}, DC[4] = {0, 0, -1, 1};
    int r, c, d, around;
    for (r = 0; r < TN; r++) for (c = 0; c < TN; c++) {
        if (tb[tidx(r, c)] != 3) continue;
        around = 0;
        for (d = 0; d < 4; d++) {
            int nr = r + DR[d], nc = c + DC[d];
            if (!ton(nr, nc)) { around++; continue; }        /* the edge helps */
            if (tb[tidx(nr, nc)] == 2) around++;
        }
        return around >= 4;
    }
    return 1;                                                /* no king: taken */
}

static int king_escaped(void)
{
    int r, c;
    for (r = 0; r < TN; r++) for (c = 0; c < TN; c++)
        if (tb[tidx(r, c)] == 3 && corner(r, c)) return 1;
    return 0;
}

void fam_tafl(const GParams *p)
{
    int variant = gp_int(p->variant, 0), cr, cc, sel = -1, moves;

    TN = gp_int(p->size, 11);
    if (TN < 7) TN = 7;
    if (TN > TMAX) TN = TMAX;

    for (;;) {
        int over = 0, defwin = 0;
        tafl_setup(variant);
        cr = TN / 2; cc = TN / 2; sel = -1; moves = 0;

        while (!over) {
            int k, r, c;
            char sub[150];
            snprintf(sub, sizeof sub,
                     "%dx%d — you defend: get the king (K) to a corner. "
                     "Arrows move, Enter picks then puts, Q quits", TN, TN);
            draw_title("TAFL", sub);
            for (r = 0; r < TN; r++) for (c = 0; c < TN; c++) {
                int v = tb[tidx(r, c)];
                const char *bg = (r == cr && c == cc) ? BG_BLUE
                               : (sel >= 0 && sel == tidx(r, c)) ? BG_GREEN
                               : corner(r, c) ? BG_GREEN : "";
                draw_textf(4 + r, 6 + c * 2, "%s%s%c%s", bg,
                           v == 3 ? C_YELLOW : v == 1 ? C_GREEN : v == 2 ? C_RED : C_GREY,
                           v == 3 ? 'K' : v == 1 ? 'O' : v == 2 ? 'X' : '.', C_RESET);
            }
            draw_textf(5 + TN, 6, "%smoves %d — corners are highlighted%s", C_GREY, moves, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "tafl", moves); return; }
            if (k == KEY_UP)    { if (cr > 0) cr--; continue; }
            if (k == KEY_DOWN)  { if (cr < TN - 1) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (cc < TN - 1) cc++; continue; }
            if (k != KEY_ENTER && k != ' ') continue;

            if (sel < 0) {
                if (!is_defender(tb[tidx(cr, cc)])) continue;
                sel = tidx(cr, cc);
                continue;
            }
            {
                int sr = sel / TN, sc = sel % TN, step, ok = 1;
                if (tb[tidx(cr, cc)]) { sel = -1; continue; }
                if (sr != cr && sc != cc) { sel = -1; continue; }   /* rook moves only */
                if (corner(cr, cc) && tb[sel] != 3) { sel = -1; continue; }
                if (sr == cr) for (step = (sc < cc ? 1 : -1); sc + step != cc; step += (sc < cc ? 1 : -1))
                    if (tb[tidx(sr, sc + step)]) ok = 0;
                if (sc == cc) for (step = (sr < cr ? 1 : -1); sr + step != cr; step += (sr < cr ? 1 : -1))
                    if (tb[tidx(sr + step, sc)]) ok = 0;
                if (!ok) { sel = -1; continue; }
                tb[tidx(cr, cc)] = tb[sel];
                tb[sel] = 0;
                sel = -1;
                moves++;
                tafl_captures(cr, cc, 1);
            }
            if (king_escaped()) { over = 1; defwin = 1; break; }

            {   /* The attackers move towards the king and take what they can.
                 * Each candidate is tried on a saved copy of the board, because
                 * a capture mutates squares the search would otherwise carry
                 * into the next candidate. */
                static int save[TMAX * TMAX];
                int r2, c2, i2, bestfrom = -1, bestto = -1, bestscore = -99999;
                int kr = -1, kc = -1;
                static const int DR[4] = {-1, 1, 0, 0}, DC[4] = {0, 0, -1, 1};

                for (r2 = 0; r2 < TN; r2++) for (c2 = 0; c2 < TN; c2++)
                    if (tb[tidx(r2, c2)] == 3) { kr = r2; kc = c2; }
                if (kr < 0) { over = 1; defwin = 0; break; }

                for (i2 = 0; i2 < TN * TN; i2++) save[i2] = tb[i2];

                for (r2 = 0; r2 < TN; r2++) for (c2 = 0; c2 < TN; c2++) {
                    int dir;
                    if (tb[tidx(r2, c2)] != 2) continue;
                    for (dir = 0; dir < 4; dir++) {
                        int nr = r2, nc = c2, dist;
                        for (dist = 1; dist < TN; dist++) {
                            int s, before = 0, after = 0;
                            nr += DR[dir]; nc += DC[dir];
                            if (!ton(nr, nc) || tb[tidx(nr, nc)]) break;
                            if (corner(nr, nc)) continue;      /* only the king may sit there */

                            tb[tidx(r2, c2)] = 0;
                            tb[tidx(nr, nc)] = 2;
                            for (i2 = 0; i2 < TN * TN; i2++) if (tb[i2] == 1) before++;
                            tafl_captures(nr, nc, 2);
                            for (i2 = 0; i2 < TN * TN; i2++) if (tb[i2] == 1) after++;
                            s = (before - after) * 20 - (abs(nr - kr) + abs(nc - kc));
                            if (king_taken()) s += 1000;
                            for (i2 = 0; i2 < TN * TN; i2++) tb[i2] = save[i2];

                            if (s > bestscore) { bestscore = s; bestfrom = tidx(r2, c2); bestto = tidx(nr, nc); }
                        }
                    }
                }
                if (bestfrom < 0) { over = 1; defwin = 1; break; }
                tb[bestto] = tb[bestfrom];
                tb[bestfrom] = 0;
                tafl_captures(bestto / TN, bestto % TN, 2);
                moves++;
                if (king_taken()) { over = 1; defwin = 0; }
            }
        }
        scr_clear();
        draw_centered(12, 80, defwin ? "The king reached a corner." : "The king was taken.");
        score_report(p->title ? p->title : "tafl", defwin ? 500 : moves * 5);
        if (!confirm("\n  Another game?")) return;
    }
}
