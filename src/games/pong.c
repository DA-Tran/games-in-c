/* GIC:PARAMETERISED pong
 * pong.c - the paddle-rally family.
 *
 * params: variant = 0 classic, 1 curve, 2 obstacles, 3 shrinking paddle,
 *                   4 four-player, 5 air hockey
 *
 * Curve adds spin that keeps bending the flight after the bounce; obstacles
 * put blocks in the court; the shrinking paddle costs you length every rally;
 * four-player adds top and bottom walls with their own defenders; air hockey
 * gives both sides a free-moving striker and a puck that slides. Each needs
 * its own physics step, not just a different backdrop.
 */
#include "engine.h"
#include "games.h"

#define W 60
#define H 20
#define MAXBLOCK 8

static double by, bx, vx, vy, spin;
static int py_, ay, ps, as, paddle;
static int VAR;
static int bkx[MAXBLOCK], bky[MAXBLOCK], nblock;
static int tx, bxp;                 /* four-player: top and bottom defenders */
static double phx, phy;             /* air hockey: your striker             */

static void reset_ball(int towards)
{
    bx = W / 2.0; by = H / 2.0;
    vx = towards * 0.9;
    vy = (rnd(2) ? 0.4 : -0.4);
    spin = 0;
}

static void build_blocks(void)
{
    int i;
    nblock = (VAR == 2) ? 5 : 0;
    for (i = 0; i < nblock; i++) {
        bkx[i] = W / 2 - 8 + rnd(16);
        bky[i] = 2 + rnd(H - 4);
    }
}

static void render(void)
{
    int i;
    char sub[110];
    snprintf(sub, sizeof sub, "%s — Up/Down move%s, first to 7, Q quits",
             VAR == 1 ? "CURVE: the ball carries spin"
           : VAR == 2 ? "OBSTACLES in the court"
           : VAR == 3 ? "SHRINKING: your paddle shortens each rally"
           : VAR == 4 ? "FOUR-PLAYER: defend two walls"
           : VAR == 5 ? "AIR HOCKEY: free-moving striker"
                      : "CLASSIC",
             VAR == 5 ? ", Left/Right too" : "");
    draw_title("PONG", sub);
    draw_box(5, 10, H + 2, W + 2, C_BLUE);
    for (i = 0; i < H; i++) draw_textf(6 + i, 10 + W / 2, "%s|%s", C_GREY, C_RESET);
    for (i = 0; i < nblock; i++)
        draw_textf(6 + bky[i], 11 + bkx[i], "%s#%s", C_MAGENTA, C_RESET);

    if (VAR == 5) {
        draw_textf(6 + (int)phy, 11 + (int)phx, "%sU%s", C_CYAN, C_RESET);
        for (i = 0; i < paddle; i++)
            draw_textf(6 + ay + i, 10 + W - 1, "%s#%s", C_RED, C_RESET);
    } else {
        for (i = 0; i < paddle; i++) {
            draw_textf(6 + py_ + i, 12,        "%s#%s", C_CYAN, C_RESET);
            draw_textf(6 + ay + i, 10 + W - 1, "%s#%s", C_RED,  C_RESET);
        }
    }
    if (VAR == 4) {
        for (i = 0; i < 6; i++) {
            draw_textf(6, 11 + tx + i,       "%s=%s", C_YELLOW, C_RESET);
            draw_textf(5 + H, 11 + bxp + i,  "%s=%s", C_GREEN, C_RESET);
        }
    }
    draw_textf(6 + (int)by, 11 + (int)bx, "%sO%s", C_YELLOW, C_RESET);
    draw_textf(4, 10 + W / 2 - 6, "%s%2d%s   %s%2d%s", C_CYAN, ps, C_RESET, C_RED, as, C_RESET);
    scr_flush();
}

