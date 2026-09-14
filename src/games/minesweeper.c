/* GIC:PARAMETERISED minesweeper
 * minesweeper.c - mine sweeping across board sizes and adjacency rules.
 *
 * params: width, height = board size (up to 40x20)
 *         count         = number of mines
 *         variant       = 0 classic  1 no-guess  2 wrap  3 knight adjacency
 *
 * "No guess" boards are re-rolled until a logic-only solver can clear them
 * from the opening click, so the player never has to gamble.
 */
#include "engine.h"
#include "games.h"

#define MAXW 40
#define MAXH 20

static int W, H, MINES, VARIANT;
static int mine[MAXH][MAXW], shown[MAXH][MAXW], flag[MAXH][MAXW];

/* Neighbour offsets depend on the adjacency rule in play. */
static int neigh_of(int r, int c, int idx, int *nr, int *nc)
{
    static const int KR[8] = {-2,-2,-1,-1,1,1,2,2};
    static const int KC[8] = {-1,1,-2,2,-2,2,-1,1};
    int dr, dc;
    if (VARIANT == 3) {
        if (idx >= 8) return 0;
        *nr = r + KR[idx]; *nc = c + KC[idx];
    } else {
        if (idx >= 9) return 0;
        dr = idx / 3 - 1; dc = idx % 3 - 1;
        if (!dr && !dc) return -1;            /* skip the cell itself */
        *nr = r + dr; *nc = c + dc;
    }
    if (VARIANT == 2) {                        /* wrap */
        *nr = (*nr + H) % H;
        *nc = (*nc + W) % W;
        return 1;
    }
    if (*nr < 0 || *nr >= H || *nc < 0 || *nc >= W) return -1;
    return 1;
}

static int neighbours(int r, int c)
{
    int i, n = 0, a, b;
    for (i = 0; ; i++) {
        int s = neigh_of(r, c, i, &a, &b);
        if (s == 0) break;
        if (s < 0) continue;
        if (mine[a][b]) n++;
    }
    return n;
}

static int hidden_neighbours(int r, int c, int *flags)
{
    int i, n = 0, a, b;
    *flags = 0;
    for (i = 0; ; i++) {
        int s = neigh_of(r, c, i, &a, &b);
        if (s == 0) break;
        if (s < 0) continue;
        if (flag[a][b]) (*flags)++;
        else if (!shown[a][b]) n++;
    }
    return n;
}

static void lay_mines(int sr, int sc)
{
    int placed = 0;
    memset(mine, 0, sizeof mine);
    while (placed < MINES) {
        int r = rnd(H), c = rnd(W);
        if (mine[r][c]) continue;
        if (r >= sr - 1 && r <= sr + 1 && c >= sc - 1 && c <= sc + 1) continue;
        mine[r][c] = 1;
        placed++;
    }
}

static void reveal(int r, int c)
{
    int i, a, b;
    if (r < 0 || r >= H || c < 0 || c >= W) return;
    if (shown[r][c] || flag[r][c]) return;
    shown[r][c] = 1;
    if (mine[r][c] || neighbours(r, c) != 0) return;
    for (i = 0; ; i++) {
        int s = neigh_of(r, c, i, &a, &b);
        if (s == 0) break;
        if (s < 0) continue;
        reveal(a, b);
    }
}

/* Single-point constraint solver: repeatedly apply "all remaining hidden
 * neighbours are mines" and "all remaining hidden neighbours are safe". */
static int logic_solvable(int sr, int sc)
{
    int r, c, progress = 1, i, a, b;
    memset(shown, 0, sizeof shown);
    memset(flag, 0, sizeof flag);
    reveal(sr, sc);

    while (progress) {
        progress = 0;
        for (r = 0; r < H; r++) for (c = 0; c < W; c++) {
            int flags, hid, n;
            if (!shown[r][c] || mine[r][c]) continue;
            n = neighbours(r, c);
            hid = hidden_neighbours(r, c, &flags);
            if (!hid) continue;
            if (n - flags == hid) {                 /* every hidden one is a mine */
                for (i = 0; ; i++) {
                    int s = neigh_of(r, c, i, &a, &b);
                    if (s == 0) break;
                    if (s < 0) continue;
                    if (!shown[a][b] && !flag[a][b]) { flag[a][b] = 1; progress = 1; }
                }
            } else if (n == flags) {                /* every hidden one is safe */
                for (i = 0; ; i++) {
                    int s = neigh_of(r, c, i, &a, &b);
                    if (s == 0) break;
                    if (s < 0) continue;
                    if (!shown[a][b] && !flag[a][b]) { reveal(a, b); progress = 1; }
                }
            }
        }
    }
    for (r = 0; r < H; r++) for (c = 0; c < W; c++)
        if (!mine[r][c] && !shown[r][c]) return 0;
    return 1;
}

