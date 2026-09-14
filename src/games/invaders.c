/* invaders.c - formation shooter that accelerates as the ranks thin out. */
#include "engine.h"
#include "games.h"

#define W 48
#define H 22
#define AR 4
#define AC 10
#define MAXB 8

static int alive[AR][AC];
static int ax, ay, adir;
static int ship, score, lives, wave;
static int bx[MAXB], by_[MAXB], bn;
static int ex[MAXB], ey[MAXB], en;

static int alive_count(void)
{
    int r, c, n = 0;
    for (r = 0; r < AR; r++) for (c = 0; c < AC; c++) n += alive[r][c];
    return n;
}

static void reset_wave(void)
{
    int r, c;
    for (r = 0; r < AR; r++) for (c = 0; c < AC; c++) alive[r][c] = 1;
    ax = 2; ay = 1; adir = 1;
    bn = en = 0;
}

static void render(void)
{
    int r, c, i;
    static const char *ACOL[AR] = {C_MAGENTA, C_CYAN, C_GREEN, C_YELLOW};
    draw_box(5, 14, H + 2, W + 2, C_BLUE);
    for (r = 0; r < AR; r++) for (c = 0; c < AC; c++) {
        if (!alive[r][c]) continue;
        draw_textf(6 + ay + r, 15 + ax + c * 4, "%s%s%s", ACOL[r], r == 0 ? "ᗧ" : "ᙢ", C_RESET);
    }
    for (i = 0; i < bn; i++)  draw_textf(6 + by_[i], 15 + bx[i], "%s|%s", C_WHITE, C_RESET);
    for (i = 0; i < en; i++)  draw_textf(6 + ey[i],  15 + ex[i], "%s!%s", C_RED, C_RESET);
    draw_textf(6 + H - 1, 15 + ship, "%s▲%s", C_CYAN, C_RESET);
    draw_textf(H + 8, 14, "Score %-6d Lives %-3d Wave %-3d   ", score, lives, wave);
    scr_flush();
}

void fam_invaders(const GParams *p)
{
    (void)p;
    for (;;) {
        long last = now_ms(), lastfire = 0;
        score = 0; lives = 3; wave = 1;
        ship = W / 2;
        reset_wave();

        while (lives > 0) {
            int k = key_poll();
            int step, i, j;

            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT  && ship > 0)     ship--;
            if (k == KEY_RIGHT && ship < W - 1) ship++;
            if ((k == ' ' || k == KEY_UP) && bn < MAXB && now_ms() - lastfire > 180) {
                bx[bn] = ship; by_[bn] = H - 2; bn++;
                lastfire = now_ms();
            }

            /* Formation speeds up as fewer aliens remain. */
            step = 60 + alive_count() * 12;
            if (now_ms() - last < (long)(step / 4)) { sleep_ms(4); continue; }
            last = now_ms();

            for (i = 0; i < bn; i++) {
                by_[i]--;
                if (by_[i] < 0) { bx[i] = bx[bn-1]; by_[i] = by_[bn-1]; bn--; i--; }
            }
            for (i = 0; i < en; i++) {
                ey[i]++;
                if (ey[i] >= H) { ex[i] = ex[en-1]; ey[i] = ey[en-1]; en--; i--; }
                else if (ey[i] == H - 1 && ex[i] == ship) {
                    lives--;
                    ex[i] = ex[en-1]; ey[i] = ey[en-1]; en--; i--;
                }
            }

            {
                static int phase = 0;
                if (++phase % 4 == 0) {
                    int lo = W, hi = 0, r, c;
                    for (r = 0; r < AR; r++) for (c = 0; c < AC; c++) {
                        if (!alive[r][c]) continue;
                        if (ax + c * 4 < lo) lo = ax + c * 4;
                        if (ax + c * 4 > hi) hi = ax + c * 4;
                    }
                    if ((adir > 0 && hi >= W - 2) || (adir < 0 && lo <= 1)) { adir = -adir; ay++; }
                    else ax += adir;
                    if (ay + AR >= H - 1) lives = 0;
                }
                /* Random alien returns fire from the bottom of its column. */
                if (rnd(100) < 8 && en < MAXB) {
                    int c = rnd(AC), r;
                    for (r = AR - 1; r >= 0; r--)
                        if (alive[r][c]) {
                            ex[en] = ax + c * 4; ey[en] = ay + r + 1; en++;
                            break;
                        }
                }
            }

            for (i = 0; i < bn; i++) {
                int r, c, hit = 0;
                for (r = 0; r < AR && !hit; r++) for (c = 0; c < AC; c++) {
                    if (!alive[r][c]) continue;
                    if (bx[i] == ax + c * 4 && by_[i] == ay + r) {
                        alive[r][c] = 0;
                        score += (AR - r) * 10;
                        bx[i] = bx[bn-1]; by_[i] = by_[bn-1]; bn--; i--;
                        hit = 1;
                        break;
                    }
                }
            }
            if (alive_count() == 0) { wave++; score += 500; reset_wave(); }

            j = 0; (void)j;
            scr_clear();
            draw_title("SPACE INVADERS", "Left/Right move, Space fires, Q quits");
            render();
        }
        draw_centered(H + 10, 80, C_BOLD C_RED "Game over" C_RESET);
        score_report("space-invaders", score);
        if (!confirm("\n  Play again?")) return;
    }
}
