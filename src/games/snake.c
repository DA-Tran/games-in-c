/* GIC:PARAMETERISED snake
 * snake.c - the growing-worm family.
 *
 * params: variant = 0 classic, 1 wrap, 2 maze, 3 speed, 4 portal,
 *                   5 shrinking arena, 6 poison, 7 nibbles
 *
 * Each variant changes a rule rather than a colour: walls that kill versus
 * walls that wrap, interior obstacles, a starting speed, paired teleports,
 * an arena that closes in, food that must be avoided, and a level-based
 * campaign. The collision test and the food table both depend on which.
 */
#include "engine.h"
#include "games.h"

#define W 40
#define H 20
#define MAXLEN (W * H)
#define MAXFOOD 6

static int sx[MAXLEN], sy[MAXLEN], len;
static int dx, dy, score;
static char wall[H][W];
static int fx[MAXFOOD], fy[MAXFOOD], fbad[MAXFOOD], nfood;
static int px[2], py[2], nportal;
static int VAR, margin;

static int on_snake(int x, int y)
{
    int i;
    for (i = 0; i < len; i++) if (sx[i] == x && sy[i] == y) return 1;
    return 0;
}

static int blocked(int x, int y)
{
    if (VAR == 5) {
        /* the arena closes in as you score */
        if (x < margin || x >= W - margin || y < margin || y >= H - margin) return 1;
    }
    return wall[y][x];
}

static void place_one(int i)
{
    do {
        fx[i] = rnd(W); fy[i] = rnd(H);
    } while (on_snake(fx[i], fy[i]) || blocked(fx[i], fy[i]));
    /* Poison mode salts the board with food that kills. */
    fbad[i] = (VAR == 6 && i > 0 && rnd(100) < 55);
}

static void build_walls(int level)
{
    int i, x, y;
    memset(wall, 0, sizeof wall);
    nportal = 0;
    if (VAR == 2 || VAR == 7) {
        int blocks = (VAR == 7) ? 3 + level * 2 : 8;
        for (i = 0; i < blocks; i++) {
            int bx = 3 + rnd(W - 10), by = 2 + rnd(H - 6);
            int lenr = 3 + rnd(6), vert = rnd(2);
            for (x = 0; x < lenr; x++) {
                int wx = vert ? bx : bx + x, wy = vert ? by + x : by;
                if (wx > 0 && wx < W - 1 && wy > 0 && wy < H - 1) wall[wy][wx] = 1;
            }
        }
        /* never wall in the spawn corridor */
        for (x = W / 2 - 6; x <= W / 2 + 2; x++)
            if (x > 0 && x < W) wall[H / 2][x] = 0;
    }
    if (VAR == 4) {
        nportal = 2;
        px[0] = 5;      py[0] = 3;
        px[1] = W - 6;  py[1] = H - 4;
        for (i = 0; i < 2; i++) wall[py[i]][px[i]] = 0;
    }
    (void)y;
}

static void render(const char *title)
{
    int i, x, y;
    draw_box(4, 18, H + 2, W + 2, C_BLUE);
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            const char *g = " ";
            const char *col = C_GREY;
            if (wall[y][x]) { g = "#"; col = C_BLUE; }
            else if (VAR == 5 && blocked(x, y)) { g = "%"; col = C_GREY; }
            draw_textf(5 + y, 19 + x, "%s%s%s", col, g, C_RESET);
        }
    for (i = 0; i < nportal; i++)
        draw_textf(5 + py[i], 19 + px[i], "%sO%s", C_MAGENTA, C_RESET);
    for (i = 0; i < len; i++)
        draw_textf(5 + sy[i], 19 + sx[i], "%s%s%s", i ? C_GREEN : C_BOLD C_GREEN,
                   i ? "o" : "@", C_RESET);
    for (i = 0; i < nfood; i++)
        draw_textf(5 + fy[i], 19 + fx[i], "%s%s%s",
                   fbad[i] ? C_RED : C_YELLOW, fbad[i] ? "x" : "*", C_RESET);
    draw_textf(H + 7, 18, "Score: %-6d Best: %-6d   ", score, score_load(title));
    scr_flush();
}