static int cleared(void)
{
    int r, c, n = 0;
    for (r = 0; r < H; r++) for (c = 0; c < W; c++) if (shown[r][c]) n++;
    return n == W * H - MINES;
}

static void render(int cr, int cc, int dead, const char *msg)
{
    static const char *NCOL[9] = {C_GREY, C_BLUE, C_GREEN, C_RED, C_MAGENTA,
                                  C_YELLOW, C_CYAN, C_WHITE, C_WHITE};
    static const char *VNAME[4] = {"Classic", "No Guess", "Wrap", "Knight"};
    int r, c, flags = 0, left;
    char sub[120];
    for (r = 0; r < H; r++) for (c = 0; c < W; c++) flags += flag[r][c];
    left = 40 - W;
    if (left < 2) left = 2;

    snprintf(sub, sizeof sub, "%dx%d %s, %d mines - arrows move, Enter reveals, F flags, Q quits",
             W, H, VNAME[VARIANT], MINES);
    draw_title("MINESWEEPER", sub);

    for (r = 0; r < H; r++) {
        scr_move(6 + r, left);
        for (c = 0; c < W; c++) {
            int sel = (r == cr && c == cc);
            const char *bg = sel ? BG_BLUE : "";
            if (flag[r][c] && !shown[r][c])  printf("%s%s F %s", bg, C_RED, C_RESET);
            else if (!shown[r][c] && !dead)  printf("%s%s . %s", bg, C_GREY, C_RESET);
            else if (mine[r][c])             printf("%s%s * %s", bg, C_RED, C_RESET);
            else if (!shown[r][c])           printf("%s%s . %s", bg, C_GREY, C_RESET);
            else {
                int n = neighbours(r, c);
                if (n == 0) printf("%s   %s", bg, C_RESET);
                else        printf("%s%s%2d %s", bg, NCOL[n > 8 ? 8 : n], n, C_RESET);
            }
        }
    }
    draw_textf(H + 7, left, "Mines %d   Flags %d      ", MINES, flags);
    draw_text(H + 9, 16, "                                                        ");
    if (msg) draw_textf(H + 9, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_minesweeper(const GParams *p)
{
    W = gp_int(p->width, 16);  if (W > MAXW) W = MAXW;
    H = gp_int(p->height, 16); if (H > MAXH) H = MAXH;
    MINES = gp_int(p->count, 40);
    if (MINES > W * H - 10) MINES = W * H - 10;
    VARIANT = (p->variant >= 0 && p->variant <= 3) ? p->variant : 0;

    for (;;) {
        int cr = H / 2, cc = W / 2, first = 1, dead = 0, won = 0;
        long start;
        memset(mine, 0, sizeof mine);
        memset(shown, 0, sizeof shown);
        memset(flag, 0, sizeof flag);
        start = now_ms();

        while (!dead && !won) {
            int k;
            render(cr, cc, 0, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < H - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < W - 1) cc++;
            if (k == 'f' || k == 'F') {
                if (!shown[cr][cc]) flag[cr][cc] = !flag[cr][cc];
                continue;
            }
            if (k != KEY_ENTER && k != ' ') continue;
            if (flag[cr][cc]) continue;

            if (first) {
                int tries = 0;
                do {
                    lay_mines(cr, cc);
                    tries++;
                } while (VARIANT == 1 && tries < 400 && !logic_solvable(cr, cc));
                memset(shown, 0, sizeof shown);
                memset(flag, 0, sizeof flag);
                first = 0;
            }
            if (mine[cr][cc]) dead = 1;
            else { reveal(cr, cc); won = cleared(); }
        }
        render(cr, cc, dead, dead ? "BOOM - you hit a mine." : "Field cleared!");
        if (won)
            score_report(p->title ? p->title : "minesweeper",
                         (int)(MINES * 200 / (1 + (now_ms() - start) / 1000)));
        if (!confirm("\n  Play again?")) return;
    }
}
