/* GIC:PARAMETERISED typing
 * GIC:PARAMETERISED guessnumber
 * GIC:PARAMETERISED bullscows
 *
 * misc.c - typing test, guess the number, and bulls and cows.
 *
 * Rock paper scissors, Simon, the race games and the piano moved to misc_b.c
 * when they were parameterised; this file kept the three word/number games.
 *
 * Markers above name the families in this file that honour their parameters;
 * the others still run at a single default configuration.
 *
 * typing:      variant = drill mode (see TYPING_MODE_NAME)
 * guessnumber: count   = upper bound of the range
 * bullscows:   count   = number of digits
 */
#include "engine.h"
#include "words.h"
#include "games.h"

/* ------------------------------------------------------------ typing test */

void fam_typing(const GParams *p)
{
    int mode = (p->variant >= 0 && p->variant < TYPING_MODES) ? p->variant : 0;

    for (;;) {
        const char *text = TYPING_TEXT[mode][rnd(TYPING_TEXT_COUNT[mode])];
        int len = (int)strlen(text), pos = 0, errors = 0;
        long start;
        char sub[90];

        snprintf(sub, sizeof sub, "%s - type it exactly, the clock starts on your first key",
                 TYPING_MODE_NAME[mode]);
        scr_clear();
        draw_title("TYPING TEST", sub);
        draw_textf(9, 6, "%s%.70s%s", C_GREY, text, C_RESET);
        if (len > 70) draw_textf(10, 6, "%s%s%s", C_GREY, text + 70, C_RESET);
        draw_text(13, 6, "Press any key to begin, Q to quit");
        scr_flush();
        {
            int k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
        }
        start = now_ms();

        while (pos < len) {
            int k, i, row = 9, col = 6;
            scr_move(9, 6);
            for (i = 0; i < len; i++) {
                if (i && i % 70 == 0) { row++; col = 6; scr_move(row, col); }
                if (i < pos)       printf("%s%c%s", C_GREEN, text[i], C_RESET);
                else if (i == pos) printf("%s%c%s", C_REV, text[i], C_RESET);
                else               printf("%s%c%s", C_GREY, text[i], C_RESET);
            }
            {
                long el = (now_ms() - start) / 1000;
                int wpm = el > 0 ? (int)((pos / 5.0) / (el / 60.0)) : 0;
                draw_textf(13, 6, "Time %3lds   WPM %3d   Errors %d    ", el, wpm, errors);
            }
            scr_flush();

            k = key_get();
            if (k == KEY_ESC) return;
            if (k == KEY_BACKSPACE || k == 8) { if (pos) pos--; continue; }
            if (k == KEY_ENTER) k = ' ';
            if (k < 32 || k > 126) continue;
            if ((char)k == text[pos]) pos++;
            else errors++;
        }
        {
            long el = (now_ms() - start) / 1000;
            int wpm = el > 0 ? (int)((len / 5.0) / (el / 60.0)) : 0;
            int acc = (len + errors) > 0 ? (len * 100) / (len + errors) : 100;
            char m[96];
            snprintf(m, sizeof m, "%d WPM, %d%% accuracy, %ld seconds", wpm, acc, el);
            draw_centered(16, 80, m);
            scr_flush();
            score_report(p->title ? p->title : "typing", wpm);
        }
        if (!confirm("\n  Play again?")) return;
    }
}

/* --------------------------------------------------------- guess the number */

