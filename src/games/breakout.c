/* breakout.c - brick clearing with paddle-english and multi-hit bricks. */
#include "engine.h"
#include "games.h"

#define W 48
#define H 22
#define BR 5
#define BC 12
#define PADDLE 7

static int brick[BR][BC];
static double bx, by, vx, vy;
static int paddle, score, lives, level;

static void reset_level(void)
{
    int r, c;
    for (r = 0; r < BR; r++) for (c = 0; c < BC; c++)
        brick[r][c] = (BR - r) > 2 ? 2 : 1;     /* top rows take two hits */
    bx = W / 2.0; by = H - 4.0;
    vx = 0.7; vy = -0.7;
    paddle = W / 2 - PADDLE / 2;
}

static int bricks_left(void)
{
    int r, c, n = 0;
    for (r = 0; r < BR; r++) for (c = 0; c < BC; c++) if (brick[r][c]) n++;
    return n;
}

static void render(void)
{
    int r, c, i;
    static const char *RC[BR] = {C_RED, C_MAGENTA, C_YELLOW, C_GREEN, C_CYAN};
    draw_box(5, 14, H + 2, W + 2, C_BLUE);
    for (r = 0; r < BR; r++) {
        scr_move(7 + r, 16);
        for (c = 0; c < BC; c++) {
            if (brick[r][c] == 2)      printf("%s███%s", RC[r], C_RESET);
            else if (brick[r][c] == 1) printf("%s▒▒▒%s", RC[r], C_RESET);
            else                       printf("   ");
        }
    }
    for (i = 0; i < PADDLE; i++) draw_textf(6 + H - 1, 15 + paddle + i, "%s█%s", C_WHITE, C_RESET);
    draw_textf(6 + (int)by, 15 + (int)bx, "%s●%s", C_YELLOW, C_RESET);
    draw_textf(H + 8, 14, "Score %-6d Lives %-3d Level %-3d   ", score, lives, level);
    scr_flush();
}

void fam_breakout(const GParams *p)
{
    (void)p;
    for (;;) {
        long last = now_ms();
        score = 0; lives = 3; level = 1;
        reset_level();

        while (lives > 0) {
            int k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT  && paddle > 0)          paddle -= 2;
            if (k == KEY_RIGHT && paddle < W - PADDLE) paddle += 2;
            if (paddle < 0) paddle = 0;
            if (paddle > W - PADDLE) paddle = W - PADDLE;

            if (now_ms() - last < 35) { sleep_ms(4); continue; }
            last = now_ms();

            bx += vx; by += vy;
            if (bx <= 0)     { bx = 0;     vx = -vx; }
            if (bx >= W - 1) { bx = W - 1; vx = -vx; }
            if (by <= 0)     { by = 0;     vy = -vy; }

            /* Brick collision: rows occupy screen lines 2..2+BR. */
            {
                int r = (int)by - 2, c = (int)bx / (W / BC);
                if (r >= 0 && r < BR && c >= 0 && c < BC && brick[r][c]) {
                    brick[r][c]--;
                    score += 10;
                    vy = -vy;
                }
            }

            /* Paddle bounce: where it lands sets the outgoing angle. */
            if ((int)by >= H - 2 && vy > 0 && bx >= paddle - 1 && bx <= paddle + PADDLE) {
                double hit = (bx - (paddle + PADDLE / 2.0)) / (PADDLE / 2.0);
                vy = -(vy < 0 ? -vy : vy);
                vx += hit * 0.5;
                if (vx > 1.1)  vx = 1.1;
                if (vx < -1.1) vx = -1.1;
                by = H - 2;
            }
            if (by >= H - 1) {
                lives--;
                if (lives > 0) { bx = W / 2.0; by = H - 4.0; vx = 0.7; vy = -0.7; sleep_ms(500); }
            }
            if (bricks_left() == 0) {
                level++;
                score += 200;
                reset_level();
            }
            scr_clear();
            draw_title("BREAKOUT", "Left/Right move, Q quits");
            render();
        }
        draw_centered(H + 10, 80, C_BOLD C_RED "Game over" C_RESET);
        score_report("breakout", score);
        if (!confirm("\n  Play again?")) return;
    }
}
