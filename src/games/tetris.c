/* GIC:PARAMETERISED tetris
 * tetris.c - the falling-block family.
 *
 * params: variant = 0 classic, 1 sprint 40, 2 ultra (2 min), 3 zen,
 *                   4 big mode, 5 master, 6 invisible, 7 cascade, 8 pentix
 *
 * Rotations are computed at run time from a base cell list rather than stored
 * as a table, because Pentix uses the twelve pentominoes and hard-coding four
 * rotations for each would be 240 literals that could drift out of step. The
 * other variants change the goal (clear 40, score in two minutes, never lose),
 * the gravity curve, the visibility of locked blocks, or what happens after a
 * clear — cascade re-drops every floating cell.
 */
#include "engine.h"
#include "games.h"

#define MAXW 12
#define MAXH 22
#define MAXCELL 5
#define MAXPIECE 12

typedef struct { int n; int c[MAXCELL][2]; } Shape;

/* Base orientations. Tetrominoes first, then the twelve pentominoes. */
static const Shape TETRO[7] = {
    {4, {{0,1},{1,1},{2,1},{3,1}}},                    /* I */
    {4, {{0,0},{0,1},{1,1},{2,1}}},                    /* J */
    {4, {{2,0},{0,1},{1,1},{2,1}}},                    /* L */
    {4, {{1,0},{2,0},{1,1},{2,1}}},                    /* O */
    {4, {{1,0},{2,0},{0,1},{1,1}}},                    /* S */
    {4, {{1,0},{0,1},{1,1},{2,1}}},                    /* T */
    {4, {{0,0},{1,0},{1,1},{2,1}}}                     /* Z */
};
static const Shape PENTO[12] = {
    {5, {{0,0},{0,1},{0,2},{0,3},{0,4}}},              /* I */
    {5, {{0,0},{0,1},{0,2},{0,3},{1,3}}},              /* L */
    {5, {{1,0},{1,1},{1,2},{0,3},{1,3}}},              /* J */
    {5, {{0,0},{0,1},{1,1},{1,2},{1,3}}},              /* N */
    {5, {{0,0},{1,0},{0,1},{1,1},{0,2}}},              /* P */
    {5, {{0,0},{0,1},{1,1},{0,2},{1,2}}},              /* Q */
    {5, {{1,0},{0,1},{1,1},{0,2},{1,2}}},              /* R */
    {5, {{0,0},{1,0},{1,1},{1,2},{2,2}}},              /* S/Z */
    {5, {{0,0},{1,0},{2,0},{1,1},{1,2}}},              /* T */
    {5, {{0,0},{2,0},{0,1},{1,1},{2,1}}},              /* U */
    {5, {{0,0},{0,1},{0,2},{1,2},{2,2}}},              /* V */
    {5, {{1,0},{0,1},{1,1},{2,1},{1,2}}}               /* X / plus */
};

static Shape rots[MAXPIECE][4];
static int NPIECE, NCELL;
static int W, H, VAR, cellw;
static int board[MAXH][MAXW];
static long lock_at[MAXH][MAXW];          /* when a cell landed (invisible mode) */
static int piece, rot, px, py, score, lines, level;
static int bag[MAXPIECE], bag_pos;

static const char *PCOL[12] = {
    C_CYAN, C_BLUE, C_YELLOW, C_YELLOW, C_GREEN, C_MAGENTA, C_RED,
    C_CYAN, C_GREEN, C_BLUE, C_MAGENTA, C_WHITE
};

/* One quarter turn about the shape's bounding box, then renormalised. */
static Shape rotate_once(Shape s)
{
    Shape o;
    int i, maxy = 0, minx = 99, miny = 99;
    for (i = 0; i < s.n; i++) if (s.c[i][1] > maxy) maxy = s.c[i][1];
    o.n = s.n;
    for (i = 0; i < s.n; i++) {
        o.c[i][0] = maxy - s.c[i][1];
        o.c[i][1] = s.c[i][0];
    }
    for (i = 0; i < o.n; i++) {
        if (o.c[i][0] < minx) minx = o.c[i][0];
        if (o.c[i][1] < miny) miny = o.c[i][1];
    }
    for (i = 0; i < o.n; i++) { o.c[i][0] -= minx; o.c[i][1] -= miny; }
    return o;
}

static void build_rotations(void)
{
    const Shape *base = (VAR == 8) ? PENTO : TETRO;
    int i, r;
    NPIECE = (VAR == 8) ? 12 : 7;
    NCELL  = (VAR == 8) ? 5 : 4;
    for (i = 0; i < NPIECE; i++) {
        rots[i][0] = base[i];
        for (r = 1; r < 4; r++) rots[i][r] = rotate_once(rots[i][r - 1]);
    }
}

