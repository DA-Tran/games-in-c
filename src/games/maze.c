/* GIC:PARAMETERISED maze
 * maze.c - the maze family.
 *
 * params: size    = grid edge, odd (11, 21, 31, 41)
 *         variant = which generation algorithm
 *
 * The ten algorithms are the point. They are not speed settings: each leaves a
 * different texture behind. Recursive backtracking gives long winding
 * corridors and few junctions; Prim and Kruskal give short branchy passages;
 * binary tree and sidewinder leave a permanent bias along two edges; Wilson
 * and Aldous-Broder are uniform-random over all spanning trees and look it.
 * Solving them feels different, which is why each is its own catalogue entry.
 */
#include "engine.h"
#include "games.h"

#define MAXN 41
#define MAXC ((MAXN - 1) / 2)

static char wall[MAXN][MAXN];
static int N, CW;                 /* grid edge, and cells per side */
static int VAR;

static const char *ALGO[10] = {
    "Recursive Backtracker", "Prim", "Kruskal", "Eller", "Wilson",
    "Aldous-Broder", "Hunt and Kill", "Binary Tree", "Sidewinder", "Growing Tree"
};

/* Cell (cr,cc) sits at grid (2*cr+1, 2*cc+1). */
static int gr(int cr) { return cr * 2 + 1; }

static void open_between(int r1, int c1, int r2, int c2)
{
    wall[(gr(r1) + gr(r2)) / 2][(gr(c1) + gr(c2)) / 2] = 0;
    wall[gr(r1)][gr(c1)] = 0;
    wall[gr(r2)][gr(c2)] = 0;
}

static const int DR[4] = {-1, 1, 0, 0};
static const int DC[4] = {0, 0, -1, 1};

static int in_cells(int r, int c) { return r >= 0 && r < CW && c >= 0 && c < CW; }

/* ---------------------------------------------------------- union-find */
static int uf[MAXC * MAXC];
static int find(int a) { while (uf[a] != a) { uf[a] = uf[uf[a]]; a = uf[a]; } return a; }
static void unite(int a, int b) { uf[find(a)] = find(b); }

static void gen_backtracker(void)
{
    int stack[MAXC * MAXC][2], sp = 0;
    char seen[MAXC][MAXC];
    int r = rnd(CW), c = rnd(CW);
    memset(seen, 0, sizeof seen);
    seen[r][c] = 1;
    wall[gr(r)][gr(c)] = 0;
    stack[sp][0] = r; stack[sp][1] = c; sp++;
    while (sp > 0) {
        int d, order[4], n = 0, moved = 0;
        r = stack[sp-1][0]; c = stack[sp-1][1];
        for (d = 0; d < 4; d++) order[n++] = d;
        shuffle_int(order, 4);
        for (d = 0; d < 4; d++) {
            int nr = r + DR[order[d]], nc = c + DC[order[d]];
            if (!in_cells(nr, nc) || seen[nr][nc]) continue;
            seen[nr][nc] = 1;
            open_between(r, c, nr, nc);
            stack[sp][0] = nr; stack[sp][1] = nc; sp++;
            moved = 1;
            break;
        }
        if (!moved) sp--;
    }
}

static void gen_prim(void)
{
    char seen[MAXC][MAXC];
    int fr[MAXC * MAXC * 4][4], fn = 0;   /* frontier: to_r,to_c,from_r,from_c */
    int r = rnd(CW), c = rnd(CW), d;
    memset(seen, 0, sizeof seen);
    seen[r][c] = 1;
    wall[gr(r)][gr(c)] = 0;
    for (d = 0; d < 4; d++) {
        int nr = r + DR[d], nc = c + DC[d];
        if (!in_cells(nr, nc)) continue;
        fr[fn][0] = nr; fr[fn][1] = nc; fr[fn][2] = r; fr[fn][3] = c; fn++;
    }
    while (fn > 0) {
        int i = rnd(fn), nr = fr[i][0], nc = fr[i][1], pr = fr[i][2], pc = fr[i][3];
        fr[i][0] = fr[fn-1][0]; fr[i][1] = fr[fn-1][1];
        fr[i][2] = fr[fn-1][2]; fr[i][3] = fr[fn-1][3];
        fn--;
        if (seen[nr][nc]) continue;
        seen[nr][nc] = 1;
        open_between(pr, pc, nr, nc);
        for (d = 0; d < 4; d++) {
            int ar = nr + DR[d], ac = nc + DC[d];
            if (!in_cells(ar, ac) || seen[ar][ac]) continue;
            if (fn >= MAXC * MAXC * 4) continue;
            fr[fn][0] = ar; fr[fn][1] = ac; fr[fn][2] = nr; fr[fn][3] = nc; fn++;
        }
    }
}

