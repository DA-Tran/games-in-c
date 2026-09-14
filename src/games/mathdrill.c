/* GIC:PARAMETERISED mathdrill
 * mathdrill.c - twenty number games.
 *
 * params: variant = which drill
 *
 * These share a prompt-and-check loop and nothing else: Countdown is a target
 * search, cryptarithm is a letter-to-digit puzzle, nim-sum trainer asks for an
 * xor, the Collatz race asks for a step count. Each generates its own problem
 * and validates its own answer.
 */
#include "engine.h"
#include "games.h"

enum {
    M_COUNTDOWN, M_24, M_FACTOR, M_PRIME, M_MENTAL, M_BINARY, M_COLLATZ,
    M_NIMSUM, M_FIZZBUZZ, M_EQUATION, M_FRACTION, M_SEQUENCE, M_MODULAR,
    M_DICEPROB, M_ESTIMATE, M_TRIANGLE, M_CRYPTARITHM, M_BASE, M_GCD,
    M_PI, M_COUNT
};

static const char *MNAME[M_COUNT] = {
    "Countdown Numbers", "24 Game", "Factor Game", "Prime Hunt",
    "Mental Arithmetic", "Binary Conversion Race", "Collatz Race",
    "Nim-Sum Trainer", "Fizz Buzz Duel", "Equation Builder", "Fraction Match",
    "Number Sequence", "Modular Clock", "Dice Probability", "Estimation",
    "Magic Triangle", "Cryptarithm", "Base Conversion", "GCD Duel",
    "Pi Digit Memory"
};

static const char *PI_DIGITS = "141592653589793238462643383279502884197169399375";

/* Read a line of typed input, echoing it at (row,col). */
static int drill_read_line(int row, int col, char *buf, int max)
{
    int n = 0;
    buf[0] = '\0';
    for (;;) {
        int k;
        draw_textf(row, col, "%s%-28s%s", C_CYAN, buf, C_RESET);
        scr_flush();
        k = key_get();
        if (k == 'q' && n == 0) return 0;
        if (k == KEY_ESC) return 0;
        if (k == KEY_ENTER) return 1;
        if (k == KEY_BACKSPACE || k == 127 || k == 8) { if (n > 0) buf[--n] = '\0'; continue; }
        if (k < 32 || k > 126) continue;
        if (n < max - 1) { buf[n++] = (char)k; buf[n] = '\0'; }
    }
}

static int gcd_of(int a, int b) { while (b) { int t = a % b; a = b; b = t; } return a; }

static int is_prime(int n)
{
    int i;
    if (n < 2) return 0;
    for (i = 2; i * i <= n; i++) if (n % i == 0) return 0;
    return 1;
}

static int collatz_steps(int n)
{
    int s = 0;
    while (n != 1 && s < 1000) { n = (n & 1) ? 3 * n + 1 : n / 2; s++; }
    return s;
}

/* Can the six numbers reach the target using + - * / ? Recursive search. */
static int reach(int *v, int n, int target)
{
    int i, j, k;
    if (n == 1) return v[0] == target;
    for (i = 0; i < n; i++) for (j = 0; j < n; j++) {
        int rest[8], m = 0, a = v[i], b = v[j], cand[4], nc = 0, c;
        if (i == j) continue;
        for (k = 0; k < n; k++) if (k != i && k != j) rest[m++] = v[k];
        cand[nc++] = a + b;
        cand[nc++] = a * b;
        if (a > b) cand[nc++] = a - b;
        if (b && a % b == 0) cand[nc++] = a / b;
        for (c = 0; c < nc; c++) {
            rest[m] = cand[c];
            if (reach(rest, m + 1, target)) return 1;
        }
    }
    return 0;
}

