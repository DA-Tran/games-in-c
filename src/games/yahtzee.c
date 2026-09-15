/* GIC:PARAMETERISED yahtzee
 * yahtzee.c - the five-dice scorecard family.
 *
 * params: variant = 0 classic, 1 triple scorecard, 2 six dice,
 *                   3 duplicate (shared rolls against the computer),
 *                   4 speed (two rolls), 5 solo challenge (beat a target)
 *
 * Dice count, rolls per turn, how many scorecards you fill, whether an
 * opponent plays the identical dice, and whether there is a target to beat are
 * all separate switches, so the six entries really are six games.
 */
#include "engine.h"
#include "games.h"

#define CATS 13
#define MAXDICE 6
#define MAXCOL 3

static const char *CAT_NAME[CATS] = {
    "Ones","Twos","Threes","Fours","Fives","Sixes",
    "Three of a Kind","Four of a Kind","Full House",
    "Small Straight","Large Straight","Yahtzee","Chance"
};

static int ND, NROLL, NCOL, DUP, TARGET;
static int dice[MAXDICE], keep[MAXDICE];
static int used[MAXCOL][CATS], sc[MAXCOL][CATS];
static int cpu_used[CATS], cpu_sc[CATS];

static int count_face(int f)
{
    int i, n = 0;
    for (i = 0; i < ND; i++) if (dice[i] == f) n++;
    return n;
}

/* Sum of the n highest dice, so six-dice play does not inflate the
 * set categories simply by having an extra die on the table. */
static int sum_best(int n)
{
    int sorted[MAXDICE], i, j, t, s = 0;
    for (i = 0; i < ND; i++) sorted[i] = dice[i];
    for (i = 0; i < ND; i++) for (j = i + 1; j < ND; j++)
        if (sorted[j] > sorted[i]) { t = sorted[i]; sorted[i] = sorted[j]; sorted[j] = t; }
    for (i = 0; i < n && i < ND; i++) s += sorted[i];
    return s;
}

static int score_for(int cat)
{
    int f, counts[7] = {0}, i, three = 0, four = 0, five = 0, pair = 0, triple = 0;
    for (i = 0; i < ND; i++) counts[dice[i]]++;
    for (f = 1; f <= 6; f++) {
        if (counts[f] >= 3) three = 1;
        if (counts[f] >= 4) four = 1;
        if (counts[f] >= 5) five = 1;
        if (counts[f] == 2) pair = 1;
        if (counts[f] == 3) triple = 1;
    }
    if (cat < 6) return count_face(cat + 1) * (cat + 1);
    switch (cat) {
        case 6:  return three ? sum_best(5) : 0;
        case 7:  return four  ? sum_best(5) : 0;
        case 8:  return (pair && triple) ? 25 : 0;
        case 9:
            for (f = 1; f <= 3; f++)
                if (counts[f] && counts[f+1] && counts[f+2] && counts[f+3]) return 30;
            return 0;
        case 10:
            for (f = 1; f <= 2; f++)
                if (counts[f] && counts[f+1] && counts[f+2] && counts[f+3] && counts[f+4]) return 40;
            return 0;
        case 11: return five ? 50 : 0;
        default: return sum_best(5);
    }
}

static void roll(void)
{
    int i;
    for (i = 0; i < ND; i++) if (!keep[i]) dice[i] = rnd_range(1, 6);
}

static int col_total(int c, int *upper_out)
{
    int i, upper = 0, total = 0;
    for (i = 0; i < CATS; i++) {
        if (!used[c][i]) continue;
        total += sc[c][i];
        if (i < 6) upper += sc[c][i];
    }
    if (upper >= 63) total += 35;
    if (upper_out) *upper_out = upper;
    return total * (NCOL > 1 ? c + 1 : 1);     /* triple play multiplies columns */
}

static int grand_total(void)
{
    int c, t = 0;
    for (c = 0; c < NCOL; c++) t += col_total(c, NULL);
    return t;
}

