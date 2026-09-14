/* frogger.c - cross the traffic, then ride the logs across the river. */
#include "engine.h"
#include "games.h"

#define W 44
#define LANES 11     /* 0 = home, 1-4 river, 5 median, 6-9 road, 10 start */

static int offset[LANES];
static int speed[LANES];

void fam_frogger(const GParams *p)
{
    (void)p;
    for (;;) {
        int fx = W / 2, fy = LANES - 1, lives = 3, score = 0, i, home = 0;
        long last = now_ms();

        for (i = 0; i < LANES; i++) {
            offset[i] = rnd(W);
            speed[i] = (i >= 1 && i <= 4) ? (i % 2 ? 1 : -1)
                     : (i >= 6 && i <= 9) ? (i % 2 ? -1 : 1) : 0;
        }

        while (lives > 0) {
            int k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && fy > 0)         { fy--; score += 10; }
            if (k == KEY_DOWN  && fy < LANES - 1) fy++;
            if (k == KEY_LEFT  && fx > 0)         fx--;
            if (k == KEY_RIGHT && fx < W - 1)     fx++;

            if (now_ms() - last < 140) { sleep_ms(5); continue; }
            last = now_ms();

            for (i = 0; i < LANES; i++)
                if (speed[i]) offset[i] = (offset[i] + speed[i] + W) % W;

            /* Rows 1-4 are river: you must be standing on a log. */
            if (fy >= 1 && fy <= 4) {
                int on_log = ((fx + offset[fy]) % 8) < 4;
                if (!on_log) { lives--; fx = W / 2; fy = LANES - 1; }
                else fx = (fx + speed[fy] + W) % W;
            } else if (fy >= 6 && fy <= 9) {
                int car = ((fx + offset[fy]) % 9) < 3;
                if (car) { lives--; fx = W / 2; fy = LANES - 1; }
            } else if (fy == 0) {
                home++;
                score += 100;
                fx = W / 2; fy = LANES - 1;
                if (home >= 5) { score += 500; home = 0; }
            }

            scr_clear();
            draw_title("FROGGER", "Arrows hop, reach the top, Q quits");
            draw_box(5, 16, LANES + 2, W + 2, C_BLUE);
            for (i = 0; i < LANES; i++) {
                int x;
                scr_move(6 + i, 17);
                for (x = 0; x < W; x++) {
                    if (i == 0)                  printf("%s▒%s", C_GREEN, C_RESET);
                    else if (i >= 1 && i <= 4)
                        printf("%s%s%s", ((x + offset[i]) % 8) < 4 ? C_YELLOW : C_BLUE,
                               ((x + offset[i]) % 8) < 4 ? "▬" : "≈", C_RESET);
                    else if (i == 5 || i == LANES - 1) printf("%s░%s", C_GREY, C_RESET);
                    else
                        printf("%s%s%s", ((x + offset[i]) % 9) < 3 ? C_RED : C_BLACK,
                               ((x + offset[i]) % 9) < 3 ? "▄" : " ", C_RESET);
                }
            }
            draw_textf(6 + fy, 17 + fx, "%s%s@%s", C_BOLD, C_GREEN, C_RESET);
            draw_textf(LANES + 8, 16, "Score %-6d Lives %-3d Home %d/5   ", score, lives, home);
            scr_flush();
        }
        draw_centered(LANES + 10, 80, C_BOLD C_RED "Game over" C_RESET);
        score_report("frogger", score);
        if (!confirm("\n  Play again?")) return;
    }
}
