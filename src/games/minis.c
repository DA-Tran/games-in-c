/* GIC:PARAMETERISED reaction
 * GIC:PARAMETERISED wordmisc
 * GIC:PARAMETERISED idle
 *
 * minis.c - three families of short-session games.
 *
 * Markers above name the families in this file that honour their parameters.
 */
#include "engine.h"
#include "games.h"
#include "words.h"

/* ================================================================ reaction
 * params: variant = which test
 *
 * Eighteen tests of different faculties: raw latency, working memory, span,
 * interference, aim, and tracking. They are not one timer with new labels —
 * n-back and digit span measure memory, Stroop measures interference, the aim
 * drills measure pointing.
 */
enum {
    R_TIMER, R_NBACK, R_DIGITSPAN, R_PATTERN, R_STROOP, R_AIM, R_CHIMP,
    R_VISUALSPAN, R_SEQREPEAT, R_CARDSPRINT, R_AUDIOMEM, R_RHYTHM,
    R_PERIPHERAL, R_FLICK, R_MULTI, R_TRACKING, R_SOUNDCUE, R_COLOURCHANGE,
    R_COUNT
};

static const char *RNAME[R_COUNT] = {
    "Reaction Timer", "N-Back", "Digit Span", "Pattern Recall",
    "Colour Match (Stroop)", "Aim Trainer", "Chimp Test", "Visual Span",
    "Sequence Repeat", "Card Memory Sprint", "Audio Memory", "Rhythm Tap",
    "Peripheral Vision", "Flick Shot", "Multi-Target", "Tracking Drill",
    "Sound Cue", "Colour Change"
};

/* Tests that boil down to "press the moment the cue appears". */
static void reaction_cue(const GParams *p, int VAR)
{
    int round, best = 99999, total = 0, n = 0;
    for (round = 0; round < 5; round++) {
        long wait = 800 + rnd(2200), t0;
        int k;
        scr_clear();
        draw_title("REACTION", RNAME[VAR]);
        draw_textf(10, 24, "%sWait for the cue...%s", C_GREY, C_RESET);
        draw_textf(14, 24, "Round %d of 5", round + 1);
        scr_flush();

        t0 = now_ms();
        while (now_ms() - t0 < wait) {
            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k != KEY_NONE) {
                draw_textf(12, 24, "%sToo early!%s        ", C_RED, C_RESET);
                scr_flush();
                sleep_ms(200);
                t0 = now_ms();
                wait = 800 + rnd(2200);
            }
            sleep_ms(8);
        }

        if (VAR == R_SOUNDCUE) beep(880, 120);
        if (VAR == R_COLOURCHANGE)
            draw_textf(10, 24, "%s#### NOW ####%s   ", C_BOLD C_GREEN, C_RESET);
        else if (VAR == R_PERIPHERAL)
            draw_textf(10 + (rnd(2) ? -4 : 6), rnd(2) ? 6 : 56, "%s*%s", C_BOLD C_YELLOW, C_RESET);
        else
            draw_textf(10, 24, "%s>>> PRESS <<<%s   ", C_BOLD C_GREEN, C_RESET);
        scr_flush();

        t0 = now_ms();
        for (;;) {
            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k != KEY_NONE) break;
            if (now_ms() - t0 > 4000) break;
            sleep_ms(2);
        }
        {
            int ms = (int)(now_ms() - t0);
            if (ms < best) best = ms;
            total += ms; n++;
            draw_textf(12, 24, "%s%d ms%s            ", C_CYAN, ms, C_RESET);
            scr_flush();
            sleep_ms(700);
        }
    }
    if (n) {
        char m[80];
        snprintf(m, sizeof m, "Best %d ms, average %d ms", best, total / n);
        scr_clear();
        draw_centered(12, 80, m);
        scr_flush();
        score_report(p->title ? p->title : "reaction", best ? 100000 / best : 0);
    }
    pause_msg("Press any key...");
}

