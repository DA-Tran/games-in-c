/* GIC:PARAMETERISED quiz
 * quiz.c - the multiple-choice quiz family.
 *
 * params: theme (or variant) selects the topic, difficulty selects the band.
 *
 * The bands are separate question pools, not a shorter timer over the same
 * questions: band 0 is common knowledge and band 3 is specialist. Sixteen
 * topics times four bands is what makes sixty-three distinct entries.
 */
#include "engine.h"
#include "games.h"
#include "quiz.h"

#define ROUND 6

/* The bank is split across two files purely for readability. */
static const QItem *bank_at(int i)
{
    if (i < QUIZ_BANK_COUNT) return &QUIZ_BANK[i];
    return &QUIZ_BANK2[i - QUIZ_BANK_COUNT];
}
static int bank_total(void) { return QUIZ_BANK_COUNT + QUIZ_BANK2_COUNT; }

void fam_quiz(const GParams *p)
{
    int topic = -1, level, i, total = bank_total();
    int pool[64], pn = 0;

    for (i = 0; i < QUIZ_TOPICS; i++)
        if (p->theme && strcmp(p->theme, QUIZ_TOPIC[i]) == 0) topic = i;
    if (topic < 0) topic = gp_int(p->variant, 0) % QUIZ_TOPICS;

    /* difficulty 1..4 in the catalogue maps to bands 0..3 */
    level = gp_int(p->difficulty, 1) - 1;
    if (level < 0) level = 0;
    if (level >= QUIZ_LEVELS) level = QUIZ_LEVELS - 1;

    for (i = 0; i < total && pn < 64; i++) {
        const QItem *it = bank_at(i);
        if (it->topic == topic && it->level == level) pool[pn++] = i;
    }
    if (pn == 0) {                      /* fall back to the topic's easy band */
        for (i = 0; i < total && pn < 64; i++)
            if (bank_at(i)->topic == topic) pool[pn++] = i;
    }

    for (;;) {
        int order[64], n = pn, asked = 0, right = 0;
        static const char *BAND[4] = {"easy", "medium", "hard", "expert"};
        char sub[110];

        for (i = 0; i < n; i++) order[i] = pool[i];
        shuffle_int(order, n);
        snprintf(sub, sizeof sub, "%s, %s — press 1-4 to answer, Q quits",
                 QUIZ_TOPIC[topic], BAND[level]);

        for (i = 0; i < n && asked < ROUND; i++) {
            const QItem *it = bank_at(order[i]);
            int shown[4] = {0,1,2,3}, k, pick = -1, j;
            shuffle_int(shown, 4);

            for (;;) {
                draw_title("QUIZ", sub);
                draw_textf(6, 10, "Question %d of %d", asked + 1, ROUND);
                draw_textf(8, 10, "%s%-66s%s", C_BOLD, it->q, C_RESET);
                for (j = 0; j < 4; j++)
                    draw_textf(11 + j * 2, 12, "%s%d) %-56s%s",
                               pick < 0 ? C_WHITE
                               : (shown[j] == it->correct ? C_GREEN
                               : (j == pick ? C_RED : C_GREY)),
                               j + 1, it->a[shown[j]], C_RESET);
                draw_textf(21, 10, "Score %d of %d      ", right, asked);
                scr_flush();
                if (pick >= 0) { key_get(); break; }
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                if (k < '1' || k > '4') continue;
                pick = k - '1';
                asked++;
                if (shown[pick] == it->correct) right++;
            }
        }
        {
            char m[80];
            snprintf(m, sizeof m, "You scored %d out of %d.", right, asked);
            scr_clear();
            draw_title("QUIZ", sub);
            draw_centered(12, 80, m);
            scr_flush();
            score_report(p->title ? p->title : "quiz", right * 100 / (asked ? asked : 1));
        }
        if (!confirm("\n  Play again?")) return;
    }
}
