/* GIC:PARAMETERISED hanoi
 * hanoi.c - Towers of Hanoi with a configurable disc count.
 * params: size = discs (3..10)
 */
#include "engine.h"
#include "games.h"

#define MAXD 10
static int DISCS;
static int peg[3][MAXD], height[3], moves;

static int push_disc(int p, int d)
{
    if (height[p] && peg[p][height[p] - 1] < d) return 0;
    peg[p][height[p]++] = d;
    return 1;
}

static void render(int from, int cur, const char *msg)
{
    int p, i, base_row = 6 + DISCS;
    char sub[90];
    snprintf(sub, sizeof sub, "%d discs, optimal is %d moves - Left/Right pick, Enter lifts or drops",
             DISCS, (1 << DISCS) - 1);
    draw_title("TOWERS OF HANOI", sub);

    for (p = 0; p < 3; p++) {
        int cx = 18 + p * 22;
        for (i = 0; i < DISCS; i++) {
            int row = base_row - 1 - i;
            int w = (i < height[p]) ? peg[p][i] : 0;
            scr_move(row, cx - MAXD);
            printf("%*s", MAXD * 2 + 1, "");
            scr_move(row, cx - MAXD);
            if (w) {
                int j;
                printf("%*s", MAXD - w, "");
                fputs(w % 2 ? C_CYAN : C_MAGENTA, stdout);
                for (j = 0; j < w * 2 - 1; j++) fputs("=", stdout);
                fputs(C_RESET, stdout);
            } else {
                printf("%*s|", MAXD - 1, "");
            }
        }
        draw_textf(base_row, cx - 1, "%s %c %s",
                   p == cur ? C_REV : C_GREY, 'A' + p, C_RESET);
    }
    draw_textf(base_row + 2, 18, "Moves: %-6d  Optimal: %-6d", moves, (1 << DISCS) - 1);
    if (from >= 0) draw_textf(base_row + 3, 18, "%sHolding a disc from peg %c%s   ",
                              C_YELLOW, 'A' + from, C_RESET);
    else           draw_text(base_row + 3, 18, "                                     ");
    draw_text(base_row + 5, 16, "                                                    ");
    if (msg) draw_textf(base_row + 5, 18, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_hanoi(const GParams *p)
{
    DISCS = gp_int(p->size, 5);
    if (DISCS < 3) DISCS = 3;
    if (DISCS > MAXD) DISCS = MAXD;

    for (;;) {
        int cur = 0, from = -1, i;
        memset(height, 0, sizeof height);
        for (i = 0; i < DISCS; i++) peg[0][i] = DISCS - i;
        height[0] = DISCS;
        moves = 0;

        while (height[2] < DISCS) {
            int k;
            render(from, cur, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  cur = (cur + 2) % 3;
            if (k == KEY_RIGHT) cur = (cur + 1) % 3;
            if (k != KEY_ENTER && k != ' ') continue;
            if (from < 0) {
                if (height[cur] > 0) from = cur;
            } else if (from == cur) {
                from = -1;
            } else {
                int d = peg[from][height[from] - 1];
                if (push_disc(cur, d)) { height[from]--; moves++; from = -1; }
                else { render(from, cur, "Cannot stack larger on smaller."); sleep_ms(700); }
            }
        }
        {
            char m[80];
            snprintf(m, sizeof m, "Solved in %d moves (optimal %d).", moves, (1 << DISCS) - 1);
            render(-1, cur, m);
            score_report(p->title ? p->title : "hanoi", DISCS * 1000 / (moves + 1));
        }
        if (!confirm("\n  Play again?")) return;
    }
}
