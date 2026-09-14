/* GIC:PARAMETERISED breakout
 * breakout.c - the brick-clearing family.
 *
 * params: variant = 0 classic, 1 Arkanoid (drops), 2 multiball, 3 gravity,
 *                   4 boss, 5 endless, 6 ultra
 *
 * The variants change what is on the screen and how the ball behaves: falling
 * capsules that widen or shrink the paddle, up to three balls at once,
 * downward acceleration, a moving boss block that takes many hits, an endless
 * ladder of rebuilt walls, and a faster harder mix. Each is a different run.
 */
#include "engine.h"
#include "games.h"

#define W 48
#define H 22
#define BR 5
#define BC 12
#define MAXBALL 3

static int brick[BR][BC];
static double bx[MAXBALL], by[MAXBALL], vx[MAXBALL], vy[MAXBALL];
static int alive[MAXBALL], nball;
static int paddle, score, lives, level, PADDLE;
static int VAR, boss_x, boss_hp, drop_active, drop_kind;
static double drop_x, drop_y;

static void launch(int i, double x)
{
    bx[i] = x; by[i] = H - 4.0;
    vx[i] = (rnd(2) ? 0.7 : -0.7);
    vy[i] = -0.7;
    alive[i] = 1;
}

static void reset_level(void)
{
    int r, c, i;
    for (r = 0; r < BR; r++) for (c = 0; c < BC; c++) {
        if (VAR == 5 || VAR == 6) brick[r][c] = 1 + rnd(3);   /* tougher walls */
        else brick[r][c] = (BR - r) > 2 ? 2 : 1;
    }
    if (VAR == 4) {
        /* Boss mode clears the wall and puts one heavy target up top. */
        for (r = 0; r < BR; r++) for (c = 0; c < BC; c++) brick[r][c] = 0;
        boss_x = BC / 2;
        boss_hp = 20 + level * 6;
    }
    PADDLE = 7;
    nball = (VAR == 2) ? 3 : 1;
    for (i = 0; i < MAXBALL; i++) alive[i] = 0;
    for (i = 0; i < nball; i++) launch(i, W / 2.0 + i * 2);
    paddle = W / 2 - PADDLE / 2;
    drop_active = 0;
}

static int bricks_left(void)
{
    int r, c, n = 0;
    for (r = 0; r < BR; r++) for (c = 0; c < BC; c++) if (brick[r][c]) n++;
    if (VAR == 4) n += boss_hp > 0 ? boss_hp : 0;
    return n;
}

static int balls_alive(void)
{
    int i, n = 0;
    for (i = 0; i < MAXBALL; i++) if (alive[i]) n++;
    return n;
}

static void render(void)
{
    int r, c, i;
    static const char *RC[BR] = {C_RED, C_MAGENTA, C_YELLOW, C_GREEN, C_CYAN};
    static const char *NAME[7] = {
        "Left/Right move",
        "ARKANOID: catch the falling capsules",
        "MULTIBALL: three at once",
        "GRAVITY: the ball is pulled downwards",
        "BOSS: one heavy target that moves",
        "ENDLESS: the wall rebuilds, tougher each time",
        "ULTRA: fast ball, hard bricks"};
    char sub[100];
    snprintf(sub, sizeof sub, "%s, Q quits", NAME[VAR]);
    draw_title("BREAKOUT", sub);
    draw_box(5, 14, H + 2, W + 2, C_BLUE);
    for (r = 0; r < BR; r++) {
        scr_move(7 + r, 16);
        for (c = 0; c < BC; c++) {
            if (brick[r][c] >= 2)      printf("%s###%s", RC[r], C_RESET);
            else if (brick[r][c] == 1) printf("%s:::%s", RC[r], C_RESET);
            else                       printf("   ");
        }
    }
    if (VAR == 4 && boss_hp > 0)
        draw_textf(7, 16 + boss_x * 3, "%s[BOSS %-3d]%s", C_BOLD C_RED, boss_hp, C_RESET);
    for (i = 0; i < PADDLE; i++) draw_textf(6 + H - 1, 15 + paddle + i, "%s#%s", C_WHITE, C_RESET);
    for (i = 0; i < MAXBALL; i++)
        if (alive[i]) draw_textf(6 + (int)by[i], 15 + (int)bx[i], "%sO%s", C_YELLOW, C_RESET);
    if (drop_active)
        draw_textf(6 + (int)drop_y, 15 + (int)drop_x, "%s%c%s",
                   drop_kind ? C_GREEN : C_RED, drop_kind ? 'W' : 'S', C_RESET);
    draw_textf(H + 8, 14, "Score %-6d Lives %-3d Level %-3d Paddle %-2d  ",
               score, lives, level, PADDLE);
    scr_flush();
}

