/* GIC:PARAMETERISED typing
 * GIC:PARAMETERISED guessnumber
 * GIC:PARAMETERISED bullscows
 *
 * misc.c - typing test, guess the number, bulls and cows, rock paper scissors,
 * Simon, snakes and ladders, and the virtual piano.
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

/* -------------------------------------------------- rock paper scissors */

void fam_rps(const GParams *p)
{
    (void)p;
    static const char *NAME[3] = {"Rock", "Paper", "Scissors"};
    static const char *ART[3]  = {"✊", "✋", "✌"};
    int freq[3] = {0, 0, 0};
    int wins = 0, losses = 0, draws = 0;

    for (;;) {
        int k, me, ai, result;
        draw_title("ROCK PAPER SCISSORS", "R rock, P paper, S scissors, Q quits");
        draw_textf(9,  28, "Wins %-3d  Losses %-3d  Draws %-3d", wins, losses, draws);
        draw_textf(11, 28, "%sThe computer is tracking your habits.%s", C_GREY, C_RESET);
        scr_flush();

        k = tolower(key_get());
        if (k == 'q' || k == KEY_ESC) break;
        if (k == 'r') me = 0;
        else if (k == 'p') me = 1;
        else if (k == 's') me = 2;
        else continue;

        /* Counter the player's most frequent throw. */
        {
            int most = 0, i;
            for (i = 1; i < 3; i++) if (freq[i] > freq[most]) most = i;
            ai = (freq[most] > 0 && rnd(100) < 65) ? (most + 1) % 3 : rnd(3);
        }
        freq[me]++;

        result = (me - ai + 3) % 3;      /* 0 draw, 1 player wins, 2 ai wins */
        if (result == 1) wins++;
        else if (result == 2) losses++;
        else draws++;

        draw_textf(14, 28, "You: %s %-10s   CPU: %s %-10s", ART[me], NAME[me], ART[ai], NAME[ai]);
        draw_textf(16, 28, "%s%-20s%s", C_BOLD,
                   result == 1 ? "You win the round!" : result == 2 ? "Computer wins." : "Draw.",
                   C_RESET);
        scr_flush();
        sleep_ms(1100);
    }
    score_report("rock-paper-scissors", wins);
    pause_msg("Press any key...");
}

/* ------------------------------------------------------------------ simon */

void fam_simon(const GParams *p)
{
    (void)p;
    static const char *CNAME[4] = {"RED", "GREEN", "BLUE", "YELLOW"};
    static const char *CCOL[4]  = {C_RED, C_GREEN, C_BLUE, C_YELLOW};
    static const char KEYS[4]   = {'r', 'g', 'b', 'y'};

    for (;;) {
        int seq[64], len = 0, alive = 1;

        while (alive && len < 64) {
            int i;
            seq[len++] = rnd(4);

            draw_title("SIMON", "Watch the sequence, then repeat it with R G B Y");
            draw_textf(9, 30, "Round %d", len);
            scr_flush();
            sleep_ms(700);

            for (i = 0; i < len; i++) {
                draw_textf(12, 30, "%s%s  ██████  %s", C_BOLD, CCOL[seq[i]], C_RESET);
                draw_textf(14, 30, "%s%-10s%s", CCOL[seq[i]], CNAME[seq[i]], C_RESET);
                beep(400 + seq[i] * 120, 200);
                scr_flush();
                sleep_ms(480);
                draw_text(12, 30, "            ");
                draw_text(14, 30, "          ");
                scr_flush();
                sleep_ms(180);
            }

            draw_textf(12, 30, "%sYour turn - %d step%s%s", C_CYAN, len, len == 1 ? "" : "s", C_RESET);
            scr_flush();

            for (i = 0; i < len; i++) {
                int k = tolower(key_get()), j, pick = -1;
                if (k == 'q' || k == KEY_ESC) return;
                for (j = 0; j < 4; j++) if (k == KEYS[j]) pick = j;
                if (pick < 0) { i--; continue; }
                draw_textf(14, 30, "%s%-10s%s", CCOL[pick], CNAME[pick], C_RESET);
                scr_flush();
                if (pick != seq[i]) { alive = 0; break; }
                sleep_ms(140);
            }
            if (!alive) break;
            draw_textf(16, 30, "%sCorrect!%s      ", C_GREEN, C_RESET);
            scr_flush();
            sleep_ms(600);
            draw_text(16, 30, "              ");
        }
        {
            char m[64];
            snprintf(m, sizeof m, "Wrong - you reached round %d.", len);
            draw_centered(18, 80, m);
            scr_flush();
            score_report("simon", len - 1);
        }
        if (!confirm("\n  Play again?")) return;
    }
}

/* -------------------------------------------------------- snakes & ladders */

