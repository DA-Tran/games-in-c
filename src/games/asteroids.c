/* asteroids.c - inertial flight with screen wrap and splitting rocks. */
#include "engine.h"
#include "games.h"

#define W 60
#define H 22
#define MAXR 16
#define MAXS 6

typedef struct { double x, y, vx, vy; int size, live; } Rock;
typedef struct { double x, y, vx, vy; int life; } Shot;

static Rock rock[MAXR];
static Shot shot[MAXS];
static double sx_, sy_, svx, svy, angle;
static int score, lives;

static void wrap(double *x, double *y)
{
    if (*x < 0) *x += W;
    if (*x >= W) *x -= W;
    if (*y < 0) *y += H;
    if (*y >= H) *y -= H;
}

static void spawn_rocks(int n)
{
    int i;
    memset(rock, 0, sizeof rock);
    for (i = 0; i < n && i < MAXR; i++) {
        rock[i].x = rnd(W);
        rock[i].y = rnd(H);
        rock[i].vx = (rnd_f() - 0.5) * 0.6;
        rock[i].vy = (rnd_f() - 0.5) * 0.4;
        rock[i].size = 3;
        rock[i].live = 1;
    }
}

static int rocks_left(void)
{
    int i, n = 0;
    for (i = 0; i < MAXR; i++) n += rock[i].live;
    return n;
}

static void split(int i)
{
    int j, made = 0;
    rock[i].live = 0;
    score += rock[i].size * 20;
    if (rock[i].size <= 1) return;
    for (j = 0; j < MAXR && made < 2; j++) {
        if (rock[j].live) continue;
        rock[j].x = rock[i].x;
        rock[j].y = rock[i].y;
        rock[j].vx = (rnd_f() - 0.5) * 1.2;
        rock[j].vy = (rnd_f() - 0.5) * 0.8;
        rock[j].size = rock[i].size - 1;
        rock[j].live = 1;
        made++;
    }
}

void fam_asteroids(const GParams *p)
{
    (void)p;
    for (;;) {
        long last = now_ms();
        int i, j, wave = 4;
        score = 0; lives = 3;
        sx_ = W / 2.0; sy_ = H / 2.0; svx = svy = 0; angle = 0;
        memset(shot, 0, sizeof shot);
        spawn_rocks(wave);

        while (lives > 0) {
            int k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  angle -= 0.4;
            if (k == KEY_RIGHT) angle += 0.4;
            if (k == KEY_UP) {
                /* Cheap fixed-point trig via a small lookup of directions. */
                svx += 0.18 * ((angle > -1.6 && angle < 1.6) ? 1 : -1) * 0.6;
                svy -= 0.12 * ((angle > -3.0 && angle < 0.2) ? 1 : -1) * 0.6;
                if (svx > 1.2) svx = 1.2;
                if (svx < -1.2) svx = -1.2;
                if (svy > 0.9) svy = 0.9;
                if (svy < -0.9) svy = -0.9;
            }
            if (k == ' ') {
                for (i = 0; i < MAXS; i++) {
                    if (shot[i].life) continue;
                    shot[i].x = sx_; shot[i].y = sy_;
                    shot[i].vx = svx + (angle > -1.6 && angle < 1.6 ? 1.4 : -1.4);
                    shot[i].vy = svy + (angle > 0.2 ? 0.5 : -0.5);
                    shot[i].life = 18;
                    break;
                }
            }

            if (now_ms() - last < 55) { sleep_ms(4); continue; }
            last = now_ms();

            sx_ += svx; sy_ += svy;
            svx *= 0.985; svy *= 0.985;       /* light drag so it stays playable */
            wrap(&sx_, &sy_);

            for (i = 0; i < MAXS; i++) {
                if (!shot[i].life) continue;
                shot[i].x += shot[i].vx;
                shot[i].y += shot[i].vy;
                wrap(&shot[i].x, &shot[i].y);
                shot[i].life--;
            }
            for (i = 0; i < MAXR; i++) {
                if (!rock[i].live) continue;
                rock[i].x += rock[i].vx;
                rock[i].y += rock[i].vy;
                wrap(&rock[i].x, &rock[i].y);
                if ((int)rock[i].x == (int)sx_ && (int)rock[i].y == (int)sy_) {
                    lives--;
                    sx_ = W / 2.0; sy_ = H / 2.0; svx = svy = 0;
                    sleep_ms(500);
                }
                for (j = 0; j < MAXS; j++) {
                    if (!shot[j].life) continue;
                    if ((int)shot[j].x == (int)rock[i].x && (int)shot[j].y == (int)rock[i].y) {
                        shot[j].life = 0;
                        split(i);
                        break;
                    }
                }
            }
            if (rocks_left() == 0) { wave++; score += 300; spawn_rocks(wave > MAXR ? MAXR : wave); }

            scr_clear();
            draw_title("ASTEROIDS", "Left/Right turn, Up thrusts, Space fires, Q quits");
            draw_box(5, 10, H + 2, W + 2, C_BLUE);
            for (i = 0; i < MAXR; i++) {
                if (!rock[i].live) continue;
                draw_textf(6 + (int)rock[i].y, 11 + (int)rock[i].x, "%s%s%s",
                           C_GREY, rock[i].size == 3 ? "O" : rock[i].size == 2 ? "o" : ".", C_RESET);
            }
            for (i = 0; i < MAXS; i++)
                if (shot[i].life)
                    draw_textf(6 + (int)shot[i].y, 11 + (int)shot[i].x, "%s·%s", C_YELLOW, C_RESET);
            draw_textf(6 + (int)sy_, 11 + (int)sx_, "%s▲%s", C_CYAN, C_RESET);
            draw_textf(H + 8, 10, "Score %-6d Lives %-3d Wave %-3d", score, lives, wave);
            scr_flush();
        }
        draw_centered(H + 10, 80, C_BOLD C_RED "Game over" C_RESET);
        score_report("asteroids", score);
        if (!confirm("\n  Play again?")) return;
    }
}
