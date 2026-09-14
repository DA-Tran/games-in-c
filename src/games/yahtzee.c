/* yahtzee.c - full thirteen-category scorecard with three rolls per turn. */
#include "engine.h"
#include "games.h"

#define CATS 13

static const char *CAT_NAME[CATS] = {
    "Ones","Twos","Threes","Fours","Fives","Sixes",
    "Three of a Kind","Four of a Kind","Full House",
    "Small Straight","Large Straight","Yahtzee","Chance"
};

static int dice[5], keep[5], used[CATS], sc[CATS];

static int count_face(int f)
{
    int i, n = 0;
    for (i = 0; i < 5; i++) if (dice[i] == f) n++;
    return n;
}

static int sum_all(void)
{
    int i, s = 0;
    for (i = 0; i < 5; i++) s += dice[i];
    return s;
}

static int score_for(int cat)
{
    int f, counts[7] = {0}, i, three = 0, four = 0, pair = 0, triple = 0;
    for (i = 0; i < 5; i++) counts[dice[i]]++;
    for (f = 1; f <= 6; f++) {
        if (counts[f] >= 3) three = 1;
        if (counts[f] >= 4) four = 1;
        if (counts[f] == 2) pair = 1;
        if (counts[f] == 3) triple = 1;
    }
    if (cat < 6) return count_face(cat + 1) * (cat + 1);
    switch (cat) {
        case 6:  return three ? sum_all() : 0;
        case 7:  return four ? sum_all() : 0;
        case 8:  return (pair && triple) ? 25 : 0;
        case 9: {
            for (f = 1; f <= 3; f++)
                if (counts[f] && counts[f+1] && counts[f+2] && counts[f+3]) return 30;
            return 0;
        }
        case 10: {
            for (f = 1; f <= 2; f++)
                if (counts[f] && counts[f+1] && counts[f+2] && counts[f+3] && counts[f+4]) return 40;
            return 0;
        }
        case 11: {
            for (f = 1; f <= 6; f++) if (counts[f] == 5) return 50;
            return 0;
        }
        default: return sum_all();
    }
}

static void roll(void)
{
    int i;
    for (i = 0; i < 5; i++) if (!keep[i]) dice[i] = rnd_range(1, 6);
}

static void render(int cur, int rolls, int selecting, const char *msg)
{
    int i, upper = 0, total = 0;
    char sub[80];
    snprintf(sub, sizeof sub, "Rolls left: %d - Space toggles a die, R rerolls, Enter scores, Q quits", rolls);
    draw_title("YAHTZEE", sub);

    for (i = 0; i < 5; i++) {
        draw_textf(6, 18 + i * 8, "%s%s ┌───┐ %s", keep[i] ? BG_GREEN : "", C_WHITE, C_RESET);
        draw_textf(7, 18 + i * 8, "%s%s │ %d │ %s", keep[i] ? BG_GREEN : "", C_WHITE, dice[i], C_RESET);
        draw_textf(8, 18 + i * 8, "%s%s └───┘ %s", keep[i] ? BG_GREEN : "", C_WHITE, C_RESET);
        draw_textf(9, 18 + i * 8, "%s%s%s", (!selecting && i == cur) ? C_REV : "",
                   (!selecting && i == cur) ? "  ^^^  " : "       ", C_RESET);
        draw_textf(10, 18 + i * 8, "%s%s%s", C_GREY, keep[i] ? " HELD  " : "       ", C_RESET);
    }

    for (i = 0; i < CATS; i++) {
        int shown = used[i] ? sc[i] : score_for(i);
        draw_textf(12 + i, 18, "%s%s%-17s %s%3d%s",
                   (selecting && i == cur) ? C_REV : "",
                   used[i] ? C_GREY : C_WHITE, CAT_NAME[i],
                   used[i] ? C_GREY : C_GREEN, shown, C_RESET);
        if (used[i]) { total += sc[i]; if (i < 6) upper += sc[i]; }
    }
    if (upper >= 63) total += 35;
    draw_textf(12 + CATS + 1, 18, "Total: %-5d %s", total, upper >= 63 ? "(+35 bonus)" : "");
    draw_text(12 + CATS + 3, 14, "                                                        ");
    if (msg) draw_textf(12 + CATS + 3, 18, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_yahtzee(const GParams *p)
{
    (void)p;
    for (;;) {
        int turn, i, total = 0, upper = 0;
        memset(used, 0, sizeof used);
        memset(sc, 0, sizeof sc);

        for (turn = 0; turn < CATS; turn++) {
            int rolls = 3, cur = 0, selecting = 0, placed = 0;
            memset(keep, 0, sizeof keep);
            roll();
            rolls--;

            while (!placed) {
                int k;
                render(cur, rolls, selecting, selecting ? "Choose a category" : "Hold dice, then reroll or score");
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                if (k == KEY_TAB) { selecting = !selecting; cur = 0; continue; }
                if (!selecting) {
                    if (k == KEY_LEFT)  cur = (cur + 4) % 5;
                    if (k == KEY_RIGHT) cur = (cur + 1) % 5;
                    if (k == ' ')       keep[cur] = !keep[cur];
                    if ((k == 'r' || k == 'R') && rolls > 0) { roll(); rolls--; }
                    if (k == KEY_ENTER) { selecting = 1; cur = 0; }
                } else {
                    if (k == KEY_UP)   cur = (cur + CATS - 1) % CATS;
                    if (k == KEY_DOWN) cur = (cur + 1) % CATS;
                    if (k == KEY_LEFT || k == KEY_RIGHT) { selecting = 0; cur = 0; }
                    if (k == KEY_ENTER || k == ' ') {
                        if (used[cur]) continue;
                        sc[cur] = score_for(cur);
                        used[cur] = 1;
                        placed = 1;
                    }
                }
            }
        }
        for (i = 0; i < CATS; i++) { total += sc[i]; if (i < 6) upper += sc[i]; }
        if (upper >= 63) total += 35;
        {
            char m[64];
            snprintf(m, sizeof m, "Final score: %d", total);
            render(0, 0, 1, m);
            score_report("yahtzee", total);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