/* Tests that show a sequence and ask for it back. */
static void reaction_memory(const GParams *p, int VAR)
{
    int len = 3, best = 0;
    for (;;) {
        int seq[24], i, ok = 1;
        char typed[32];
        for (i = 0; i < len; i++)
            seq[i] = (VAR == R_AUDIOMEM) ? rnd(6) : rnd(10);

        scr_clear();
        draw_title("REACTION", RNAME[VAR]);
        draw_textf(8, 20, "Length %d - watch closely", len);
        scr_flush();
        sleep_ms(500);

        for (i = 0; i < len; i++) {
            if (VAR == R_AUDIOMEM) {
                draw_textf(11, 20, "%s tone %d %s   ", C_YELLOW, seq[i] + 1, C_RESET);
                beep(300 + seq[i] * 90, 220);
            } else {
                draw_textf(11, 20, "%s   %d   %s   ", C_BOLD C_CYAN, seq[i], C_RESET);
            }
            scr_flush();
            sleep_ms(420);
            draw_text(11, 20, "            ");
            scr_flush();
            sleep_ms(140);
        }

        draw_textf(13, 20, "%sType it back, Enter to submit%s", C_GREY, C_RESET);
        scr_flush();
        {
            int n = 0;
            typed[0] = '\0';
            for (;;) {
                int k;
                draw_textf(15, 20, "%s%-24s%s", C_CYAN, typed, C_RESET);
                scr_flush();
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) goto done;
                if (k == KEY_ENTER) break;
                if ((k == KEY_BACKSPACE || k == 127) && n > 0) { typed[--n] = '\0'; continue; }
                if (k < '0' || k > '9') continue;
                if (n < 24) { typed[n++] = (char)k; typed[n] = '\0'; }
            }
            for (i = 0; i < len; i++) {
                int want = (VAR == R_AUDIOMEM) ? seq[i] + 1 : seq[i];
                if (typed[i] != (char)('0' + want % 10)) ok = 0;
            }
        }
        if (ok) {
            best = len;
            len++;
            draw_textf(17, 20, "%sCorrect - now %d%s   ", C_GREEN, len, C_RESET);
        } else {
            draw_textf(17, 20, "%sWrong. Best span %d%s  ", C_RED, best, C_RESET);
        }
        scr_flush();
        sleep_ms(700);
        if (!ok) {
            score_report(p->title ? p->title : "reaction", best * 100);
            if (!confirm("\n  Try again?")) return;
            len = 3; best = 0;
        }
    }
done:
    score_report(p->title ? p->title : "reaction", best * 100);
}

/* Grid tests: remember lit cells, or click numbers in order. */
static void reaction_grid(const GParams *p, int VAR)
{
    int level = 3, best = 0;
    for (;;) {
        int n = level, i, found = 0, cr = 0, cc = 0;
        char lit[6][6];
        memset(lit, 0, sizeof lit);
        for (i = 0; i < n; i++) {
            int r, c;
            do { r = rnd(6); c = rnd(6); } while (lit[r][c]);
            lit[r][c] = (char)(i + 1);
        }

        scr_clear();
        draw_title("REACTION", RNAME[VAR]);
        {
            int r, c;
            for (r = 0; r < 6; r++) for (c = 0; c < 6; c++)
                draw_textf(7 + r, 30 + c * 3, "%s%c %s",
                           lit[r][c] ? C_BOLD C_YELLOW : C_GREY,
                           lit[r][c] ? (VAR == R_CHIMP ? (char)('0' + lit[r][c]) : '#') : '.',
                           C_RESET);
            draw_textf(15, 24, "Memorise %d cell%s", n, n == 1 ? "" : "s");
            scr_flush();
            sleep_ms(VAR == R_CHIMP ? 1400 : 1100);
            for (r = 0; r < 6; r++) for (c = 0; c < 6; c++)
                draw_textf(7 + r, 30 + c * 3, "%s. %s", C_GREY, C_RESET);
            draw_textf(15, 24, "Now pick them%s      ", VAR == R_CHIMP ? " in order" : "");
            scr_flush();
        }

        for (;;) {
            int k, r, c;
            for (r = 0; r < 6; r++) for (c = 0; c < 6; c++)
                draw_textf(7 + r, 30 + c * 3, "%s%s%c %s",
                           (r == cr && c == cc) ? BG_BLUE : "",
                           lit[r][c] == 0 ? C_GREY : C_GREEN,
                           lit[r][c] < 0 ? '#' : '.', C_RESET);
            scr_flush();
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "reaction", best * 100); return; }
            if (k == KEY_UP    && cr > 0) cr--;
            if (k == KEY_DOWN  && cr < 5) cr++;
            if (k == KEY_LEFT  && cc > 0) cc--;
            if (k == KEY_RIGHT && cc < 5) cc++;
            if (k != KEY_ENTER && k != ' ') continue;

            if (VAR == R_CHIMP) {
                if (lit[cr][cc] == (char)(found + 1)) { lit[cr][cc] = -1; found++; }
                else break;
            } else {
                if (lit[cr][cc] > 0) { lit[cr][cc] = -1; found++; }
                else break;
            }
            if (found >= n) break;
        }
        if (found >= n) {
            best = level;
            level++;
            draw_textf(17, 24, "%sCorrect - now %d%s   ", C_GREEN, level, C_RESET);
            scr_flush();
            sleep_ms(600);
        } else {
            draw_textf(17, 24, "%sMissed. Best %d%s    ", C_RED, best, C_RESET);
            scr_flush();
            score_report(p->title ? p->title : "reaction", best * 100);
            if (!confirm("\n  Try again?")) return;
            level = 3; best = 0;
        }
        if (level > 12) level = 12;
    }
}