static void render(int col, int cur, int rolls, int selecting, const char *msg)
{
    int i, c;
    char sub[120];
    snprintf(sub, sizeof sub, "%d dice, %d roll%s/turn%s%s — Space holds, R rerolls, Tab switches, Enter scores",
             ND, NROLL, NROLL == 1 ? "" : "s",
             NCOL > 1 ? ", 3 columns (x1 x2 x3)" : "",
             DUP ? ", shared dice" : "");
    draw_title("YAHTZEE", sub);

    for (i = 0; i < ND; i++) {
        draw_textf(5, 14 + i * 8, "%s%s +---+ %s", keep[i] ? BG_GREEN : "", C_WHITE, C_RESET);
        draw_textf(6, 14 + i * 8, "%s%s | %d | %s", keep[i] ? BG_GREEN : "", C_WHITE, dice[i], C_RESET);
        draw_textf(7, 14 + i * 8, "%s%s +---+ %s", keep[i] ? BG_GREEN : "", C_WHITE, C_RESET);
        draw_textf(8, 14 + i * 8, "%s%s%s", (!selecting && i == cur) ? C_REV : "",
                   (!selecting && i == cur) ? "  ^^^  " : "       ", C_RESET);
        draw_textf(9, 14 + i * 8, "%s%s%s", C_GREY, keep[i] ? " HELD  " : "       ", C_RESET);
    }
    draw_textf(10, 14, "Rolls left: %d    ", rolls);

    for (i = 0; i < CATS; i++) {
        draw_textf(12 + i, 14, "%s%s%-17s%s",
                   (selecting && i == cur) ? C_REV : "",
                   used[col][i] ? C_GREY : C_WHITE, CAT_NAME[i], C_RESET);
        for (c = 0; c < NCOL; c++) {
            int shown = used[c][i] ? sc[c][i] : (c == col ? score_for(i) : 0);
            draw_textf(12 + i, 34 + c * 6, "%s%3d%s",
                       used[c][i] ? C_GREY : (c == col ? C_GREEN : C_GREY), shown, C_RESET);
        }
        if (DUP)
            draw_textf(12 + i, 34 + NCOL * 6 + 4, "%s%3d%s",
                       C_MAGENTA, cpu_used[i] ? cpu_sc[i] : 0, C_RESET);
    }
    draw_textf(12 + CATS + 1, 14, "Total: %-6d %s", grand_total(),
               DUP ? "  CPU: " : "");
    if (DUP) {
        int i2, ct = 0, cu = 0;
        for (i2 = 0; i2 < CATS; i2++) if (cpu_used[i2]) { ct += cpu_sc[i2]; if (i2 < 6) cu += cpu_sc[i2]; }
        if (cu >= 63) ct += 35;
        draw_textf(12 + CATS + 1, 38, "CPU: %-6d", ct);
    }
    if (TARGET) draw_textf(12 + CATS + 2, 14, "Target: %d      ", TARGET);
    draw_text(12 + CATS + 4, 10, "                                                        ");
    if (msg) draw_textf(12 + CATS + 4, 14, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

/* The duplicate opponent plays the same dice, taking its best open box. */
static void cpu_place(void)
{
    int i, best = -1, bi = -1;
    for (i = 0; i < CATS; i++) {
        int v;
        if (cpu_used[i]) continue;
        v = score_for(i);
        if (i >= 6) v += 2;                     /* mild bias to the set boxes */
        if (v > best) { best = v; bi = i; }
    }
    if (bi >= 0) { cpu_sc[bi] = score_for(bi); cpu_used[bi] = 1; }
}

void fam_yahtzee(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    ND = (v == 2) ? 6 : 5;
    NROLL = (v == 4) ? 2 : 3;
    NCOL = (v == 1) ? 3 : 1;
    DUP = (v == 3);
    TARGET = (v == 5) ? 220 : 0;

    for (;;) {
        int turn, col, total;
        memset(used, 0, sizeof used);
        memset(sc, 0, sizeof sc);
        memset(cpu_used, 0, sizeof cpu_used);
        memset(cpu_sc, 0, sizeof cpu_sc);

        for (col = 0; col < NCOL; col++)
        for (turn = 0; turn < CATS; turn++) {
            int rolls = NROLL, cur = 0, selecting = 0, placed = 0;
            memset(keep, 0, sizeof keep);
            roll();
            rolls--;

            while (!placed) {
                int k;
                render(col, cur, rolls, selecting,
                       selecting ? "Choose a category" : "Hold dice, then reroll or score");
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                if (k == KEY_TAB) { selecting = !selecting; cur = 0; continue; }
                if (!selecting) {
                    if (k == KEY_LEFT)  cur = (cur + ND - 1) % ND;
                    if (k == KEY_RIGHT) cur = (cur + 1) % ND;
                    if (k == ' ')       keep[cur] = !keep[cur];
                    if ((k == 'r' || k == 'R') && rolls > 0) { roll(); rolls--; }
                    if (k == KEY_ENTER) { selecting = 1; cur = 0; }
                } else {
                    if (k == KEY_UP)   cur = (cur + CATS - 1) % CATS;
                    if (k == KEY_DOWN) cur = (cur + 1) % CATS;
                    if (k == KEY_LEFT || k == KEY_RIGHT) { selecting = 0; cur = 0; }
                    if (k == KEY_ENTER || k == ' ') {
                        if (used[col][cur]) continue;
                        sc[col][cur] = score_for(cur);
                        used[col][cur] = 1;
                        if (DUP) cpu_place();
                        placed = 1;
                    }
                }
            }
        }

        total = grand_total();
        {
            char m[96];
            if (DUP) {
                int i2, ct = 0, cu = 0;
                for (i2 = 0; i2 < CATS; i2++) if (cpu_used[i2]) { ct += cpu_sc[i2]; if (i2 < 6) cu += cpu_sc[i2]; }
                if (cu >= 63) ct += 35;
                snprintf(m, sizeof m, "You %d, computer %d — %s", total, ct,
                         total > ct ? "you win!" : total < ct ? "computer wins." : "a tie.");
            } else if (TARGET) {
                snprintf(m, sizeof m, "Final score %d — target was %d: %s",
                         total, TARGET, total >= TARGET ? "beaten!" : "missed.");
            } else {
                snprintf(m, sizeof m, "Final score: %d", total);
            }
            render(0, 0, 0, 1, m);
            score_report(p->title ? p->title : "yahtzee", total);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
