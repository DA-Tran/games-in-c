/* flappy.c - tap against gravity and thread the gaps. */
#include "engine.h"
#include "games.h"

#define W 50
#define H 20
#define PIPES 4

static int px[PIPES], pgap[PIPES];

void fam_flappy(const GParams *p)
{
    (void)p;
    for (;;) {
        double y = H / 2.0, vy = 0;
        int score = 0, dead = 0, i;
        long last = now_ms();

        for (i = 0; i < PIPES; i++) {
            px[i] = W + i * (W / PIPES);
            pgap[i] = 3 + rnd(H - 9);
        }

        while (!dead) {
            int k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == ' ' || k == KEY_UP) vy = -1.15;

            if (now_ms() - last < 70) { sleep_ms(3); continue; }
            last = now_ms();

            vy += 0.32;
            y += vy;
            if (y < 0 || y >= H) dead = 1;

            for (i = 0; i < PIPES; i++) {
                px[i]--;
                if (px[i] < -2) { px[i] = W; pgap[i] = 3 + rnd(H - 9); score++; }
                if (px[i] == 8 && ((int)y < pgap[i] || (int)y > pgap[i] + 4)) dead = 1;
            }

            scr_clear();
            draw_title("FLAPPY", "Space or Up flaps, Q quits");
            draw_box(5, 14, H + 2, W + 2, C_BLUE);
            for (i = 0; i < PIPES; i++) {
                int r;
                if (px[i] < 0 || px[i] >= W) continue;
                for (r = 0; r < H; r++)
                    if (r < pgap[i] || r > pgap[i] + 4)
                        draw_textf(6 + r, 15 + px[i], "%s█%s", C_GREEN, C_RESET);
            }
            if ((int)y >= 0 && (int)y < H)
                draw_textf(6 + (int)y, 15 + 8, "%s●%s", C_YELLOW, C_RESET);
            draw_textf(H + 8, 14, "Score %-5d Best %-5d", score, score_load("flappy"));
            scr_flush();
        }
        draw_centered(H + 10, 80, C_BOLD C_RED "Crashed!" C_RESET);
        score_report("flappy", score);
        if (!confirm("\n  Play again?")) return;
    }
}
