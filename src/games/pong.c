/* pong.c - paddle rally with angle-of-impact deflection and a tracking AI. */
#include "engine.h"
#include "games.h"

#define W 60
#define H 20
#define PADDLE 4

static double by, bx, vx, vy;
static int py_, ay, ps, as;

static void reset_ball(int towards)
{
    bx = W / 2.0; by = H / 2.0;
    vx = towards * 0.9;
    vy = (rnd(2) ? 0.4 : -0.4);
}

static void render(void)
{
    int i;
    draw_box(5, 10, H + 2, W + 2, C_BLUE);
    for (i = 0; i < H; i++) draw_textf(6 + i, 10 + W / 2, "%s│%s", C_GREY, C_RESET);
    for (i = 0; i < PADDLE; i++) {
        draw_textf(6 + py_ + i, 12,         "%s█%s", C_CYAN, C_RESET);
        draw_textf(6 + ay + i,  10 + W - 1, "%s█%s", C_RED,  C_RESET);
    }
    draw_textf(6 + (int)by, 11 + (int)bx, "%s●%s", C_YELLOW, C_RESET);
    draw_textf(4, 10 + W / 2 - 6, "%s%2d%s   %s%2d%s", C_CYAN, ps, C_RESET, C_RED, as, C_RESET);
    scr_flush();
}

void fam_pong(const GParams *p)
{
    (void)p;
    for (;;) {
        long last = now_ms();
        py_ = ay = H / 2 - PADDLE / 2;
        ps = as = 0;
        reset_ball(1);

        while (ps < 7 && as < 7) {
            int k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP   && py_ > 0)          py_--;
            if (k == KEY_DOWN && py_ < H - PADDLE) py_++;

            if (now_ms() - last < 40) { sleep_ms(4); continue; }
            last = now_ms();

            bx += vx; by += vy;
            if (by <= 0)     { by = 0;     vy = -vy; }
            if (by >= H - 1) { by = H - 1; vy = -vy; }

            /* Player paddle: deflection angle depends on where it hits. */
            if (bx <= 2 && vx < 0) {
                if (by >= py_ - 0.5 && by <= py_ + PADDLE) {
                    vx = -vx;
                    vy += ((by - (py_ + PADDLE / 2.0)) / (PADDLE / 2.0)) * 0.45;
                    if (vy > 0.9) vy = 0.9;
                    if (vy < -0.9) vy = -0.9;
                } else { as++; reset_ball(1); }
            }
            if (bx >= W - 2 && vx > 0) {
                if (by >= ay - 0.5 && by <= ay + PADDLE) {
                    vx = -vx;
                    vy += ((by - (ay + PADDLE / 2.0)) / (PADDLE / 2.0)) * 0.45;
                } else { ps++; reset_ball(-1); }
            }

            /* AI tracks the ball but with a deliberate reaction lag. */
            if (vx > 0) {
                double target = by - PADDLE / 2.0;
                if (ay < target - 0.5 && ay < H - PADDLE) ay++;
                else if (ay > target + 0.5 && ay > 0)     ay--;
            }

            scr_clear();
            draw_title("PONG", "Up/Down move, first to 7, Q quits");
            render();
        }
        draw_centered(H + 9, 80, ps > as ? C_BOLD C_GREEN "You win!" C_RESET
                                         : C_BOLD C_RED "Computer wins." C_RESET);
        score_report("pong", ps * 100 - as * 50);
        if (!confirm("\n  Play again?")) return;
    }
}