/* Aim and tracking: hit the marker before it moves on. */
static void reaction_aim(const GParams *p, int VAR)
{
    int hits = 0, misses = 0, cr = 5, cc = 20;
    int ntarget = (VAR == R_MULTI) ? 3 : 1;
    int t2r[3], t2c[3], i;
    long deadline = now_ms() + 30000;

    for (i = 0; i < ntarget; i++) { t2r[i] = rnd(12); t2c[i] = rnd(40); }

    while (now_ms() < deadline) {
        int k;
        scr_clear();
        draw_title("REACTION", RNAME[VAR]);
        draw_box(5, 18, 14, 42, C_BLUE);
        for (i = 0; i < ntarget; i++)
            draw_textf(6 + t2r[i], 19 + t2c[i], "%sO%s", C_RED, C_RESET);
        draw_textf(6 + cr, 19 + cc, "%s+%s", C_BOLD C_CYAN, C_RESET);
        draw_textf(21, 18, "Hits %-4d Misses %-4d  %ld s left   ",
                   hits, misses, (deadline - now_ms()) / 1000);
        scr_flush();

        if (VAR == R_TRACKING) {
            /* The target drifts, so aiming has to lead it. */
            t2c[0] += rnd(3) - 1;
            t2r[0] += rnd(3) - 1;
            if (t2c[0] < 0) t2c[0] = 0;
            if (t2c[0] > 39) t2c[0] = 39;
            if (t2r[0] < 0) t2r[0] = 0;
            if (t2r[0] > 11) t2r[0] = 11;
        }

        k = key_poll();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        if (k == KEY_UP    && cr > 0)  cr--;
        if (k == KEY_DOWN  && cr < 11) cr++;
        if (k == KEY_LEFT  && cc > 0)  cc--;
        if (k == KEY_RIGHT && cc < 39) cc++;
        if (k == KEY_ENTER || k == ' ') {
            int got = 0;
            for (i = 0; i < ntarget; i++)
                if (cr == t2r[i] && cc == t2c[i]) {
                    hits++;
                    t2r[i] = rnd(12); t2c[i] = rnd(40);
                    got = 1;
                    break;
                }
            if (!got) misses++;
        }
        sleep_ms(VAR == R_TRACKING ? 60 : 25);
    }
    score_report(p->title ? p->title : "reaction", hits * 10 - misses * 2);
    scr_clear();
    draw_centered(12, 80, "Time.");
    pause_msg("Press any key...");
}

