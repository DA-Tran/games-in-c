/* GIC:PARAMETERISED anagram
 * anagram.c - timed unscrambling from a themed dictionary.
 * params: theme = dictionary name, or variant = its index if theme is unset
 */
#include "engine.h"
#include "words.h"
#include "games.h"

void fam_anagram(const GParams *p)
{
    int theme = theme_index(p->theme);
    if (theme < 0) theme = (p->variant >= 0 && p->variant < THEME_COUNT) ? p->variant : 0;

    for (;;) {
        int score = 0, round;

        for (round = 1; round <= 8; round++) {
            const char *word = theme_pick(theme);
            char scram[64], answer[64];
            int len = (int)strlen(word), i, hint = 0, solved = 0;
            long start;

            strcpy(scram, word);
            for (i = 0; i < 60; i++) {
                int a = rnd(len), b = rnd(len);
                char t = scram[a]; scram[a] = scram[b]; scram[b] = t;
            }
            if (strcmp(scram, word) == 0 && len > 1) {
                char t = scram[0]; scram[0] = scram[len - 1]; scram[len - 1] = t;
            }
            start = now_ms();

            while (!solved) {
                char sub[80];
                snprintf(sub, sizeof sub, "Round %d of 8 - theme %s", round, THEME_NAME[theme]);
                draw_title("ANAGRAM", sub);
                draw_textf(9, 28, "Scrambled: %s%-24s%s", C_BOLD C_YELLOW, scram, C_RESET);
                if (hint) draw_textf(11, 28, "Hint: starts with '%c', %d letters   ", word[0], len);
                draw_textf(13, 28, "Score: %-5d   (type h for a hint, q to quit)", score);
                draw_text(15, 28, "Your answer: ");
                scr_move(15, 41);
                printf("\033[K");
                scr_flush();

                {
                    int got = read_line(answer, sizeof answer);
                    if (got < 0) return;          /* Escape, or end of input */
                    if (got == 0) continue;       /* empty line: ask again   */
                }
                for (i = 0; answer[i]; i++) answer[i] = (char)tolower(answer[i]);
                if (strcmp(answer, "q") == 0) return;
                if (strcmp(answer, "h") == 0) { hint = 1; continue; }
                if (strcmp(answer, word) == 0) {
                    int secs = (int)((now_ms() - start) / 1000);
                    int pts = 100 - secs * 2 - (hint ? 40 : 0);
                    if (pts < 10) pts = 10;
                    score += pts;
                    draw_textf(17, 28, "%sCorrect! +%d points%s          ", C_GREEN, pts, C_RESET);
                    scr_flush();
                    sleep_ms(900);
                    solved = 1;
                } else {
                    draw_textf(17, 28, "%sNot quite - try again.%s      ", C_RED, C_RESET);
                    scr_flush();
                    sleep_ms(700);
                    draw_text(17, 28, "                              ");
                }
            }
        }
        scr_clear();
        draw_title("ANAGRAM", "Round complete");
        draw_textf(11, 34, "Final score: %s%d%s", C_BOLD C_YELLOW, score, C_RESET);
        score_report(p->title ? p->title : "anagram", score);
        if (!confirm("\n  Play again?")) return;
    }
}
