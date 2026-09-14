/* GIC:PARAMETERISED hangman
 * hangman.c - letter deduction over any of the themed dictionaries.
 * params: theme = dictionary name, or variant = its index if theme is unset
 */
#include "engine.h"
#include "words.h"
#include "games.h"

#define LIVES 6

static const char *GALLOWS[LIVES + 1][6] = {
{"  +---+","  |   |","      |","      |","      |","========="},
{"  +---+","  |   |","  O   |","      |","      |","========="},
{"  +---+","  |   |","  O   |","  |   |","      |","========="},
{"  +---+","  |   |","  O   |"," /|   |","      |","========="},
{"  +---+","  |   |","  O   |"," /|\\  |","      |","========="},
{"  +---+","  |   |","  O   |"," /|\\  |"," /    |","========="},
{"  +---+","  |   |","  O   |"," /|\\  |"," / \\  |","========="}
};

void fam_hangman(const GParams *p)
{
    int theme = theme_index(p->theme);
    if (theme < 0) theme = (p->variant >= 0 && p->variant < THEME_COUNT) ? p->variant : 0;

    for (;;) {
        const char *word = theme_pick(theme);
        int wrong = 0, guessed[26] = {0}, i, won = 0;

        while (wrong < LIVES && !won) {
            int k, len = (int)strlen(word), shown = 0;
            char sub[80];
            snprintf(sub, sizeof sub, "Theme: %s - guess a letter, Q quits",
                     THEME_NAME[theme]);
            draw_title("HANGMAN", sub);

            for (i = 0; i < 6; i++)
                draw_textf(7 + i, 30, "%s%-12s%s", C_YELLOW, GALLOWS[wrong][i], C_RESET);

            scr_move(15, 30);
            for (i = 0; i < len; i++) {
                if (guessed[word[i] - 'a']) { printf("%s%c %s", C_GREEN, word[i], C_RESET); shown++; }
                else printf("%s_ %s", C_WHITE, C_RESET);
            }
            if (shown == len) won = 1;

            scr_move(17, 30);
            printf("%sWrong: %s", C_RED, C_RESET);
            for (i = 0; i < 26; i++)
                if (guessed[i] && !strchr(word, 'a' + i)) printf("%c ", 'a' + i);
            printf("          ");
            draw_textf(19, 30, "Lives: %d   ", LIVES - wrong);
            scr_flush();
            if (won) break;

            k = tolower(key_get());
            if (k == 'q' || k == KEY_ESC) return;
            if (k < 'a' || k > 'z') continue;
            if (guessed[k - 'a']) continue;
            guessed[k - 'a'] = 1;
            if (!strchr(word, k)) wrong++;
        }
        {
            char m[96];
            if (won) snprintf(m, sizeof m, "You got it: %s", word);
            else     snprintf(m, sizeof m, "Out of lives - the word was '%s'.", word);
            draw_centered(21, 80, m);
            scr_flush();
            if (won) score_report(p->title ? p->title : "hangman", (LIVES - wrong) * 100);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