/* Stroop: the word and its colour disagree, and you answer for the colour. */
static void reaction_stroop(const GParams *p)
{
    static const char *NAME[4] = {"RED", "GREEN", "BLUE", "YELLOW"};
    static const char *COL[4] = {C_RED, C_GREEN, C_BLUE, C_YELLOW};
    static const char KEYS[4] = {'r', 'g', 'b', 'y'};
    int round, right = 0;

    for (round = 0; round < 12; round++) {
        int word = rnd(4), ink = rnd(4), k, i, pick = -1;
        scr_clear();
        draw_title("REACTION", "Stroop - answer for the INK colour, not the word");
        draw_textf(11, 34, "%s%s%s", C_BOLD, COL[ink], NAME[word]);
        draw_textf(14, 24, "%sr g b y%s", C_GREY, C_RESET);
        draw_textf(16, 24, "Score %d of %d   ", right, round);
        scr_flush();
        k = tolower(key_get());
        if (k == 'q' || k == KEY_ESC) break;
        for (i = 0; i < 4; i++) if (k == KEYS[i]) pick = i;
        if (pick < 0) { round--; continue; }
        if (pick == ink) right++;
        draw_textf(18, 24, "%s%-10s%s", pick == ink ? C_GREEN : C_RED,
                   pick == ink ? "Correct" : "Wrong", C_RESET);
        scr_flush();
        sleep_ms(400);
    }
    score_report(p->title ? p->title : "reaction", right * 100 / 12);
    pause_msg("Press any key...");
}

void fam_reaction(const GParams *p)
{
    int VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR >= R_COUNT) VAR = 0;
    switch (VAR) {
    case R_NBACK: case R_DIGITSPAN: case R_SEQREPEAT: case R_AUDIOMEM:
        reaction_memory(p, VAR); break;
    case R_PATTERN: case R_CHIMP: case R_VISUALSPAN: case R_CARDSPRINT:
        reaction_grid(p, VAR); break;
    case R_AIM: case R_FLICK: case R_MULTI: case R_TRACKING:
        reaction_aim(p, VAR); break;
    case R_STROOP:
        reaction_stroop(p); break;
    case R_RHYTHM: case R_PERIPHERAL: case R_SOUNDCUE: case R_COLOURCHANGE:
    default:
        reaction_cue(p, VAR); break;
    }
}

/* ================================================================ wordmisc
 * params: variant = which word game
 */
enum {
    W_CRYPTOGRAM, W_LADDER, W_BOGGLE, W_GHOST, W_SUPERGHOST, W_CHAIN,
    W_SPELLINGBEE, W_JOTTO, W_TEXTTWIST, W_COUNTDOWN, W_MISSINGVOWELS,
    W_PALINDROME, W_COUNT
};

static const char *WNAME[W_COUNT] = {
    "Cryptogram", "Word Ladder", "Boggle", "Ghost", "Superghost", "Word Chain",
    "Spelling Bee", "Jotto", "Text Twist", "Countdown Letters",
    "Missing Vowels", "Palindrome Hunt"
};

static int mini_read(int row, int col, char *buf, int max)
{
    int n = 0;
    buf[0] = '\0';
    for (;;) {
        int k;
        draw_textf(row, col, "%s%-30s%s", C_CYAN, buf, C_RESET);
        scr_flush();
        k = key_get();
        if (k == KEY_ESC) return 0;
        if (k == KEY_ENTER) return 1;
        if ((k == KEY_BACKSPACE || k == 127) && n > 0) { buf[--n] = '\0'; continue; }
        if (k < 32 || k > 126) continue;
        if (n < max - 1) { buf[n++] = (char)tolower(k); buf[n] = '\0'; }
    }
}