void fam_snakes_ladders(const GParams *p)
{
    (void)p;
    /* from -> to */
    static const int JUMP_FROM[] = {1,4,9,21,28,36,51,71,80,16,47,49,56,62,64,87,93,95,98};
    static const int JUMP_TO[]   = {38,14,31,42,84,44,67,91,100,6,26,11,53,19,60,24,73,75,78};
    #define NJUMP ((int)(sizeof(JUMP_FROM)/sizeof(JUMP_FROM[0])))

    for (;;) {
        int pos[2] = {0, 0}, turn = 0, over = 0;

        while (!over) {
            int k, die, i, from;
            char msg[96] = "";

            draw_title("SNAKES AND LADDERS", "Enter rolls the die, Q quits");
            /* Board: 10x10 boustrophedon. */
            for (i = 0; i < 10; i++) {
                int c;
                scr_move(6 + i, 22);
                for (c = 0; c < 10; c++) {
                    int row = 9 - i;
                    int sq = row * 10 + ((row % 2 == 0) ? c + 1 : 10 - c);
                    const char *col = C_GREY;
                    int j;
                    for (j = 0; j < NJUMP; j++) {
                        if (JUMP_FROM[j] != sq) continue;
                        col = JUMP_TO[j] > sq ? C_GREEN : C_RED;
                    }
                    if (pos[0] == sq && pos[1] == sq) printf("%s[**]%s", C_BOLD, C_RESET);
                    else if (pos[0] == sq)            printf("%s[Y ]%s", C_CYAN, C_RESET);
                    else if (pos[1] == sq)            printf("%s[ C]%s", C_MAGENTA, C_RESET);
                    else                              printf("%s%3d %s", col, sq, C_RESET);
                }
            }
            draw_textf(17, 22, "%sYou: %-3d%s   %sCPU: %-3d%s   %s",
                       C_CYAN, pos[0], C_RESET, C_MAGENTA, pos[1], C_RESET,
                       turn == 0 ? "Your turn " : "CPU turn  ");
            draw_textf(19, 22, "%sGreen = ladder, red = snake%s", C_GREY, C_RESET);
            scr_flush();

            if (turn == 0) {
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                if (k != KEY_ENTER && k != ' ') continue;
            } else {
                sleep_ms(900);
            }

            die = rnd_range(1, 6);
            from = pos[turn];
            if (from + die <= 100) pos[turn] = from + die;
            snprintf(msg, sizeof msg, "%s rolled %d: %d -> %d",
                     turn == 0 ? "You" : "Computer", die, from, pos[turn]);

            for (i = 0; i < NJUMP; i++) {
                if (JUMP_FROM[i] != pos[turn]) continue;
                snprintf(msg, sizeof msg, "%s rolled %d: %d -> %d, then %s to %d!",
                         turn == 0 ? "You" : "Computer", die, from, pos[turn],
                         JUMP_TO[i] > pos[turn] ? "climbs" : "slides", JUMP_TO[i]);
                pos[turn] = JUMP_TO[i];
                break;
            }
            draw_textf(21, 22, "%s%-70s%s", C_BOLD, msg, C_RESET);
            scr_flush();
            sleep_ms(1300);

            if (pos[turn] >= 100) {
                draw_centered(23, 80, turn == 0 ? C_BOLD C_GREEN "You reach 100 - you win!" C_RESET
                                                : C_BOLD C_RED "Computer reaches 100." C_RESET);
                scr_flush();
                over = 1;
            }
            turn = 1 - turn;
        }
        if (!confirm("\n  Play again?")) return;
    }
}

/* ---------------------------------------------------------- virtual piano */

void fam_piano(const GParams *p)
{
    (void)p;
    /* Two octaves mapped across the home and number rows. */
    static const char  KEYS[]  = "zsxdcvgbhnjm,l.;/q2w3er5t6y7ui9o0p";
    static const char *NAMES[] = {"C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4",
                                  "A#4","B4","C5","C#5","D5","D#5","E5","F5","F#5","G5",
                                  "G#5","A5","A#5","B5","C6","C#6","D6","D#6","E6","F6",
                                  "F#6","G6","G#6","A6"};
    static const int FREQ[] = {262,277,294,311,330,349,370,392,415,440,466,494,
                               523,554,587,622,659,698,740,784,831,880,932,988,
                               1047,1109,1175,1245,1319,1397,1480,1568,1661,1760};
    const int NKEYS = (int)(sizeof(KEYS) - 1);
    char history[64] = "";

    for (;;) {
        int k, i, idx = -1;
        draw_title("VIRTUAL PIANO", "Play with z s x d c v g b h n j m  /  q 2 w 3 e r 5 t 6 y 7 u");
        draw_text(8, 12, "  ┌─┬┬─┬┬─┬─┬┬─┬┬─┬┬─┬─┐  ┌─┬┬─┬┬─┬─┬┬─┬┬─┬┬─┬─┐");
        draw_text(9, 12, "  │ ││ ││ │ ││ ││ ││ │ │  │ ││ ││ │ ││ ││ ││ │ │");
        draw_text(10,12, "  │ └┘ └┘ │ └┘ └┘ └┘ │ │  │ └┘ └┘ │ └┘ └┘ └┘ │ │");
        draw_text(11,12, "  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │");
        draw_text(12,12, "  └──┴──┴──┴──┴──┴──┴──┘  └──┴──┴──┴──┴──┴──┴──┘");
        draw_text(13,12, "   z  x  c  v  b  n  m     q  w  e  r  t  y  u");
        draw_textf(16, 12, "%sRecent: %-48s%s", C_GREY, history, C_RESET);
        draw_text(18, 12, "Press Q to quit");
        scr_flush();

        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
        for (i = 0; i < NKEYS; i++) if (KEYS[i] == tolower(k)) idx = i;
        if (idx < 0) continue;

        beep(FREQ[idx], 220);
        draw_textf(20, 12, "%s%s  %-5s  %4d Hz%s   ", C_BOLD C_YELLOW, "♪", NAMES[idx], FREQ[idx], C_RESET);
        scr_flush();
        {
            char add[8];
            snprintf(add, sizeof add, "%s ", NAMES[idx]);
            if (strlen(history) + strlen(add) >= sizeof history) history[0] = '\0';
            strncat(history, add, sizeof history - strlen(history) - 1);
        }
        sleep_ms(90);
    }
}
