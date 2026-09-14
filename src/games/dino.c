/* dino.c - one-button endless runner over an accelerating cactus field. */
#include "engine.h"
#include "games.h"

#define W 60
#define GROUND 14

static int obstacle[W];

void fam_dino(const GParams *p)
{
    (void)p;
    for (;;) {
        double y = 0, vy = 0;
        int score = 0, dead = 0, gap = 0;
        double speed = 55;
        long last = now_ms();
        memset(obstacle, 0, sizeof obstacle);

        while (!dead) {
            int k = key_poll(), i;
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if ((k == ' ' || k == KEY_UP) && y == 0) vy = 2.4;

            if (now_ms() - last < (long)speed) { sleep_ms(3); continue; }
            last = now_ms();

            y += vy;
            vy -= 0.42;                      /* gravity */
            if (y <= 0) { y = 0; vy = 0; }

            for (i = 0; i < W - 1; i++) obstacle[i] = obstacle[i + 1];
            obstacle[W - 1] = 0;
            if (--gap <= 0) {
                obstacle[W - 1] = 1;
                gap = 12 + rnd(14);
            }

            if (obstacle[6] && y < 1.6) dead = 1;
            score++;
            if (speed > 22) speed -= 0.06;   /* accelerate steadily */

            scr_clear();
            draw_title("DINO RUN", "Space or Up jumps, Q quits");
            for (i = 0; i < W; i++)
                if (obstacle[i]) {
                    draw_textf(GROUND - 1, 12 + i, "%s♣%s", C_GREEN, C_RESET);
                    draw_textf(GROUND,     12 + i, "%s║%s", C_GREEN, C_RESET);
                }
            draw_textf(GROUND - (int)y,     18, "%s🦖%s", C_YELLOW, C_RESET);
            draw_hline(GROUND + 1, 12, W, C_GREY);
            draw_textf(GROUND + 3, 12, "Score %-6d Best %-6d", score, score_load("dino-run"));
            scr_flush();
        }
        draw_centered(GROUND + 5, 80, C_BOLD C_RED "Crashed!" C_RESET);
        score_report("dino-run", score);
        if (!confirm("\n  Play again?")) return;
    }
}