void fam_wordmisc(const GParams *p)
{
    int VAR = gp_int(p->variant, 0);
    int score = 0, rounds = 0;
    if (VAR < 0 || VAR >= W_COUNT) VAR = 0;

    for (;;) {
        char prompt[160] = "", typed[40] = "", target[32] = "";
        int correct = 0, i;

        switch (VAR) {
        case W_CRYPTOGRAM: {
            const char *w = theme_pick(rnd(THEME_COUNT));
            int shift = 1 + rnd(25);
            char enc[32];
            snprintf(target, sizeof target, "%s", w);
            for (i = 0; w[i] && i < 30; i++)
                enc[i] = (char)('a' + ((tolower((unsigned char)w[i]) - 'a' + shift) % 26));
            enc[i] = '\0';
            snprintf(prompt, sizeof prompt, "Shift cipher: %s", enc);
            break;
        }
        case W_LADDER: {
            const char *w = WORDS_BY_LEN[4][rnd(WORDS_BY_LEN_COUNT[4])];
            char changed[8];
            int pos = rnd(4);
            snprintf(changed, sizeof changed, "%s", w);
            changed[pos] = (char)('a' + rnd(26));
            snprintf(prompt, sizeof prompt, "Change one letter of %s back to a real word", changed);
            snprintf(target, sizeof target, "%s", w);
            break;
        }
        case W_BOGGLE: case W_COUNTDOWN: case W_TEXTTWIST: {
            const char *w = WORDS_BY_LEN[VAR == W_COUNTDOWN ? 8 : 6]
                            [rnd(WORDS_BY_LEN_COUNT[VAR == W_COUNTDOWN ? 8 : 6])];
            char pool[16];
            snprintf(pool, sizeof pool, "%s", w);
            for (i = (int)strlen(pool) - 1; i > 0; i--) {
                int j = rnd(i + 1);
                char t = pool[i]; pool[i] = pool[j]; pool[j] = t;
            }
            snprintf(prompt, sizeof prompt, "Make a word from these letters: %s", pool);
            snprintf(target, sizeof target, "%s", w);
            break;
        }
        case W_GHOST: case W_SUPERGHOST: case W_CHAIN: {
            const char *w = theme_pick(rnd(THEME_COUNT));
            char frag[8];
            int n = 2 + rnd(2);
            for (i = 0; i < n && w[i]; i++) frag[i] = w[i];
            frag[i] = '\0';
            snprintf(prompt, sizeof prompt,
                     VAR == W_CHAIN ? "Give a word starting with '%s'"
                                    : "Extend '%s' toward a real word", frag);
            snprintf(target, sizeof target, "%s", w);
            break;
        }
        case W_SPELLINGBEE: {
            const char *w = WORDS_BY_LEN[5][rnd(WORDS_BY_LEN_COUNT[5])];
            snprintf(prompt, sizeof prompt, "Use every letter of '%.*s' exactly once", 5, w);
            snprintf(target, sizeof target, "%s", w);
            break;
        }
        case W_JOTTO: {
            const char *w = WORDS_BY_LEN[5][rnd(WORDS_BY_LEN_COUNT[5])];
            snprintf(prompt, sizeof prompt, "Guess the five-letter word. First letter: %c", w[0]);
            snprintf(target, sizeof target, "%s", w);
            break;
        }
        case W_MISSINGVOWELS: {
            const char *w = theme_pick(rnd(THEME_COUNT));
            char stripped[32];
            int k = 0;
            for (i = 0; w[i]; i++)
                if (!strchr("aeiouAEIOU", w[i])) stripped[k++] = w[i];
            stripped[k] = '\0';
            snprintf(prompt, sizeof prompt, "Restore the vowels: %s", stripped);
            snprintf(target, sizeof target, "%s", w);
            break;
        }
        case W_PALINDROME:
        default: {
            static const char *PAL[8] = {"level","rotor","civic","radar","kayak","refer","madam","stats"};
            static const char *NOT[8] = {"table","chair","house","water","plant","stone","cloud","river"};
            int pal = rnd(2);
            snprintf(prompt, sizeof prompt, "Is '%s' a palindrome? type yes or no",
                     pal ? PAL[rnd(8)] : NOT[rnd(8)]);
            snprintf(target, sizeof target, "%s", pal ? "yes" : "no");
            break;
        }
        }

        scr_clear();
        draw_title("WORD GAMES", WNAME[VAR]);
        draw_textf(9, 10, "%s%-66s%s", C_BOLD, prompt, C_RESET);
        draw_textf(12, 10, "Answer: ");
        draw_textf(16, 10, "Score %d of %d     ", score, rounds);
        scr_flush();

        if (!mini_read(12, 18, typed, sizeof typed)) break;
        rounds++;
        correct = strcmp(typed, target) == 0;
        if (VAR == W_CHAIN || VAR == W_GHOST || VAR == W_SUPERGHOST)
            correct = correct || word_valid(typed, (int)strlen(typed));
        if (correct) score++;

        draw_textf(14, 10, "%s%-56s%s", correct ? C_GREEN : C_RED,
                   correct ? "Correct!" : "", C_RESET);
        if (!correct) draw_textf(14, 10, "%sNo - looking for '%s'.%s      ", C_RED, target, C_RESET);
        scr_flush();
        sleep_ms(450);
    }
    score_report(p->title ? p->title : "wordmisc", score);
    pause_msg("Press any key...");
}