static void gen_kruskal(void)
{
    int edges[MAXC * MAXC * 2][4], en = 0, i;
    int r, c;
    for (r = 0; r < CW; r++) for (c = 0; c < CW; c++) {
        if (c + 1 < CW) { edges[en][0]=r; edges[en][1]=c; edges[en][2]=r;   edges[en][3]=c+1; en++; }
        if (r + 1 < CW) { edges[en][0]=r; edges[en][1]=c; edges[en][2]=r+1; edges[en][3]=c;   en++; }
    }
    for (i = 0; i < CW * CW; i++) uf[i] = i;
    for (i = en - 1; i > 0; i--) {
        int j = rnd(i + 1), k;
        for (k = 0; k < 4; k++) { int t = edges[i][k]; edges[i][k] = edges[j][k]; edges[j][k] = t; }
    }
    for (i = 0; i < en; i++) {
        int a = edges[i][0] * CW + edges[i][1];
        int b = edges[i][2] * CW + edges[i][3];
        if (find(a) == find(b)) continue;
        unite(a, b);
        open_between(edges[i][0], edges[i][1], edges[i][2], edges[i][3]);
    }
}

/* Eller's: one row at a time, merging sets left to right then dropping down. */
static void gen_eller(void)
{
    int set[MAXC], next_set = 1, r, c;
    for (c = 0; c < CW; c++) set[c] = next_set++;
    for (r = 0; r < CW; r++) {
        int last = r == CW - 1;
        for (c = 0; c + 1 < CW; c++) {
            if (set[c] == set[c+1]) continue;
            if (!last && rnd(2)) continue;
            {
                int old = set[c+1], k;
                open_between(r, c, r, c+1);
                for (k = 0; k < CW; k++) if (set[k] == old) set[k] = set[c];
            }
        }
        if (last) break;
        {
            char dropped[MAXC];
            memset(dropped, 0, sizeof dropped);
            for (c = 0; c < CW; c++) {
                int run_end = c, k, chosen;
                while (run_end + 1 < CW && set[run_end+1] == set[c]) run_end++;
                chosen = c + rnd(run_end - c + 1);
                for (k = c; k <= run_end; k++)
                    if (k == chosen || rnd(3) == 0) { open_between(r, k, r+1, k); dropped[k] = 1; }
                c = run_end;
            }
            for (c = 0; c < CW; c++) if (!dropped[c]) set[c] = next_set++;
        }
    }
}

/* Wilson's: loop-erased random walks, uniform over spanning trees. */
static void gen_wilson(void)
{
    char in[MAXC][MAXC];
    int dir[MAXC][MAXC];
    int total = CW * CW, added = 1, guard = 0;
    int r = rnd(CW), c = rnd(CW);
    memset(in, 0, sizeof in);
    in[r][c] = 1;
    wall[gr(r)][gr(c)] = 0;
    while (added < total && guard++ < 400000) {
        int sr = rnd(CW), sc = rnd(CW), wr, wc;
        if (in[sr][sc]) continue;
        wr = sr; wc = sc;
        while (!in[wr][wc]) {
            int d = rnd(4), nr, nc;
            nr = wr + DR[d]; nc = wc + DC[d];
            if (!in_cells(nr, nc)) continue;
            dir[wr][wc] = d;
            wr = nr; wc = nc;
        }
        wr = sr; wc = sc;
        while (!in[wr][wc]) {
            int d = dir[wr][wc], nr = wr + DR[d], nc = wc + DC[d];
            in[wr][wc] = 1;
            added++;
            open_between(wr, wc, nr, nc);
            wr = nr; wc = nc;
        }
    }
}

static void gen_aldous(void)
{
    char seen[MAXC][MAXC];
    int r = rnd(CW), c = rnd(CW), added = 1, total = CW * CW, guard = 0;
    memset(seen, 0, sizeof seen);
    seen[r][c] = 1;
    wall[gr(r)][gr(c)] = 0;
    while (added < total && guard++ < 2000000) {
        int d = rnd(4), nr = r + DR[d], nc = c + DC[d];
        if (!in_cells(nr, nc)) continue;
        if (!seen[nr][nc]) { seen[nr][nc] = 1; added++; open_between(r, c, nr, nc); }
        r = nr; c = nc;
    }
}

