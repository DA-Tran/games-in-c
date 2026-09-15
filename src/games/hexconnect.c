/* GIC:PARAMETERISED hexconnect
 * hexconnect.c - the connection family.
 *
 * params: size sets the board, variant selects the winning condition
 *
 * All four games are played on a hexagonally-connected grid and all four are
 * won by joining things up, so the board, the adjacency and the union-find
 * are written once. What differs is only the question asked of a finished
 * group:
 *
 *   Hex        does one player's group touch both of their two opposite sides
 *   Y          does a group touch all three sides of the triangle
 *   Havannah   does a group form a ring, a bridge between two corners, or a
 *              fork touching three edges
 *   TwixT      played on a square grid with knight's-move links instead
 *
 * Hex and Y can never be drawn, which is worth knowing while playing: if the
 * board fills, somebody has connected.
 */
#include "engine.h"
#include "games.h"

#define MAXN 19
#define MAXC (MAXN * MAXN)

enum { V_HEX, V_Y, V_HAVANNAH, V_TWIXT };

static int board[MAXC];          /* 0 empty, 1 you, 2 them */
static int parent[MAXC + 8];
static int N, VAR;

/* The six hex directions on a rhombus laid out in rows. */
static const int DR[6] = {-1, -1,  0, 0,  1, 1};
static const int DC[6] = { 0,  1, -1, 1, -1, 0};

static int idx(int r, int c) { return r * N + c; }
static int on(int r, int c)
{
    if (r < 0 || r >= N || c < 0) return 0;
    if (VAR == V_Y) return c <= r;            /* triangle */
    return c < N;
}

static int find(int a) { while (parent[a] != a) { parent[a] = parent[parent[a]]; a = parent[a]; } return a; }
static void unite(int a, int b) { a = find(a); b = find(b); if (a != b) parent[a] = b; }

static void rebuild(int who)
{
    int r, c, d, i;
    for (i = 0; i < MAXC + 8; i++) parent[i] = i;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (!on(r, c) || board[idx(r, c)] != who) continue;
        for (d = 0; d < 6; d++) {
            int nr = r + DR[d], nc = c + DC[d];
            if (on(nr, nc) && board[idx(nr, nc)] == who) unite(idx(r, c), idx(nr, nc));
        }
    }
}

/* Virtual nodes for the edges, so "touches this side" is one union-find query. */
#define E_TOP    (MAXC + 0)
#define E_BOTTOM (MAXC + 1)
#define E_LEFT   (MAXC + 2)
#define E_RIGHT  (MAXC + 3)

static int hex_win(int who)
{
    int r, c;
    rebuild(who);
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (board[idx(r, c)] != who) continue;
        if (who == 1) { if (r == 0) unite(idx(r, c), E_TOP); if (r == N - 1) unite(idx(r, c), E_BOTTOM); }
        else          { if (c == 0) unite(idx(r, c), E_LEFT); if (c == N - 1) unite(idx(r, c), E_RIGHT); }
    }
    return who == 1 ? find(E_TOP) == find(E_BOTTOM) : find(E_LEFT) == find(E_RIGHT);
}

/* Y: the three sides of the triangle are row 0, column 0 and the diagonal. */
static int y_win(int who)
{
    int r, c;
    rebuild(who);
    for (r = 0; r < N; r++) for (c = 0; c <= r; c++) {
        if (board[idx(r, c)] != who) continue;
        if (r == 0 || c == 0 || c == r) {
            if (r == 0)     unite(idx(r, c), E_TOP);
            if (c == 0)     unite(idx(r, c), E_LEFT);
            if (c == r)     unite(idx(r, c), E_RIGHT);
        }
    }
    return find(E_TOP) == find(E_LEFT) && find(E_LEFT) == find(E_RIGHT);
}

/* Havannah: a ring (a loop around at least one cell), a bridge joining two of
 * the six corners, or a fork touching three of the six edges. */