static int next_piece(void)
{
    if (bag_pos >= NPIECE) {
        int i;
        for (i = 0; i < NPIECE; i++) bag[i] = i;
        shuffle_int(bag, NPIECE);
        bag_pos = 0;
    }
    return bag[bag_pos++];
}

static int collides(int p, int r, int x, int y)
{
    const Shape *s = &rots[p][r];
    int i;
    for (i = 0; i < s->n; i++) {
        int cx = x + s->c[i][0];
        int cy = y + s->c[i][1];
        if (cx < 0 || cx >= W || cy >= H) return 1;
        if (cy >= 0 && board[cy][cx]) return 1;
    }
    return 0;
}

static void lock_piece(void)
{
    const Shape *s = &rots[piece][rot];
    int i;
    for (i = 0; i < s->n; i++) {
        int cx = px + s->c[i][0];
        int cy = py + s->c[i][1];
        if (cy >= 0 && cy < H && cx >= 0 && cx < W) {
            board[cy][cx] = piece + 1;
            lock_at[cy][cx] = now_ms();
        }
    }
}

/* Cascade: after a clear, every unsupported cell falls on its own. */
static void cascade(void)
{
    int moved = 1, r, c, guard = 0;
    while (moved && guard++ < 40) {
        moved = 0;
        for (r = H - 2; r >= 0; r--)
            for (c = 0; c < W; c++)
                if (board[r][c] && !board[r + 1][c]) {
                    board[r + 1][c] = board[r][c];
                    lock_at[r + 1][c] = lock_at[r][c];
                    board[r][c] = 0;
                    moved = 1;
                }
    }
}

static void clear_lines(void)
{
    int r, c, cleared = 0;
    static const int PTS[6] = {0, 100, 300, 500, 800, 1200};
    for (r = H - 1; r >= 0; r--) {
        int full = 1;
        for (c = 0; c < W; c++) if (!board[r][c]) { full = 0; break; }
        if (!full) continue;
        for (c = r; c > 0; c--) {
            memcpy(board[c], board[c-1], sizeof board[0]);
            memcpy(lock_at[c], lock_at[c-1], sizeof lock_at[0]);
        }
        memset(board[0], 0, sizeof board[0]);
        cleared++;
        r++;
    }
    if (cleared) {
        lines += cleared;
        score += PTS[cleared > 5 ? 5 : cleared] * (level + 1);
        if (VAR != 3) level = lines / 10;       /* zen never speeds up */
        if (VAR == 7) { cascade(); }
    }
}

static void render(int nextp, int secs_left, int flash)
{
    int r, c, i, bx = 40 - (W * cellw) / 2;
    long t = now_ms();
    char sub[120];
    static const char *NAME[9] = {
        "Arrows move/rotate, Space hard-drops",
        "SPRINT: clear 40 lines as fast as you can",
        "ULTRA: score as much as possible in two minutes",
        "ZEN: no game over, no speed-up",
        "BIG MODE: wide board, chunky blocks",
        "MASTER: fast from the first piece",
        "INVISIBLE: locked blocks fade — press F to flash them",
        "CASCADE: loose blocks fall again after a clear",
        "PENTIX: twelve pentominoes instead of tetrominoes"
    };
    snprintf(sub, sizeof sub, "%s, Q quits", NAME[VAR]);
    draw_title("TETRIS", sub);
    draw_box(4, bx - 1, H + 2, W * cellw + 2, C_BLUE);
    for (r = 0; r < H; r++) {
        scr_move(5 + r, bx);
        for (c = 0; c < W; c++) {
            int v = board[r][c];
            /* Invisible mode hides a block a second after it lands. */
            int hidden = (VAR == 6 && v && !flash && t - lock_at[r][c] > 1000);
            if (v && !hidden) printf("%s%.*s%s", PCOL[v - 1], cellw * 2, "######", C_RESET);
            else              printf("%s%.*s%s", C_GREY, cellw * 2, " .    ", C_RESET);
        }
    }
    {
        const Shape *s = &rots[piece][rot];
        for (i = 0; i < s->n; i++) {
            int cx = px + s->c[i][0], cy = py + s->c[i][1];
            if (cy >= 0 && cy < H)
                draw_textf(5 + cy, bx + cx * cellw, "%s%.*s%s", PCOL[piece], cellw * 2, "######", C_RESET);
        }
    }
    draw_textf(6,  bx + W * cellw + 4, "Score %-8d", score);
    draw_textf(7,  bx + W * cellw + 4, "Lines %-8d", lines);
    draw_textf(8,  bx + W * cellw + 4, "Level %-8d", level);
    if (VAR == 1) draw_textf(9, bx + W * cellw + 4, "Left  %-8d", 40 - lines > 0 ? 40 - lines : 0);
    if (VAR == 2) draw_textf(9, bx + W * cellw + 4, "Time  %-8d", secs_left);
    draw_textf(11, bx + W * cellw + 4, "Next:");
    for (r = 0; r < 5; r++) draw_text(12 + r, bx + W * cellw + 4, "          ");
    {
        const Shape *s = &rots[nextp][0];
        for (i = 0; i < s->n; i++)
            draw_textf(12 + s->c[i][1], bx + W * cellw + 4 + s->c[i][0] * 2,
                       "%s##%s", PCOL[nextp], C_RESET);
    }
    scr_flush();
}