static void gen_hunt_kill(void)
{
    char seen[MAXC][MAXC];
    int r = rnd(CW), c = rnd(CW);
    memset(seen, 0, sizeof seen);
    seen[r][c] = 1;
    wall[gr(r)][gr(c)] = 0;
    for (;;) {
        int order[4] = {0,1,2,3}, d, moved = 0;
        shuffle_int(order, 4);
        for (d = 0; d < 4; d++) {
            int nr = r + DR[order[d]], nc = c + DC[order[d]];
            if (!in_cells(nr, nc) || seen[nr][nc]) continue;
            seen[nr][nc] = 1;
            open_between(r, c, nr, nc);
            r = nr; c = nc;
            moved = 1;
            break;
        }
        if (moved) continue;
        /* Hunt: scan for an unvisited cell touching the carved region. */
        {
            int hr, hc, found = 0;
            for (hr = 0; hr < CW && !found; hr++) for (hc = 0; hc < CW; hc++) {
                if (seen[hr][hc]) continue;
                for (d = 0; d < 4; d++) {
                    int ar = hr + DR[d], ac = hc + DC[d];
                    if (!in_cells(ar, ac) || !seen[ar][ac]) continue;
                    seen[hr][hc] = 1;
                    open_between(hr, hc, ar, ac);
                    r = hr; c = hc;
                    found = 1;
                    break;
                }
                if (found) break;
            }
            if (!found) return;
        }
    }
}

/* Binary tree and sidewinder both leave a visible bias along two edges. */
static void gen_binary(void)
{
    int r, c;
    for (r = 0; r < CW; r++) for (c = 0; c < CW; c++) {
        int north = r > 0, east = c + 1 < CW;
        wall[gr(r)][gr(c)] = 0;
        if (north && east) { if (rnd(2)) open_between(r, c, r-1, c); else open_between(r, c, r, c+1); }
        else if (north) open_between(r, c, r-1, c);
        else if (east)  open_between(r, c, r, c+1);
    }
}

static void gen_sidewinder(void)
{
    int r, c;
    for (c = 0; c < CW; c++) { wall[gr(0)][gr(c)] = 0; if (c + 1 < CW) open_between(0, c, 0, c+1); }
    for (r = 1; r < CW; r++) {
        int run_start = 0;
        for (c = 0; c < CW; c++) {
            wall[gr(r)][gr(c)] = 0;
            if (c + 1 < CW && rnd(2)) { open_between(r, c, r, c+1); continue; }
            {
                /* close the run and carve north from one random member */
                int pick = run_start + rnd(c - run_start + 1);
                open_between(r, pick, r - 1, pick);
            }
            run_start = c + 1;
        }
    }
}

/* Growing tree: picking the newest cell gives backtracker, random gives
 * Prim; mixing the two gives its own character. */
static void gen_growing(void)
{
    int list[MAXC * MAXC][2], n = 0;
    char seen[MAXC][MAXC];
    int r = rnd(CW), c = rnd(CW);
    memset(seen, 0, sizeof seen);
    seen[r][c] = 1;
    wall[gr(r)][gr(c)] = 0;
    list[n][0] = r; list[n][1] = c; n++;
    while (n > 0) {
        int i = (rnd(100) < 60) ? n - 1 : rnd(n);
        int order[4] = {0,1,2,3}, d, moved = 0;
        r = list[i][0]; c = list[i][1];
        shuffle_int(order, 4);
        for (d = 0; d < 4; d++) {
            int nr = r + DR[order[d]], nc = c + DC[order[d]];
            if (!in_cells(nr, nc) || seen[nr][nc]) continue;
            seen[nr][nc] = 1;
            open_between(r, c, nr, nc);
            list[n][0] = nr; list[n][1] = nc; n++;
            moved = 1;
            break;
        }
        if (!moved) { list[i][0] = list[n-1][0]; list[i][1] = list[n-1][1]; n--; }
    }
}

static void generate(void)
{
    memset(wall, 1, sizeof wall);
    switch (VAR) {
        case 1: gen_prim();        break;
        case 2: gen_kruskal();     break;
        case 3: gen_eller();       break;
        case 4: gen_wilson();      break;
        case 5: gen_aldous();      break;
        case 6: gen_hunt_kill();   break;
        case 7: gen_binary();      break;
        case 8: gen_sidewinder();  break;
        case 9: gen_growing();     break;
        default: gen_backtracker();break;
    }
    wall[1][1] = 0;
    wall[N-2][N-2] = 0;
}