static int havannah_win(int who, const char **how)
{
    int r, c, corners, edges, i;
    rebuild(who);

    /* Count the corners and edges each group touches. */
    for (i = 0; i < MAXC; i++) {
        int root = -1;
        if (board[i] != who) continue;
        root = find(i);
        corners = 0; edges = 0;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
            if (board[idx(r, c)] != who || find(idx(r, c)) != root) continue;
            if ((r == 0 || r == N - 1) && (c == 0 || c == N - 1)) corners++;
            else if (r == 0 || r == N - 1 || c == 0 || c == N - 1) edges++;
        }
        if (corners >= 2) { *how = "a bridge between two corners"; return 1; }
        if (edges   >= 3) { *how = "a fork touching three edges";  return 1; }
    }
    /* A ring: an empty or enemy cell that cannot reach the border through
     * cells this player does not own. */
    {
        int seen[MAXC], stack[MAXC], sp, esc;
        for (i = 0; i < MAXC; i++) seen[i] = 0;
        for (r = 1; r < N - 1; r++) for (c = 1; c < N - 1; c++) {
            if (board[idx(r, c)] == who || seen[idx(r, c)]) continue;
            sp = 0; esc = 0;
            stack[sp++] = idx(r, c);
            seen[idx(r, c)] = 1;
            while (sp) {
                int cell = stack[--sp], cr = cell / N, cc = cell % N, d;
                if (cr == 0 || cr == N - 1 || cc == 0 || cc == N - 1) esc = 1;
                for (d = 0; d < 6; d++) {
                    int nr = cr + DR[d], nc = cc + DC[d];
                    if (!on(nr, nc) || seen[idx(nr, nc)] || board[idx(nr, nc)] == who) continue;
                    seen[idx(nr, nc)] = 1;
                    stack[sp++] = idx(nr, nc);
                }
            }
            if (!esc) { *how = "a ring"; return 1; }
        }
    }
    return 0;
}

/* TwixT: pegs on a square grid joined by knight's moves that must not cross. */
static const int KR[8] = {-2,-2,-1,-1, 1, 1, 2, 2};
static const int KC[8] = {-1, 1,-2, 2,-2, 2,-1, 1};

static int twixt_win(int who)
{
    int r, c, d, i;
    for (i = 0; i < MAXC + 8; i++) parent[i] = i;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (board[idx(r, c)] != who) continue;
        for (d = 0; d < 8; d++) {
            int nr = r + KR[d], nc = c + KC[d];
            if (nr >= 0 && nr < N && nc >= 0 && nc < N && board[idx(nr, nc)] == who)
                unite(idx(r, c), idx(nr, nc));
        }
        if (who == 1) { if (r == 0) unite(idx(r, c), E_TOP); if (r == N - 1) unite(idx(r, c), E_BOTTOM); }
        else          { if (c == 0) unite(idx(r, c), E_LEFT); if (c == N - 1) unite(idx(r, c), E_RIGHT); }
    }
    return who == 1 ? find(E_TOP) == find(E_BOTTOM) : find(E_LEFT) == find(E_RIGHT);
}

static int wins(int who, const char **how)
{
    *how = "a connection";
    switch (VAR) {
    case V_HEX:      return hex_win(who);
    case V_Y:        return y_win(who);
    case V_HAVANNAH: return havannah_win(who, how);
    default:         return twixt_win(who);
    }
}

/* The opponent plays the move that most shortens its own path, measured by a
 * breadth-first search that treats its own stones as free. */
static int path_cost(int who)
{
    /* Weights are 0 for a cell this player already owns and 1 for an empty
     * one, so this is a 0-1 shortest path. Relaxation can reach a cell more
     * than once, so an "already queued" flag keeps the ring buffer bounded --
     * without it the queue overran its array on a 19x19 board. */
    static int dist[MAXC], inq[MAXC], q[MAXC + 1];
    int head = 0, tail = 0, count = 0, r, c, bestgoal = 9999;

    for (r = 0; r < MAXC; r++) { dist[r] = 9999; inq[r] = 0; }
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        int start = (who == 1) ? (r == 0) : (c == 0);
        if (!on(r, c) || !start || board[idx(r, c)] == 3 - who) continue;
        dist[idx(r, c)] = (board[idx(r, c)] == who) ? 0 : 1;
        if (!inq[idx(r, c)]) {
            inq[idx(r, c)] = 1;
            q[tail] = idx(r, c);
            tail = (tail + 1) % (MAXC + 1);
            count++;
        }
    }
    while (count > 0) {
        int cell = q[head], cr, cc, d;
        head = (head + 1) % (MAXC + 1);
        count--;
        inq[cell] = 0;
        cr = cell / N; cc = cell % N;
        for (d = 0; d < 6; d++) {
            int nr = cr + DR[d], nc = cc + DC[d], w, ni;
            if (!on(nr, nc) || board[idx(nr, nc)] == 3 - who) continue;
            ni = idx(nr, nc);
            w = (board[ni] == who) ? 0 : 1;
            if (dist[cell] + w < dist[ni]) {
                dist[ni] = dist[cell] + w;
                if (!inq[ni]) {
                    inq[ni] = 1;
                    q[tail] = ni;
                    tail = (tail + 1) % (MAXC + 1);
                    count++;
                }
            }
        }
    }
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        int goal = (who == 1) ? (r == N - 1) : (c == N - 1);
        if (on(r, c) && goal && dist[idx(r, c)] < bestgoal) bestgoal = dist[idx(r, c)];
    }
    return bestgoal;
}

