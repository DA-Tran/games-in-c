/* tetris.c - seven-bag randomiser, hard drop, and progressive gravity. */
#include "engine.h"
#include "games.h"

#define W 10
#define H 20

static int board[H][W];
static int piece, rot, px, py, score, lines, level;
static int bag[7], bag_pos = 7;

/* Each piece as four rotations of four (x,y) cells. */
static const int SHAPES[7][4][4][2] = {
 {{{0,1},{1,1},{2,1},{3,1}},{{2,0},{2,1},{2,2},{2,3}},{{0,2},{1,2},{2,2},{3,2}},{{1,0},{1,1},{1,2},{1,3}}}, /* I */
 {{{0,0},{0,1},{1,1},{2,1}},{{1,0},{2,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{0,2},{1,2}}}, /* J */
 {{{2,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,1},{0,2}},{{0,0},{1,0},{1,1},{1,2}}}, /* L */
 {{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}}}, /* O */
 {{{1,0},{2,0},{0,1},{1,1}},{{1,0},{1,1},{2,1},{2,2}},{{1,1},{2,1},{0,2},{1,2}},{{0,0},{0,1},{1,1},{1,2}}}, /* S */
 {{{1,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{2,1},{1,2}},{{1,0},{0,1},{1,1},{1,2}}}, /* T */
 {{{0,0},{1,0},{1,1},{2,1}},{{2,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{1,2},{2,2}},{{1,0},{0,1},{1,1},{0,2}}}  /* Z */
};
static const char *PCOL[7] = {C_CYAN, C_BLUE, C_YELLOW, C_YELLOW, C_GREEN, C_MAGENTA, C_RED};

static int next_piece(void)
{
    if (bag_pos >= 7) {
        int i;
        for (i = 0; i < 7; i++) bag[i] = i;
        shuffle_int(bag, 7);
        bag_pos = 0;
    }
    return bag[bag_pos++];
}

static int collides(int p, int r, int x, int y)
{
    int i;
    for (i = 0; i < 4; i++) {
        int cx = x + SHAPES[p][r][i][0];
        int cy = y + SHAPES[p][r][i][1];
        if (cx < 0 || cx >= W || cy >= H) return 1;
        if (cy >= 0 && board[cy][cx]) return 1;
    }
    return 0;
}

static void lock_piece(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        int cx = px + SHAPES[piece][rot][i][0];
        int cy = py + SHAPES[piece][rot][i][1];
        if (cy >= 0 && cy < H && cx >= 0 && cx < W) board[cy][cx] = piece + 1;
    }
}

static void clear_lines(void)
{
    int r, c, cleared = 0;
    static const int PTS[5] = {0, 100, 300, 500, 800};
    for (r = H - 1; r >= 0; r--) {
        int full = 1;
        for (c = 0; c < W; c++) if (!board[r][c]) { full = 0; break; }
        if (!full) continue;
        for (c = r; c > 0; c--) memcpy(board[c], board[c-1], sizeof board[0]);
        memset(board[0], 0, sizeof board[0]);
        cleared++;
        r++;                     /* re-test the row that dropped in */
    }
    if (cleared) {
        lines += cleared;
        score += PTS[cleared > 4 ? 4 : cleared] * (level + 1);
        level = lines / 10;
    }
}

static void spawn(int *dead)
{
    piece = next_piece();
    rot = 0;
    px = W / 2 - 2;
    py = -1;
    if (collides(piece, rot, px, py)) *dead = 1;
}

static void render(int nextp)
{
    int r, c, i;
    draw_title("TETRIS", "Arrows move/rotate, Space hard-drops, Q quits");
    draw_box(5, 24, H + 2, W * 2 + 2, C_BLUE);
    for (r = 0; r < H; r++) {
        scr_move(6 + r, 25);
        for (c = 0; c < W; c++) {
            if (board[r][c]) printf("%s██%s", PCOL[board[r][c] - 1], C_RESET);
            else             printf("%s ·%s", C_GREY, C_RESET);
        }
    }
    for (i = 0; i < 4; i++) {
        int cx = px + SHAPES[piece][rot][i][0];
        int cy = py + SHAPES[piece][rot][i][1];
        if (cy >= 0 && cy < H)
            draw_textf(6 + cy, 25 + cx * 2, "%s██%s", PCOL[piece], C_RESET);
    }
    draw_textf(7,  50, "Score %-8d", score);
    draw_textf(8,  50, "Lines %-8d", lines);
    draw_textf(9,  50, "Level %-8d", level);
    draw_textf(11, 50, "Next:");
    for (r = 0; r < 4; r++) {
        scr_move(12 + r, 50);
        printf("        ");
    }
    for (i = 0; i < 4; i++)
        draw_textf(12 + SHAPES[nextp][0][i][1], 50 + SHAPES[nextp][0][i][0] * 2,
                   "%s██%s", PCOL[nextp], C_RESET);
    scr_flush();
}

void fam_tetris(const GParams *p)
{
    (void)p;
    for (;;) {
        int dead = 0, nextp;
        long last;
        memset(board, 0, sizeof board);
        score = lines = level = 0;
        bag_pos = 7;
        spawn(&dead);
        nextp = bag[bag_pos % 7];
        last = now_ms();

        while (!dead) {
            int k = key_poll();
            int speed = 500 - level * 40;
            if (speed < 80) speed = 80;

            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT  && !collides(piece, rot, px - 1, py)) px--;
            if (k == KEY_RIGHT && !collides(piece, rot, px + 1, py)) px++;
            if (k == KEY_DOWN  && !collides(piece, rot, px, py + 1)) py++;
            if (k == KEY_UP) {
                int nr = (rot + 1) % 4;
                if      (!collides(piece, nr, px, py))     rot = nr;
                else if (!collides(piece, nr, px - 1, py)) { rot = nr; px--; }   /* wall kick */
                else if (!collides(piece, nr, px + 1, py)) { rot = nr; px++; }
            }
            if (k == ' ') {
                while (!collides(piece, rot, px, py + 1)) { py++; score += 2; }
                lock_piece();
                clear_lines();
                piece = nextp;
                rot = 0; px = W / 2 - 2; py = -1;
                if (collides(piece, rot, px, py)) dead = 1;
                nextp = next_piece();
                last = now_ms();
                render(nextp);
                continue;
            }

            if (now_ms() - last >= speed) {
                last = now_ms();
                if (!collides(piece, rot, px, py + 1)) py++;
                else {
                    lock_piece();
                    clear_lines();
                    piece = nextp;
                    rot = 0; px = W / 2 - 2; py = -1;
                    if (collides(piece, rot, px, py)) dead = 1;
                    nextp = next_piece();
                }
            }
            render(nextp);
            sleep_ms(12);
        }
        draw_centered(H + 9, 80, C_BOLD C_RED "Game over" C_RESET);
        score_report("tetris", score);
        if (!confirm("\n  Play again?")) return;
    }
}