void fam_pong(const GParams *p)
{
    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR > 5) VAR = 0;

    for (;;) {
        long last = now_ms();
        int quit = 0;
        paddle = 4;
        py_ = ay = H / 2 - paddle / 2;
        phx = 4; phy = H / 2.0;
        tx = bxp = W / 2 - 3;
        ps = as = 0;
        build_blocks();
        reset_ball(1);

        while (ps < 7 && as < 7) {
            int k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { quit = 1; break; }
            if (VAR == 5) {
                if (k == KEY_UP    && phy > 1)         phy -= 1;
                if (k == KEY_DOWN  && phy < H - 2)     phy += 1;
                if (k == KEY_LEFT  && phx > 1)         phx -= 1;
                if (k == KEY_RIGHT && phx < W / 2 - 2) phx += 1;
            } else {
                if (k == KEY_UP   && py_ > 0)          py_--;
                if (k == KEY_DOWN && py_ < H - paddle) py_++;
            }

            if (now_ms() - last < 40) { sleep_ms(4); continue; }
            last = now_ms();

            if (VAR == 1) vy += spin * 0.06;      /* curve keeps bending */
            bx += vx; by += vy;

            if (VAR == 4) {
                /* Top and bottom are goals too, guarded by their own bars. */
                if (by <= 0) {
                    if (bx >= tx && bx <= tx + 6) { by = 0; vy = -vy; }
                    else { as++; reset_ball(1); }
                }
                if (by >= H - 1) {
                    if (bx >= bxp && bx <= bxp + 6) { by = H - 1; vy = -vy; }
                    else { ps++; reset_ball(-1); }
                }
            } else {
                if (by <= 0)     { by = 0;     vy = -vy; }
                if (by >= H - 1) { by = H - 1; vy = -vy; }
            }

            {
                int i;
                for (i = 0; i < nblock; i++)
                    if ((int)bx == bkx[i] && (int)by == bky[i]) { vx = -vx; bx += vx; }
            }

            if (VAR == 5) {
                /* Puck deflects off the striker and then drifts. */
                double dxp = bx - phx, dyp = by - phy;
                if (dxp * dxp + dyp * dyp < 2.2 && vx < 0) {
                    vx = -vx;
                    vy += dyp * 0.35;
                }
                vx *= 0.999; vy *= 0.999;
                if (bx <= 1) { as++; reset_ball(1); }
            } else if (bx <= 2 && vx < 0) {
                if (by >= py_ - 0.5 && by <= py_ + paddle) {
                    double off = (by - (py_ + paddle / 2.0)) / (paddle / 2.0);
                    vx = -vx;
                    vy += off * 0.45;
                    if (VAR == 1) spin = off;
                    if (vy > 0.9)  vy = 0.9;
                    if (vy < -0.9) vy = -0.9;
                    if (VAR == 3 && paddle > 2) paddle--;   /* it shortens */
                } else { as++; reset_ball(1); if (VAR == 3) paddle = 4; }
            }

            if (bx >= W - 2 && vx > 0) {
                if (by >= ay - 0.5 && by <= ay + paddle) {
                    vx = -vx;
                    vy += ((by - (ay + paddle / 2.0)) / (paddle / 2.0)) * 0.45;
                } else { ps++; reset_ball(-1); }
            }

            if (vx > 0) {
                double target = by - paddle / 2.0;
                if (ay < target - 0.5 && ay < H - paddle) ay++;
                else if (ay > target + 0.5 && ay > 0)     ay--;
            }
            if (VAR == 4) {
                /* The two extra defenders chase the ball horizontally. */
                if (vy < 0) { if (tx < bx - 3) tx++; else if (tx > bx - 3) tx--; }
                else        { if (bxp < bx - 3) bxp++; else if (bxp > bx - 3) bxp--; }
                if (tx < 0) tx = 0;
                if (tx > W - 7) tx = W - 7;
                if (bxp < 0) bxp = 0;
                if (bxp > W - 7) bxp = W - 7;
            }

            scr_clear();
            render();
        }
        if (quit) return;
        draw_centered(H + 9, 80, ps > as ? C_BOLD C_GREEN "You win!" C_RESET
                                         : C_BOLD C_RED "Computer wins." C_RESET);
        score_report(p->title ? p->title : "pong", ps * 100 - as * 50);
        if (!confirm("\n  Play again?")) return;
    }
}
