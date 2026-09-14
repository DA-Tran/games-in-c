/* GIC:PARAMETERISED wordle
 * wordle.c - positional-feedback word guessing at 4 to 8 letters.
 * params: count = word length (4..8)
 */
#include "engine.h"
#include "words.h"
#include "games.h"

#define TRIES 6

static int LEN;
static char guesses[TRIES][MAXLEN + 1];
static char marks[TRIES][MAXLEN + 1];
static char secret[MAXLEN + 1];

/* Two passes so repeated letters score correctly. */
static void score_guess(const char *g, char *out)
{
    int used[MAXLEN] = {0}, i, j;
    for (i = 0; i < LEN; i++) out[i] = '.';
    out[LEN] = '\0';
    for (i = 0; i < LEN; i++)
        if (g[i] == secret[i]) { out[i] = 'g'; used[i] = 1; }
    for (i = 0; i < LEN; i++) {
        if (out[i] == 'g') continue;
        for (j = 0; j < LEN; j++) {
            if (used[j] || secret[j] != g[i]) continue;
            out[i] = 'y';
            used[j] = 1;
            break;
        }
    }
}

static void render(int row, const char *typing, int letters[26], const char *msg)
{
    int r, i, left = 40 - LEN * 2;
    char sub[90];
    snprintf(sub, sizeof sub, "%d letters - type, Enter submits, Backspace deletes, Q quits", LEN);
    draw_title("WORDLE", sub);

    for (r = 0; r < TRIES; r++) {
        scr_move(6 + r * 2, left);
        for (i = 0; i < LEN; i++) {
            char ch = ' ';
            const char *bg = "";
            if (r < row) {
                ch = guesses[r][i];
                bg = marks[r][i] == 'g' ? BG_GREEN : marks[r][i] == 'y' ? BG_YELLOW : BG_BLACK;
            } else if (r == row && i < (int)strlen(typing)) {
                ch = typing[i];
            }
            printf("%s%s %c %s", bg, C_BOLD, toupper(ch), C_RESET);
        }
    }
    scr_move(6 + TRIES * 2 + 1, 24);
    for (i = 0; i < 26; i++) {
        const char *col = letters[i] == 2 ? C_GREEN : letters[i] == 1 ? C_YELLOW
                        : letters[i] == -1 ? C_GREY : C_WHITE;
        printf("%s%c%s ", col, 'A' + i, C_RESET);
    }
    draw_text(6 + TRIES * 2 + 3, 20, "                                                        ");
    if (msg) draw_textf(6 + TRIES * 2 + 3, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_wordle(const GParams *p)
{
    LEN = gp_int(p->count, 5);
    if (LEN < MINLEN) LEN = MINLEN;
    if (LEN > MAXLEN) LEN = MAXLEN;

    for (;;) {
        char typing[MAXLEN + 2] = "";
        int row = 0, won = 0, letters[26] = {0}, i;
        strcpy(secret, WORDS_BY_LEN[LEN][rnd(WORDS_BY_LEN_COUNT[LEN])]);

        while (row < TRIES && !won) {
            int k;
            render(row, typing, letters, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_BACKSPACE || k == 8) {
                int n = (int)strlen(typing);
                if (n) typing[n - 1] = '\0';
                continue;
            }
            if (isalpha(k)) {
                int n = (int)strlen(typing);
                if (n < LEN) { typing[n] = (char)tolower(k); typing[n + 1] = '\0'; }
                continue;
            }
            if (k != KEY_ENTER) continue;
            if ((int)strlen(typing) != LEN) {
                render(row, typing, letters, "Needs the full length.");
                sleep_ms(700);
                continue;
            }
            if (!word_valid(typing, LEN)) {
                render(row, typing, letters, "Not in the word list.");
                sleep_ms(800);
                continue;
            }
            strcpy(guesses[row], typing);
            score_guess(typing, marks[row]);
            for (i = 0; i < LEN; i++) {
                int idx = typing[i] - 'a';
                int v = marks[row][i] == 'g' ? 2 : marks[row][i] == 'y' ? 1 : -1;
                if (v > letters[idx]) letters[idx] = v;
            }
            if (strcmp(typing, secret) == 0) won = 1;
            row++;
            typing[0] = '\0';
        }
        {
            char m[96];
            if (won) snprintf(m, sizeof m, "Solved in %d guess%s!", row, row == 1 ? "" : "es");
            else     snprintf(m, sizeof m, "Out of guesses - the word was '%s'.", secret);
            render(row, "", letters, m);
            if (won) score_report(p->title ? p->title : "wordle", (TRIES - row + 1) * 100);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