void fam_mathdrill(const GParams *p)
{
    int VAR = gp_int(p->variant, 0);
    int score = 0, asked = 0;
    if (VAR < 0 || VAR >= M_COUNT) VAR = 0;

    for (;;) {
        char prompt[160] = "", answer[40] = "", expect[40] = "";
        char note[120] = "";
        int correct = 0;

        switch (VAR) {
        case M_COUNTDOWN: {
            int v[6], i, target, tries;
            for (tries = 0; tries < 200; tries++) {
                static const int BIG[4] = {25, 50, 75, 100};
                for (i = 0; i < 6; i++) v[i] = (i < 2) ? BIG[rnd(4)] : 1 + rnd(10);
                target = 100 + rnd(900);
                if (reach(v, 6, target)) break;
            }
            snprintf(prompt, sizeof prompt, "Reach %d using %d %d %d %d %d %d",
                     target, v[0], v[1], v[2], v[3], v[4], v[5]);
            snprintf(note, sizeof note, "Type the target back once you have found a route.");
            snprintf(expect, sizeof expect, "%d", target);
            break;
        }
        case M_24: {
            int v[4], i;
            for (i = 0; i < 4; i++) v[i] = 1 + rnd(9);
            snprintf(prompt, sizeof prompt, "Make 24 from %d %d %d %d", v[0], v[1], v[2], v[3]);
            snprintf(note, sizeof note, "%s", reach(v, 4, 24) ? "This one is solvable." : "This one may not be solvable - type 0.");
            snprintf(expect, sizeof expect, "%d", reach(v, 4, 24) ? 24 : 0);
            break;
        }
        case M_FACTOR: {
            int n = 12 + rnd(80), i, sum = 0;
            for (i = 1; i < n; i++) if (n % i == 0) sum += i;
            snprintf(prompt, sizeof prompt, "Sum of the proper divisors of %d?", n);
            snprintf(expect, sizeof expect, "%d", sum);
            break;
        }
        case M_PRIME: {
            int n = 100 + rnd(400), next = n;
            while (!is_prime(next)) next++;
            snprintf(prompt, sizeof prompt, "First prime at or above %d?", n);
            snprintf(expect, sizeof expect, "%d", next);
            break;
        }
        case M_MENTAL: {
            int a = 2 + rnd(30), b = 2 + rnd(30), op = rnd(3), r;
            if (op == 0) { r = a + b; snprintf(prompt, sizeof prompt, "%d + %d = ?", a, b); }
            else if (op == 1) { r = a * b; snprintf(prompt, sizeof prompt, "%d x %d = ?", a, b); }
            else { if (a < b) { int t = a; a = b; b = t; } r = a - b; snprintf(prompt, sizeof prompt, "%d - %d = ?", a, b); }
            snprintf(expect, sizeof expect, "%d", r);
            break;
        }
        case M_BINARY: {
            int n = 1 + rnd(255), i, k = 0;
            char b[16];
            for (i = 7; i >= 0; i--) if (n >> i & 1 || k) b[k++] = (char)('0' + (n >> i & 1));
            b[k] = '\0';
            snprintf(prompt, sizeof prompt, "Write %d in binary", n);
            snprintf(expect, sizeof expect, "%s", b);
            break;
        }
        case M_COLLATZ: {
            int n = 3 + rnd(60);
            snprintf(prompt, sizeof prompt, "Collatz steps from %d down to 1?", n);
            snprintf(expect, sizeof expect, "%d", collatz_steps(n));
            break;
        }
        case M_NIMSUM: {
            int a = 1 + rnd(15), b = 1 + rnd(15), c = 1 + rnd(15);
            snprintf(prompt, sizeof prompt, "Nim-sum (xor) of heaps %d, %d, %d?", a, b, c);
            snprintf(expect, sizeof expect, "%d", a ^ b ^ c);
            break;
        }
        case M_FIZZBUZZ: {
            int n = 1 + rnd(100);
            snprintf(prompt, sizeof prompt, "Fizz Buzz for %d?", n);
            if (n % 15 == 0)     snprintf(expect, sizeof expect, "fizzbuzz");
            else if (n % 3 == 0) snprintf(expect, sizeof expect, "fizz");
            else if (n % 5 == 0) snprintf(expect, sizeof expect, "buzz");
            else                 snprintf(expect, sizeof expect, "%d", n);
            snprintf(note, sizeof note, "Type fizz, buzz, fizzbuzz or the number.");
            break;
        }
        case M_EQUATION: {
            int a = 2 + rnd(12), x = 1 + rnd(12), b = 1 + rnd(20);
            snprintf(prompt, sizeof prompt, "Solve for x:  %d x + %d = %d", a, b, a * x + b);
            snprintf(expect, sizeof expect, "%d", x);
            break;
        }
        case M_FRACTION: {
            int a = 1 + rnd(9), b = 2 + rnd(9), m = 2 + rnd(6);
            int g = gcd_of(a * m, b * m);
            snprintf(prompt, sizeof prompt, "Simplify %d/%d - give the numerator", a * m, b * m);
            snprintf(expect, sizeof expect, "%d", (a * m) / g);
            break;
        }
        case M_SEQUENCE: {
            int start = 1 + rnd(9), step = 2 + rnd(7), kind = rnd(3), t[5], i;
            for (i = 0; i < 5; i++) {
                if (kind == 0) t[i] = start + step * i;
                else if (kind == 1) t[i] = start * (i + 1) * (i + 1);
                else t[i] = start * (1 << i);
            }
            snprintf(prompt, sizeof prompt, "Next in the sequence: %d %d %d %d ?", t[0], t[1], t[2], t[3]);
            snprintf(expect, sizeof expect, "%d", t[4]);
            break;
        }
        case M_MODULAR: {
            int h = rnd(12), add = 1 + rnd(40);
            snprintf(prompt, sizeof prompt, "It is %d o'clock. What time in %d hours? (0-11)", h, add);
            snprintf(expect, sizeof expect, "%d", (h + add) % 12);
            break;
        }
        case M_DICEPROB: {
            int target = 2 + rnd(11), ways = 0, a, b;
            for (a = 1; a <= 6; a++) for (b = 1; b <= 6; b++) if (a + b == target) ways++;
            snprintf(prompt, sizeof prompt, "Two dice: how many of the 36 ways total %d?", target);
            snprintf(expect, sizeof expect, "%d", ways);
            break;
        }
        case M_ESTIMATE: {
            int a = 100 + rnd(900), b = 10 + rnd(90);
            snprintf(prompt, sizeof prompt, "Estimate %d x %d to the nearest thousand", a, b);
            snprintf(expect, sizeof expect, "%d", ((a * b + 500) / 1000) * 1000);
            snprintf(note, sizeof note, "Within 10 percent counts as correct.");
            break;
        }
        case M_TRIANGLE: {
            int side = 9 + rnd(6);
            snprintf(prompt, sizeof prompt,
                     "Place 1-6 on a triangle so each side sums to %d. Total of all six?", side);
            snprintf(expect, sizeof expect, "21");
            break;
        }
        case M_CRYPTARITHM: {
            int a = 1 + rnd(8), b = 1 + rnd(8);
            snprintf(prompt, sizeof prompt,
                     "If A=%d and B=%d, what is AB + BA as a number?", a, b);
            snprintf(expect, sizeof expect, "%d", (a * 10 + b) + (b * 10 + a));
            break;
        }
        case M_BASE: {
            int n = 10 + rnd(200), base = rnd(2) ? 8 : 16, k = 0, i;
            char b[16], tmp[16];
            int v = n;
            while (v) { tmp[k++] = "0123456789ABCDEF"[v % base]; v /= base; }
            for (i = 0; i < k; i++) b[i] = tmp[k - 1 - i];
            b[k] = '\0';
            snprintf(prompt, sizeof prompt, "Write %d in base %d", n, base);
            snprintf(expect, sizeof expect, "%s", b);
            break;
        }
        case M_GCD: {
            int a = 12 + rnd(200), b = 12 + rnd(200);
            snprintf(prompt, sizeof prompt, "Greatest common divisor of %d and %d?", a, b);
            snprintf(expect, sizeof expect, "%d", gcd_of(a, b));
            break;
        }
        case M_PI:
        default: {
            int pos = rnd(40);
            snprintf(prompt, sizeof prompt, "Digit %d of pi after the point?", pos + 1);
            snprintf(expect, sizeof expect, "%c", PI_DIGITS[pos]);
            break;
        }
        }

        scr_clear();
        draw_title("NUMBER DRILL", MNAME[VAR]);
        draw_textf(8, 10, "%s%-68s%s", C_BOLD, prompt, C_RESET);
        if (note[0]) draw_textf(10, 10, "%s%-68s%s", C_GREY, note, C_RESET);
        draw_textf(12, 10, "Answer: ");
        draw_textf(16, 10, "Score %d of %d      ", score, asked);
        scr_flush();

        if (!drill_read_line(12, 18, answer, sizeof answer)) break;
        asked++;

        if (VAR == M_ESTIMATE) {
            long want = atol(expect), got = atol(answer);
            long slack = want / 10 + 1;
            correct = (got >= want - slack && got <= want + slack);
        } else {
            char a[40], e[40];
            int i, j = 0;
            for (i = 0; answer[i]; i++) if (!isspace((unsigned char)answer[i])) a[j++] = (char)tolower((unsigned char)answer[i]);
            a[j] = '\0';
            j = 0;
            for (i = 0; expect[i]; i++) if (!isspace((unsigned char)expect[i])) e[j++] = (char)tolower((unsigned char)expect[i]);
            e[j] = '\0';
            correct = strcmp(a, e) == 0;
        }
        if (correct) score++;

        draw_textf(14, 10, "%s%-60s%s", correct ? C_GREEN : C_RED,
                   correct ? "Correct!" : "", C_RESET);
        if (!correct) draw_textf(14, 10, "%sNot quite - the answer was %s.%s      ", C_RED, expect, C_RESET);
        draw_textf(16, 10, "Score %d of %d      ", score, asked);
        scr_flush();
        sleep_ms(700);
    }
    score_report(p->title ? p->title : "mathdrill", score);
    scr_clear();
    draw_centered(12, 80, "Drill over.");
    pause_msg("Press any key...");
}
