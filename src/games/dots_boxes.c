/* GIC:PARAMETERISED dotsboxes
 * dots_boxes.c - edge claiming on any lattice from 3x3 to 7x7 boxes.
 * params: size = boxes per side (3..7)
 */
#include "engine.h"
#include "games.h"

#define MAXB 7
#define MAXD (MAXB + 1)

static int B;               /* boxes per side */
static int D;               /* dots per side  */

static int hedge[MAXD][MAXB];   /* horizontal edges */
static int vedge[MAXB][MAXD];   /* vertical edges   */
static char owner[MAXB][MAXB];
static int score_you, score_cpu;

static int box_closed(int r, int c)
{
    return hedge[r][c] && hedge[r+1][c] && vedge[r][c] && vedge[r][c+1];
}

/* Claim any newly completed boxes; return how many were closed. */
static int claim(char who)
{
    int r, c, n = 0;
    for (r = 0; r < B; r++) for (c = 0; c < B; c++)
        if (owner[r][c] == ' ' && box_closed(r, c)) {
            owner[r][c] = who;
            n++;
            if (who == 'Y') score_you++; else score_cpu++;
        }
    return n;
}

static int edges_left(void)
{
    int r, c, n = 0;
    for (r = 0; r < D; r++) for (c = 0; c < B; c++) if (!hedge[r][c]) n++;
    for (r = 0; r < B; r++) for (c = 0; c < D; c++) if (!vedge[r][c]) n++;
    return n;
}

static int box_sides(int r, int c)
{
    return hedge[r][c] + hedge[r+1][c] + vedge[r][c] + vedge[r][c+1];
}

/* Prefer completing boxes; otherwise avoid handing over a third side. */
static void ai_turn(void)
{
    for (;;) {
        int r, c, br = -1, bc = -1, bh = 0, safe_r = -1, safe_c = -1, safe_h = 0;
        for (r = 0; r < D && br < 0; r++) for (c = 0; c < B; c++) {
            if (hedge[r][c]) continue;
            hedge[r][c] = 1;
            if ((r < B && box_closed(r, c)) || (r > 0 && box_closed(r-1, c))) {
                hedge[r][c] = 0; br = r; bc = c; bh = 1; break;
            }
            if ((r >= B || box_sides(r, c) < 3) && (r == 0 || box_sides(r-1, c) < 3)) {
                safe_r = r; safe_c = c; safe_h = 1;
            }
            hedge[r][c] = 0;
        }
        if (br < 0) {
            for (r = 0; r < B && br < 0; r++) for (c = 0; c < D; c++) {
                if (vedge[r][c]) continue;
                vedge[r][c] = 1;
                if ((c < B && box_closed(r, c)) || (c > 0 && box_closed(r, c-1))) {
                    vedge[r][c] = 0; br = r; bc = c; bh = 0; break;
                }
                if ((c >= B || box_sides(r, c) < 3) && (c == 0 || box_sides(r, c-1) < 3)) {
                    safe_r = r; safe_c = c; safe_h = 0;
                }
                vedge[r][c] = 0;
            }
        }
        if (br < 0) {
            if (safe_r >= 0) { br = safe_r; bc = safe_c; bh = safe_h; }
            else {
                for (r = 0; r < D; r++) for (c = 0; c < B; c++)
                    if (!hedge[r][c] && br < 0) { br = r; bc = c; bh = 1; }
                for (r = 0; r < B && br < 0; r++) for (c = 0; c < D; c++)
                    if (!vedge[r][c] && br < 0) { br = r; bc = c; bh = 0; }
            }
        }
        if (br < 0) return;
        if (bh) hedge[br][bc] = 1; else vedge[br][bc] = 1;
        if (!claim('C')) return;
        if (edges_left() == 0) return;
    }
}

static void render(int cr, int cc, int horiz, const char *msg)
{
    int r, c, left = 40 - (D * 4) / 2;
    char sub[90];
    snprintf(sub, sizeof sub, "%dx%d boxes - arrows move, Tab switches edge, Enter draws",
             B, B);
    draw_title("DOTS AND BOXES", sub);
    for (r = 0; r < D; r++) {
        scr_move(7 + r * 2, left);
        for (c = 0; c < D; c++) {
            printf("%s•%s", C_WHITE, C_RESET);
            if (c < B) {
                int sel = horiz && r == cr && c == cc;
                printf("%s%s%s", sel ? C_REV : (hedge[r][c] ? C_GREEN : C_GREY),
                       hedge[r][c] ? "───" : " · ", C_RESET);
            }
        }
        if (r < B) {
            scr_move(8 + r * 2, left);
            for (c = 0; c < D; c++) {
                int sel = !horiz && r == cr && c == cc;
                printf("%s%s%s", sel ? C_REV : (vedge[r][c] ? C_GREEN : C_GREY),
                       vedge[r][c] ? "│" : "·", C_RESET);
                if (c < B)
                    printf(" %s%c%s ", owner[r][c] == 'Y' ? C_CYAN : C_RED,
                           owner[r][c] == ' ' ? ' ' : owner[r][c], C_RESET);
            }
        }
    }
    draw_textf(8 + D * 2, left, "%sYou %d%s   %sCPU %d%s   ", C_CYAN, score_you, C_RESET,
               C_RED, score_cpu, C_RESET);
    draw_text(10 + D * 2, 20, "                                                      ");
    if (msg) draw_textf(10 + D * 2, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_dots_boxes(const GParams *p)
{
    B = gp_int(p->size, 4);
    if (B < 3) B = 3;
    if (B > MAXB) B = MAXB;
    D = B + 1;

    for (;;) {
        int cr = 0, cc = 0, horiz = 1, r, c, over = 0;
        memset(hedge, 0, sizeof hedge);
        memset(vedge, 0, sizeof vedge);
        for (r = 0; r < B; r++) for (c = 0; c < B; c++) owner[r][c] = ' ';
        score_you = score_cpu = 0;

        while (!over) {
            int k, maxr = horiz ? D - 1 : B - 1, maxc = horiz ? B - 1 : D - 1;
            if (cr > maxr) cr = maxr;
            if (cc > maxc) cc = maxc;
            render(cr, cc, horiz, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_TAB) { horiz = !horiz; continue; }
            if (k == KEY_UP    && cr > 0)    cr--;
            if (k == KEY_DOWN  && cr < maxr) cr++;
            if (k == KEY_LEFT  && cc > 0)    cc--;
            if (k == KEY_RIGHT && cc < maxc) cc++;
            if (k != KEY_ENTER && k != ' ') continue;
            if (horiz ? hedge[cr][cc] : vedge[cr][cc]) continue;

            if (horiz) hedge[cr][cc] = 1; else vedge[cr][cc] = 1;
            if (!claim('Y') && edges_left() > 0) ai_turn();
            if (edges_left() == 0) {
                render(cr, cc, horiz, score_you > score_cpu ? "You win!" :
                                      score_you < score_cpu ? "Computer wins." : "Draw.");
                score_report(p->title ? p->title : "dotsboxes", score_you);
                over = 1;
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