void fam_tetris(const GParams *p)
{
    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR > 8) VAR = 0;
    W = (VAR == 4) ? 8 : 10;
    H = (VAR == 4) ? 16 : 20;
    cellw = (VAR == 4) ? 3 : 2;
    build_rotations();

    for (;;) {
        int dead = 0, nextp, quit = 0, flash = 0, secs_left = 120;
        long last, started;
        memset(board, 0, sizeof board);
        memset(lock_at, 0, sizeof lock_at);
        score = lines = 0;
        level = (VAR == 5) ? 9 : 0;             /* master starts fast */
        bag_pos = NPIECE;
        piece = next_piece(); rot = 0; px = W / 2 - 2; py = -1;
        nextp = next_piece();
        last = started = now_ms();

        while (!dead) {
            int k = key_poll();
            int speed = 500 - level * 40;
            if (VAR == 3) speed = 450;
            if (speed < 60) speed = 60;

            if (k == 'q' || k == 'Q' || k == KEY_ESC) { quit = 1; break; }
            if (k == 'f' || k == 'F') flash = !flash;
            if (k == KEY_LEFT  && !collides(piece, rot, px - 1, py)) px--;
            if (k == KEY_RIGHT && !collides(piece, rot, px + 1, py)) px++;
            if (k == KEY_DOWN  && !collides(piece, rot, px, py + 1)) py++;
            if (k == KEY_UP) {
                int nr = (rot + 1) % 4;
                if      (!collides(piece, nr, px, py))     rot = nr;
                else if (!collides(piece, nr, px - 1, py)) { rot = nr; px--; }
                else if (!collides(piece, nr, px + 1, py)) { rot = nr; px++; }
                else if (!collides(piece, nr, px + 2, py)) { rot = nr; px += 2; }
            }
            if (k == ' ') {
                while (!collides(piece, rot, px, py + 1)) { py++; score += 2; }
                lock_piece();
                clear_lines();
                piece = nextp; rot = 0; px = W / 2 - 2; py = -1;
                if (collides(piece, rot, px, py)) dead = 1;
                nextp = next_piece();
                last = now_ms();
            }

            if (now_ms() - last >= speed) {
                last = now_ms();
                if (!collides(piece, rot, px, py + 1)) py++;
                else {
                    lock_piece();
                    clear_lines();
                    piece = nextp; rot = 0; px = W / 2 - 2; py = -1;
                    if (collides(piece, rot, px, py)) dead = 1;
                    nextp = next_piece();
                }
            }

            /* Zen clears the stack instead of ending the run. */
            if (dead && VAR == 3) {
                memset(board, 0, sizeof board);
                memset(lock_at, 0, sizeof lock_at);
                dead = 0;
            }
            if (VAR == 1 && lines >= 40) break;
            if (VAR == 2) {
                secs_left = 120 - (int)((now_ms() - started) / 1000);
                if (secs_left <= 0) break;
            }
            render(nextp, secs_left, flash);
            sleep_ms(12);
        }
        if (quit) return;
        if (VAR == 1 && lines >= 40) {
            char m[80];
            snprintf(m, sizeof m, "40 lines in %ld seconds!", (now_ms() - started) / 1000);
            draw_centered(H + 8, 80, m);
            score_report(p->title ? p->title : "tetris", (int)(100000 / ((now_ms() - started) / 1000 + 1)));
        } else {
            draw_centered(H + 8, 80, C_BOLD C_RED "Game over" C_RESET);
            score_report(p->title ? p->title : "tetris", score);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