void fam_snake(const GParams *p)
{
    const char *slug = p->title ? p->title : "snake";
    static const char *SUB[8] = {
        "Arrows steer, Q quits",
        "WRAP: the walls are open — you reappear on the far side",
        "MAZE: obstacles block the arena",
        "SPEED: starts fast and keeps accelerating",
        "PORTAL: the two rings are linked",
        "SHRINKING: the arena closes in as you eat",
        "POISON: red food kills — eat only the yellow",
        "NIBBLES: clear the target each level, then the maze grows"
    };
    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR > 7) VAR = 0;

    for (;;) {
        int i, dead = 0, level = 1, quit = 0;
        int tick = (VAR == 3) ? 70 : 120;
        int eaten = 0, need = 5;
        long last;

        score = 0;
        margin = 0;
        nfood = (VAR == 6) ? 4 : 1;
        build_walls(level);
        len = 4;
        for (i = 0; i < len; i++) { sx[i] = W / 2 - i; sy[i] = H / 2; }
        dx = 1; dy = 0;
        for (i = 0; i < nfood; i++) place_one(i);
        if (VAR == 6) fbad[0] = 0;
        scr_clear();
        draw_title("SNAKE", SUB[VAR]);
        last = now_ms();

        while (!dead) {
            int k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { quit = 1; break; }
            if (k == KEY_UP    && dy == 0) { dx = 0;  dy = -1; }
            if (k == KEY_DOWN  && dy == 0) { dx = 0;  dy = 1;  }
            if (k == KEY_LEFT  && dx == 0) { dx = -1; dy = 0;  }
            if (k == KEY_RIGHT && dx == 0) { dx = 1;  dy = 0;  }

            if (now_ms() - last < tick) { sleep_ms(5); continue; }
            last = now_ms();

            {
                int nx = sx[0] + dx, ny = sy[0] + dy;

                if (VAR == 1) {                       /* wrap instead of dying */
                    nx = (nx + W) % W;
                    ny = (ny + H) % H;
                } else if (nx < 0 || nx >= W || ny < 0 || ny >= H) {
                    dead = 1; break;
                }
                if (blocked(nx, ny)) { dead = 1; break; }
                for (i = 0; i < len - 1; i++)
                    if (sx[i] == nx && sy[i] == ny) { dead = 1; break; }
                if (dead) break;

                for (i = 0; i < nportal; i++)
                    if (nx == px[i] && ny == py[i]) {
                        nx = px[1 - i]; ny = py[1 - i];
                        break;
                    }

                for (i = len; i > 0; i--) { sx[i] = sx[i-1]; sy[i] = sy[i-1]; }
                sx[0] = nx; sy[0] = ny;

                for (i = 0; i < nfood; i++) {
                    if (nx != fx[i] || ny != fy[i]) continue;
                    if (fbad[i]) { dead = 1; break; }
                    if (len < MAXLEN - 1) len++;
                    score += 10;
                    eaten++;
                    if (tick > 45) tick -= 3;
                    if (VAR == 5 && eaten % 4 == 0 && margin < 6) margin++;
                    place_one(i);
                    if (VAR == 7 && eaten >= need) {   /* next nibbles level */
                        level++;
                        need += 5;
                        build_walls(level);
                        len = 4;
                        sx[0] = W / 2; sy[0] = H / 2;
                        dx = 1; dy = 0;
                        place_one(0);
                    }
                    break;
                }
            }
            scr_clear();
            draw_title("SNAKE", SUB[VAR]);
            render(slug);
        }
        if (quit) return;
        draw_centered(H + 9, 80, C_BOLD C_RED "Game over" C_RESET);
        score_report(slug, score);
        if (!confirm("\n  Play again?")) return;
    }
}