/* ==================================================================== idle
 * params: variant = which theme
 *
 * Ten incremental games. The theme is not a skin: each has its own resource
 * name, its own three upgrades and its own cost curve, so the pacing differs.
 */
void fam_idle(const GParams *p)
{
    static const char *TNAME[10] = {
        "Mine", "Bakery", "Farm", "Factory", "Laboratory",
        "Galaxy", "Dungeon", "Garden", "Kingdom", "Reactor"};
    static const char *RES[10] = {
        "ore", "loaves", "crops", "widgets", "data",
        "stars", "gold", "blooms", "taxes", "joules"};
    static const char *UP[10][3] = {
        {"pickaxe","cart","drill"},      {"oven","mixer","shopfront"},
        {"hoe","tractor","irrigation"},  {"press","belt","robot"},
        {"assistant","microscope","grant"}, {"probe","warpdrive","colony"},
        {"torch","sword","map"},         {"spade","greenhouse","bees"},
        {"village","market","castle"},   {"rod","coolant","turbine"}};
    static const int BASE[10] = {10, 12, 8, 15, 20, 25, 14, 9, 18, 30};

    int ti = gp_int(p->variant, 0) % 10;
    double res = 0, rate = 0;
    int lvl[3] = {0, 0, 0};
    long last = now_ms(), started = now_ms();

    for (;;) {
        int k, i;
        double per = 0;
        for (i = 0; i < 3; i++) per += lvl[i] * (i + 1) * (i + 1);
        rate = per;

        if (now_ms() - last >= 250) {
            res += rate * (now_ms() - last) / 1000.0;
            last = now_ms();
        }

        scr_clear();
        draw_title("IDLE", TNAME[ti]);
        draw_textf(7, 18, "%s%.1f %s%s          ", C_BOLD C_YELLOW, res, RES[ti], C_RESET);
        draw_textf(8, 18, "%s%.1f per second%s   ", C_GREY, rate, C_RESET);
        for (i = 0; i < 3; i++) {
            int cost = BASE[ti] * (1 << lvl[i]);
            draw_textf(11 + i * 2, 18, "%d) %-12s level %-3d  cost %-6d",
                       i + 1, UP[ti][i], lvl[i], cost);
        }
        draw_textf(18, 18, "%sSpace gathers by hand, 1-3 buy upgrades, Q quits%s", C_GREY, C_RESET);
        draw_textf(20, 18, "Running %ld s", (now_ms() - started) / 1000);
        scr_flush();

        k = key_poll();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        if (k == ' ' || k == KEY_ENTER) res += 1;
        if (k >= '1' && k <= '3') {
            i = k - '1';
            {
                int cost = BASE[ti] * (1 << lvl[i]);
                if (res >= cost) { res -= cost; lvl[i]++; }
            }
        }
        sleep_ms(40);
    }
    score_report(p->title ? p->title : "idle", (int)res);
    scr_clear();
    draw_centered(12, 80, "Shift over.");
    pause_msg("Press any key...");
}