void fam_breakout(const GParams *p)
{
    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR > 6) VAR = 0;

    for (;;) {
        long last = now_ms();
        int quit = 0;
        score = 0; lives = 3; level = 1;
        reset_level();

        while (lives > 0) {
            int bi;
            int k = key_poll();
            int step = (VAR == 6) ? 24 : 35;
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { quit = 1; break; }
            if (k == KEY_LEFT  && paddle > 0)          paddle -= 2;
            if (k == KEY_RIGHT && paddle < W - PADDLE) paddle += 2;
            if (paddle < 0) paddle = 0;
            if (paddle > W - PADDLE) paddle = W - PADDLE;

            if (now_ms() - last < step) { sleep_ms(4); continue; }
            last = now_ms();

            if (VAR == 4 && boss_hp > 0) {
                boss_x += (rnd(100) < 50) ? 1 : -1;
                if (boss_x < 0) boss_x = 0;
                if (boss_x > BC - 3) boss_x = BC - 3;
            }

            for (bi = 0; bi < MAXBALL; bi++) {
                if (!alive[bi]) continue;
                if (VAR == 3) vy[bi] += 0.035;          /* gravity pulls down */
                bx[bi] += vx[bi]; by[bi] += vy[bi];
                if (bx[bi] <= 0)     { bx[bi] = 0;     vx[bi] = -vx[bi]; }
                if (bx[bi] >= W - 1) { bx[bi] = W - 1; vx[bi] = -vx[bi]; }
                if (by[bi] <= 0)     { by[bi] = 0;     vy[bi] = -vy[bi]; }

                {
                    int r = (int)by[bi] - 2, c = (int)bx[bi] / (W / BC);
                    if (VAR == 4 && r == 0 && boss_hp > 0 && c >= boss_x && c <= boss_x + 2) {
                        boss_hp--;
                        score += 25;
                        vy[bi] = -vy[bi];
                    } else if (r >= 0 && r < BR && c >= 0 && c < BC && brick[r][c]) {
                        brick[r][c]--;
                        score += 10;
                        vy[bi] = -vy[bi];
                        /* Arkanoid sometimes drops a capsule from the brick. */
                        if (VAR == 1 && !drop_active && rnd(100) < 18) {
                            drop_active = 1;
                            drop_kind = rnd(2);
                            drop_x = bx[bi];
                            drop_y = by[bi];
                        }
                    }
                }

                if ((int)by[bi] >= H - 2 && vy[bi] > 0 &&
                    bx[bi] >= paddle - 1 && bx[bi] <= paddle + PADDLE) {
                    double hit = (bx[bi] - (paddle + PADDLE / 2.0)) / (PADDLE / 2.0);
                    vy[bi] = -(vy[bi] < 0 ? -vy[bi] : vy[bi]);
                    vx[bi] += hit * 0.5;
                    if (vx[bi] > 1.1)  vx[bi] = 1.1;
                    if (vx[bi] < -1.1) vx[bi] = -1.1;
                    by[bi] = H - 2;
                }
                if (by[bi] >= H - 1) alive[bi] = 0;
            }

            if (drop_active) {
                drop_y += 0.35;
                if (drop_y >= H - 2 && drop_x >= paddle && drop_x <= paddle + PADDLE) {
                    if (drop_kind && PADDLE < 13) PADDLE += 2;
                    else if (!drop_kind && PADDLE > 3) PADDLE -= 2;
                    drop_active = 0;
                } else if (drop_y >= H - 1) drop_active = 0;
            }

            if (balls_alive() == 0) {
                lives--;
                if (lives > 0) {
                    int i;
                    nball = (VAR == 2) ? 3 : 1;
                    for (i = 0; i < nball; i++) launch(i, W / 2.0 + i * 2);
                    sleep_ms(400);
                }
            }
            if (bricks_left() == 0) {
                level++;
                score += 200;
                reset_level();
            }
            scr_clear();
            render();
        }
        if (quit) return;
        draw_centered(H + 10, 80, C_BOLD C_RED "Game over" C_RESET);
        score_report(p->title ? p->title : "breakout", score);
        if (!confirm("\n  Play again?")) return;
    }
}
