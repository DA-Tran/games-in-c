/* pacman.c - maze pursuit; each ghost runs a different chase rule. */
#include "engine.h"
#include "games.h"

#define W 28
#define H 17
#define GHOSTS 4

static const char *MAZE[H] = {
"############################",
"#............##............#",
"#.####.#####.##.#####.####.#",
"#.####.#####.##.#####.####.#",
"#..........................#",
"#.####.##.########.##.####.#",
"#......##....##....##......#",
"######.##### ## #####.######",
"     #.##          ##.#     ",
"######.## ######## ##.######",
"#............##............#",
"#.####.#####.##.#####.####.#",
"#...##................##...#",
"###.##.##.########.##.##.###",
"#......##....##....##......#",
"#.##########.##.##########.#",
"############################"
};

static char grid[H][W + 1];
static int pr, pc, dr_, dc_;
static int gr[GHOSTS], gc[GHOSTS], gdr[GHOSTS], gdc[GHOSTS];
static int score, lives, pellets;

static int wall(int r, int c)
{
    if (r < 0 || r >= H || c < 0 || c >= W) return 1;
    return grid[r][c] == '#';
}

static void reset_positions(void)
{
    int i;
    pr = 12; pc = 13; dr_ = 0; dc_ = 0;
    for (i = 0; i < GHOSTS; i++) {
        gr[i] = 8; gc[i] = 11 + i;
        gdr[i] = 0; gdc[i] = (i % 2) ? 1 : -1;
    }
}

static void load_maze(void)
{
    int r, c;
    pellets = 0;
    for (r = 0; r < H; r++) {
        strcpy(grid[r], MAZE[r]);
        for (c = 0; c < W; c++) if (grid[r][c] == '.') pellets++;
    }
}

/* Ghost i moves one step, preferring the direction that closes on Pacman. */
static void move_ghost(int i)
{
    static const int DR[4] = {-1,1,0,0}, DC[4] = {0,0,-1,1};
    int best = -1, bestd = 1 << 30, d;
    int tr = pr, tc = pc;

    if (i == 1) { tr = pr + dr_ * 4; tc = pc + dc_ * 4; }   /* ambush ahead */
    if (i == 2) { tr = H - pr; tc = W - pc; }               /* patrol opposite */
    if (i == 3 && rnd(4) == 0) {                            /* occasionally random */
        for (d = 0; d < 8; d++) {
            int t = rnd(4);
            if (!wall(gr[i] + DR[t], gc[i] + DC[t])) { best = t; break; }
        }
    }
    if (best < 0) {
        for (d = 0; d < 4; d++) {
            int nr = gr[i] + DR[d], nc = gc[i] + DC[d], dist;
            if (wall(nr, nc)) continue;
            if (DR[d] == -gdr[i] && DC[d] == -gdc[i]) continue;   /* no reversing */
            dist = (nr - tr) * (nr - tr) + (nc - tc) * (nc - tc);
            if (dist < bestd) { bestd = dist; best = d; }
        }
    }
    if (best < 0) { gdr[i] = -gdr[i]; gdc[i] = -gdc[i]; return; }
    gdr[i] = DR[best]; gdc[i] = DC[best];
    gr[i] += gdr[i]; gc[i] += gdc[i];
}

static void render(void)
{
    int r, c, i;
    for (r = 0; r < H; r++) {
        scr_move(6 + r, 26);
        for (c = 0; c < W; c++) {
            char ch = grid[r][c];
            if (ch == '#')      printf("%s█%s", C_BLUE, C_RESET);
            else if (ch == '.') printf("%s·%s", C_YELLOW, C_RESET);
            else                putchar(' ');
        }
    }
    for (i = 0; i < GHOSTS; i++) {
        static const char *GC[GHOSTS] = {C_RED, C_MAGENTA, C_CYAN, C_GREEN};
        draw_textf(6 + gr[i], 26 + gc[i], "%s%s%s", GC[i], "ᙢ", C_RESET);
    }
    draw_textf(6 + pr, 26 + pc, "%s%s%s", C_BOLD C_YELLOW, "@", C_RESET);
    draw_textf(H + 7, 26, "Score %-6d Lives %-3d Pellets %-4d", score, lives, pellets);
    scr_flush();
}

void fam_pacman(const GParams *p)
{
    (void)p;
    for (;;) {
        long last = now_ms();
        int i, tick = 0;
        score = 0; lives = 3;
        load_maze();
        reset_positions();

        while (lives > 0 && pellets > 0) {
            int k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP)    { dr_ = -1; dc_ = 0; }
            if (k == KEY_DOWN)  { dr_ = 1;  dc_ = 0; }
            if (k == KEY_LEFT)  { dr_ = 0;  dc_ = -1; }
            if (k == KEY_RIGHT) { dr_ = 0;  dc_ = 1; }

            if (now_ms() - last < 130) { sleep_ms(5); continue; }
            last = now_ms();
            tick++;

            if (!wall(pr + dr_, pc + dc_)) { pr += dr_; pc += dc_; }
            if (grid[pr][pc] == '.') { grid[pr][pc] = ' '; score += 10; pellets--; }

            if (tick % 2 == 0)                       /* ghosts move slightly slower */
                for (i = 0; i < GHOSTS; i++) move_ghost(i);

            for (i = 0; i < GHOSTS; i++)
                if (gr[i] == pr && gc[i] == pc) {
                    lives--;
                    reset_positions();
                    sleep_ms(600);
                    break;
                }

            scr_clear();
            draw_title("PACMAN", "Arrows steer, Q quits");
            render();
        }
        draw_centered(H + 9, 80, pellets == 0 ? C_BOLD C_GREEN "Maze cleared!" C_RESET
                                              : C_BOLD C_RED "Game over" C_RESET);
        score_report("pacman", score);
        if (!confirm("\n  Play again?")) return;
    }
}