void fam_guess_number(const GParams *p)
{
    int hi = gp_int(p->count, 100);
    if (hi < 2) hi = 2;

    for (;;) {
        int secret = rnd_range(1, hi), tries = 0, lo_b = 1, hi_b = hi;
        int optimal = 1;
        {
            int span = hi;
            while (span > 1) { span /= 2; optimal++; }
        }

        for (;;) {
            char buf[32];
            int g;
            char sub[80];
            snprintf(sub, sizeof sub, "I am thinking of a number from 1 to %d", hi);
            draw_title("GUESS THE NUMBER", sub);
            draw_textf(9,  28, "Known range: %d to %d           ", lo_b, hi_b);
            draw_textf(10, 28, "Guesses so far: %-4d            ", tries);
            draw_textf(12, 28, "Binary search needs about %d.   ", optimal);
            draw_text(14, 28, "Your guess: ");
            scr_move(14, 40);
            printf("\033[K");
            scr_flush();

            if (read_line(buf, sizeof buf) == 0) return;
            if (buf[0] == 'q' || buf[0] == 'Q') return;
            g = atoi(buf);
            if (g < 1 || g > hi) continue;
            tries++;
            if (g == secret) break;
            if (g < secret) {
                if (g + 1 > lo_b) lo_b = g + 1;
                draw_textf(16, 28, "%sHigher!%s     ", C_YELLOW, C_RESET);
            } else {
                if (g - 1 < hi_b) hi_b = g - 1;
                draw_textf(16, 28, "%sLower!%s      ", C_YELLOW, C_RESET);
            }
            scr_flush();
            sleep_ms(600);
        }
        {
            char m[96];
            snprintf(m, sizeof m, "Got it - %d in %d guesses (optimal is about %d).",
                     secret, tries, optimal);
            draw_centered(18, 80, m);
            scr_flush();
            score_report(p->title ? p->title : "guess-number", optimal * 200 / tries);
        }
        if (!confirm("\n  Play again?")) return;
    }
}

/* ------------------------------------------------------------ bulls & cows */

void fam_bulls_cows(const GParams *p)
{
    int nd = gp_int(p->count, 4);
    if (nd < 2) nd = 2;
    if (nd > 8) nd = 8;

    for (;;) {
        int digits[8], i, j, tries = 0, won = 0;
        int used[10] = {0};
        int allowed = 10 - nd + 2;          /* generous but finite guess budget */
        char hist[16][80];
        int hn = 0;

        for (i = 0; i < nd; i++) {
            int d;
            do { d = rnd(10); } while (used[d] || (i == 0 && d == 0));
            used[d] = 1;
            digits[i] = d;
        }

        while (tries < allowed && !won) {
            char buf[32];
            int g[8], bulls = 0, cows = 0, ok = 1;
            char sub[90];
            snprintf(sub, sizeof sub, "Guess the %d-digit number - every digit is different", nd);
            draw_title("BULLS AND COWS", sub);
            for (i = 0; i < hn; i++) draw_textf(7 + i, 28, "%s%s%s", C_GREY, hist[i], C_RESET);
            draw_textf(7 + allowed + 1, 28, "Guess %d of %d: ", tries + 1, allowed);
            scr_move(7 + allowed + 1, 44);
            printf("\033[K");
            scr_flush();

            if (read_line(buf, sizeof buf) == 0) return;
            if (buf[0] == 'q' || buf[0] == 'Q') return;
            if ((int)strlen(buf) != nd) continue;
            for (i = 0; i < nd; i++) {
                if (buf[i] < '0' || buf[i] > '9') ok = 0;
                g[i] = buf[i] - '0';
            }
            for (i = 0; i < nd && ok; i++)
                for (j = i + 1; j < nd; j++)
                    if (g[i] == g[j]) ok = 0;
            if (!ok) {
                draw_textf(9 + allowed + 1, 28, "%s%d different digits, please.%s", C_RED, nd, C_RESET);
                scr_flush();
                sleep_ms(900);
                continue;
            }
            for (i = 0; i < nd; i++) {
                if (g[i] == digits[i]) bulls++;
                else for (j = 0; j < nd; j++) if (g[i] == digits[j]) cows++;
            }
            tries++;
            if (hn < 16)
                snprintf(hist[hn++], sizeof hist[0], "%-8s  %d bulls, %d cows", buf, bulls, cows);
            if (bulls == nd) won = 1;
        }
        {
            char m[96];
            if (won) snprintf(m, sizeof m, "Cracked it in %d guesses.", tries);
            else {
                char sec[16];
                for (i = 0; i < nd; i++) sec[i] = (char)('0' + digits[i]);
                sec[nd] = '\0';
                snprintf(m, sizeof m, "Out of guesses - it was %s.", sec);
            }
            draw_centered(9 + allowed + 3, 80, m);
            scr_flush();
            if (won) score_report(p->title ? p->title : "bulls-cows", (allowed - tries + 1) * 100);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