/* Breadth-first shortest path, used for the hint and the par count. */
static int solve(int sr, int sc, char path[MAXN][MAXN])
{
    static int q[MAXN * MAXN][2];
    static int dist[MAXN][MAXN];
    int head = 0, tail = 0, r, c, d;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) dist[r][c] = -1;
    dist[sr][sc] = 0;
    q[tail][0] = sr; q[tail][1] = sc; tail++;
    while (head < tail) {
        r = q[head][0]; c = q[head][1]; head++;
        for (d = 0; d < 4; d++) {
            int nr = r + DR[d], nc = c + DC[d];
            if (nr < 0 || nr >= N || nc < 0 || nc >= N) continue;
            if (wall[nr][nc] || dist[nr][nc] >= 0) continue;
            dist[nr][nc] = dist[r][c] + 1;
            q[tail][0] = nr; q[tail][1] = nc; tail++;
        }
    }
    if (dist[N-2][N-2] < 0) return -1;
    if (path) {
        r = N - 2; c = N - 2;
        memset(path, 0, sizeof(char) * MAXN * MAXN);
        while (!(r == sr && c == sc)) {
            path[r][c] = 1;
            for (d = 0; d < 4; d++) {
                int nr = r + DR[d], nc = c + DC[d];
                if (nr < 0 || nr >= N || nc < 0 || nc >= N) continue;
                if (dist[nr][nc] == dist[r][c] - 1) { r = nr; c = nc; break; }
            }
            if (d == 4) break;
        }
        path[sr][sc] = 1;
    }
    return dist[N-2][N-2];
}

void fam_maze(const GParams *p)
{
    int par;
    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR > 9) VAR = 0;
    N = gp_int(p->size, 21);
    if (N < 7) N = 7;
    if (N > MAXN) N = MAXN;
    if (!(N & 1)) N++;
    CW = (N - 1) / 2;

    for (;;) {
        static char path[MAXN][MAXN];
        int pr = 1, pc = 1, steps = 0, hint = 0, won = 0;
        char sub[110];

        generate();
        par = solve(1, 1, path);
        snprintf(sub, sizeof sub, "%s, %dx%d — arrows move, H hints, Q quits",
                 ALGO[VAR], N, N);

        for (;;) {
            /* Big mazes scroll; the viewport follows the player. */
            int vh = 18, vw = 66;
            int top = pr - vh / 2, left = pc - vw / 2, r, c, k;
            if (top < 0) top = 0;
            if (left < 0) left = 0;
            if (top > N - vh) top = N - vh;
            if (left > N - vw) left = N - vw;
            if (top < 0) top = 0;
            if (left < 0) left = 0;

            draw_title("MAZE", sub);
            for (r = 0; r < vh && top + r < N; r++) {
                scr_move(4 + r, 8);
                for (c = 0; c < vw && left + c < N; c++) {
                    int gr2 = top + r, gc2 = left + c;
                    if (gr2 == pr && gc2 == pc)            printf("%s@%s", C_BOLD C_CYAN, C_RESET);
                    else if (gr2 == N-2 && gc2 == N-2)     printf("%s$%s", C_BOLD C_YELLOW, C_RESET);
                    else if (wall[gr2][gc2])               printf("%s#%s", C_BLUE, C_RESET);
                    else if (hint && path[gr2][gc2])       printf("%s.%s", C_GREEN, C_RESET);
                    else                                   printf(" ");
                }
            }
            draw_textf(4 + vh + 1, 8, "Steps %-6d  Shortest %-6d  %s   ",
                       steps, par, hint ? "hint on " : "        ");
            if (won) draw_textf(4 + vh + 2, 8, "%sOut in %d steps (shortest %d)!%s  ",
                                C_BOLD C_GREEN, steps, par, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (won) break;
            if (k == 'h' || k == 'H') { hint = !hint; continue; }
            {
                int d = -1, nr, nc;
                if (k == KEY_UP) d = 0;
                if (k == KEY_DOWN) d = 1;
                if (k == KEY_LEFT) d = 2;
                if (k == KEY_RIGHT) d = 3;
                if (d < 0) continue;
                nr = pr + DR[d]; nc = pc + DC[d];
                if (nr < 0 || nr >= N || nc < 0 || nc >= N || wall[nr][nc]) continue;
                pr = nr; pc = nc;
                steps++;
                if (pr == N-2 && pc == N-2) {
                    won = 1;
                    score_report(p->title ? p->title : "maze", par * 1000 / (steps + 1));
                }
            }
        }
        if (!confirm("\n  Another maze?")) return;
    }
}