static int ai_move(void)
{
    int r, c, bestcell = -1, bestscore = -99999;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        int s;
        if (!on(r, c) || board[idx(r, c)]) continue;
        board[idx(r, c)] = 2;
        /* Its own progress matters, and so does blocking yours. */
        s = -path_cost(2) * 3 + path_cost(1);
        board[idx(r, c)] = 0;
        if (s > bestscore) { bestscore = s; bestcell = idx(r, c); }
    }
    return bestcell;
}

void fam_hexconnect(const GParams *p)
{
    int cr = 0, cc = 0, moves, i;
    const char *how;

    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR > 3) VAR = 0;
    N = gp_int(p->size, 11);
    if (N < 5) N = 5;
    if (N > MAXN) N = MAXN;

    for (;;) {
        int over = 0;
        for (i = 0; i < MAXC; i++) board[i] = 0;
        cr = N / 2; cc = N / 2;
        moves = 0;

        while (!over) {
            int k, r, c;
            char sub[150];
            snprintf(sub, sizeof sub,
                     "%s size %d — you are O joining top to bottom, arrows move, Enter places, Q quits",
                     VAR == V_HEX ? "Hex" : VAR == V_Y ? "Y" : VAR == V_HAVANNAH ? "Havannah" : "TwixT", N);
            draw_title("CONNECTION", sub);

            for (r = 0; r < N; r++) {
                int indent = (VAR == V_TWIXT) ? 0 : r;
                for (c = 0; c < N; c++) {
                    int v;
                    if (!on(r, c)) continue;
                    v = board[idx(r, c)];
                    draw_textf(4 + r, 6 + indent + c * 2,
                               "%s%s%c%s", (r == cr && c == cc) ? BG_BLUE : "",
                               v == 1 ? C_GREEN : v == 2 ? C_RED : C_GREY,
                               v == 1 ? 'O' : v == 2 ? 'X' : '.', C_RESET);
                }
            }
            draw_textf(5 + N, 6, "%smoves %d%s", C_GREY, moves, C_RESET);
            if (VAR == V_HAVANNAH)
                draw_textf(6 + N, 6, "%swin with a ring, a bridge between corners, or a fork on three edges%s",
                           C_GREY, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "hex", moves); return; }
            if (k == KEY_UP)    { if (cr > 0) cr--; if (!on(cr, cc)) cc = cr; continue; }
            if (k == KEY_DOWN)  { if (cr < N - 1) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (on(cr, cc + 1)) cc++; continue; }
            if (k != KEY_ENTER && k != ' ') continue;
            if (!on(cr, cc) || board[idx(cr, cc)]) continue;

            board[idx(cr, cc)] = 1;
            moves++;
            if (wins(1, &how)) {
                draw_textf(8 + N, 6, "%sYou win with %s.%s", C_GREEN, how, C_RESET);
                scr_flush();
                score_report(p->title ? p->title : "hex", 1000 - moves * 5);
                over = 1;
                break;
            }

            {
                int m = ai_move();
                if (m < 0) { over = 1; break; }
                board[m] = 2;
                moves++;
                if (wins(2, &how)) {
                    draw_textf(8 + N, 6, "%sThe opponent wins with %s.%s", C_RED, how, C_RESET);
                    scr_flush();
                    score_report(p->title ? p->title : "hex", moves);
                    over = 1;
                }
            }
        }
        sleep_ms(600);
        if (!confirm("\n  Another game?")) return;
    }
}
