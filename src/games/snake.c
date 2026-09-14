/* snake.c - growth and collision, with wrap-free walls and rising speed. */
#include "engine.h"
#include "games.h"

#define W 40
#define H 20
#define MAXLEN (W * H)

static int sx[MAXLEN], sy[MAXLEN], len;
static int dx, dy, fx, fy, score;

static int on_snake(int x, int y)
{
    int i;
    for (i = 0; i < len; i++) if (sx[i] == x && sy[i] == y) return 1;
    return 0;
}

static void place_food(void)
{
    do { fx = rnd(W); fy = rnd(H); } while (on_snake(fx, fy));
}

static void render(void)
{
    int i;
    draw_box(5, 18, H + 2, W + 2, C_BLUE);
    for (i = 0; i < len; i++)
        draw_textf(6 + sy[i], 19 + sx[i], "%s%s%s", i ? C_GREEN : C_BOLD C_GREEN,
                   i ? "o" : "@", C_RESET);
    draw_textf(6 + fy, 19 + fx, "%s✱%s", C_RED, C_RESET);
    draw_textf(H + 8, 18, "Score: %-6d Best: %-6d   ", score, score_load("snake"));
    scr_flush();
}

void fam_snake(const GParams *p)
{
    (void)p;
    for (;;) {
        int i, dead = 0, tick = 120;
        long last;
        scr_clear();
        draw_title("SNAKE", "Arrows steer, Q quits");
        len = 4;
        for (i = 0; i < len; i++) { sx[i] = W / 2 - i; sy[i] = H / 2; }
        dx = 1; dy = 0; score = 0;
        place_food();
        last = now_ms();

        while (!dead) {
            int k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && dy == 0) { dx = 0;  dy = -1; }
            if (k == KEY_DOWN  && dy == 0) { dx = 0;  dy = 1;  }
            if (k == KEY_LEFT  && dx == 0) { dx = -1; dy = 0;  }
            if (k == KEY_RIGHT && dx == 0) { dx = 1;  dy = 0;  }

            if (now_ms() - last < tick) { sleep_ms(5); continue; }
            last = now_ms();

            {
                int nx = sx[0] + dx, ny = sy[0] + dy;
                if (nx < 0 || nx >= W || ny < 0 || ny >= H) { dead = 1; break; }
                for (i = 0; i < len - 1; i++)
                    if (sx[i] == nx && sy[i] == ny) { dead = 1; break; }
                if (dead) break;

                for (i = len; i > 0; i--) { sx[i] = sx[i-1]; sy[i] = sy[i-1]; }
                sx[0] = nx; sy[0] = ny;

                if (nx == fx && ny == fy) {
                    if (len < MAXLEN - 1) len++;
                    score += 10;
                    if (tick > 45) tick -= 3;
                    place_food();
                } 
            }
            scr_clear();
            draw_title("SNAKE", "Arrows steer, Q quits");
            render();
        }
        draw_centered(H + 10, 80, C_BOLD C_RED "Game over" C_RESET);
        score_report("snake", score);
        if (!confirm("\n  Play again?")) return;
    }
}
