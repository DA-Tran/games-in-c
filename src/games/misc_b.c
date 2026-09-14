/* GIC:PARAMETERISED rps
 * GIC:PARAMETERISED simon
 * GIC:PARAMETERISED snakesladders
 * GIC:PARAMETERISED piano
 *
 * misc_b.c - rock paper scissors, Simon, the race-game family and the
 * keyboard-instrument family.
 *
 * Split out of misc.c so the four parameterised families here sit together.
 * Each marker above vouches only for its own family.
 */
#include "engine.h"
#include "games.h"

/* ------------------------------------------------------- rock paper scissors
 * params: variant 0 = classic, 1 = lizard/Spock, 2 = best of nine
 *
 * With the throws listed in the cyclic order below, a throw beats the n/2
 * entries immediately behind it. That one rule covers both the three-throw
 * and five-throw games, so adding Lizard and Spock needs no special cases.
 */
void fam_rps(const GParams *p)
{
    static const char *N3[3] = {"Rock", "Paper", "Scissors"};
    static const char *N5[5] = {"Rock", "Spock", "Paper", "Lizard", "Scissors"};
    static const char  K3[3] = {'r', 'p', 's'};
    static const char  K5[5] = {'r', 'k', 'p', 'l', 's'};
    static const char *VERB[5][5] = {
        {"", "", "covers", "crushes", "crushes"},
        {"vaporizes", "", "", "poisons", "smashes"},
        {"covers", "disproves", "", "", "eats"},
        {"crushes", "poisons", "eats", "", ""},
        {"crushes", "smashes", "cuts", "decapitates", ""}};

    int var = gp_int(p->variant, 0);
    int n = (var == 1) ? 5 : 3;
    const char **NAME = (var == 1) ? N5 : N3;
    const char *KEYS  = (var == 1) ? K5 : K3;
    int target = (var == 2) ? 5 : 0;          /* best of nine = first to five */
    int freq[5] = {0, 0, 0, 0, 0};
    int wins = 0, losses = 0, draws = 0, i;
    char keyhelp[80] = "";

    for (i = 0; i < n; i++) {
        char bit[16];
        snprintf(bit, sizeof bit, "%c %s  ", KEYS[i], NAME[i]);
        strncat(keyhelp, bit, sizeof keyhelp - strlen(keyhelp) - 1);
    }

    for (;;) {
        int k, me = -1, ai, d;
        draw_title(p->title ? p->title : "ROCK PAPER SCISSORS", keyhelp);
        draw_textf(9, 24, "Wins %-3d  Losses %-3d  Draws %-3d", wins, losses, draws);
        if (target) draw_textf(10, 24, "%sFirst to %d takes the match.%s   ", C_GREY, target, C_RESET);
        else        draw_textf(10, 24, "%sThe computer is tracking your habits.%s", C_GREY, C_RESET);
        scr_flush();

        k = tolower(key_get());
        if (k == 'q' || k == KEY_ESC) break;
        for (i = 0; i < n; i++) if (k == KEYS[i]) me = i;
        if (me < 0) continue;

        /* Counter whatever the player throws most often. */
        {
            int most = 0;
            for (i = 1; i < n; i++) if (freq[i] > freq[most]) most = i;
            ai = (freq[most] > 0 && rnd(100) < 65) ? (most + 1) % n : rnd(n);
        }
        freq[me]++;

        d = (me - ai + n) % n;                /* 1..n/2 means the player wins */
        if (d == 0) draws++;
        else if (d <= n / 2) wins++;
        else losses++;

        draw_textf(13, 24, "You: %-10s   CPU: %-10s", NAME[me], NAME[ai]);
        if (d == 0)
            draw_textf(15, 24, "%s%-46s%s", C_BOLD, "Draw.", C_RESET);
        else if (d <= n / 2)
            draw_textf(15, 24, "%s%s %s %s — you win the round!%s   ", C_BOLD,
                       NAME[me], VERB[me][ai][0] ? VERB[me][ai] : "beats", NAME[ai], C_RESET);
        else
            draw_textf(15, 24, "%s%s %s %s — computer wins.%s   ", C_BOLD,
                       NAME[ai], VERB[ai][me][0] ? VERB[ai][me] : "beats", NAME[me], C_RESET);
        scr_flush();
        sleep_ms(900);

        if (target && (wins >= target || losses >= target)) {
            draw_centered(18, 80, wins >= target ? C_BOLD C_GREEN "You take the match!" C_RESET
                                                 : C_BOLD C_RED "Computer takes the match." C_RESET);
            scr_flush();
            score_report(p->title ? p->title : "rps", wins);
            if (!confirm("\n  Play again?")) return;
            wins = losses = draws = 0;
            for (i = 0; i < 5; i++) freq[i] = 0;
        }
    }
    score_report(p->title ? p->title : "rps", wins);
    pause_msg("Press any key...");
}

/* ------------------------------------------------------------------- simon
 * params: variant 0 four colours, 1 six, 2 eight, 3 reverse, 4 silent,
 *         5 speed
 */
void fam_simon(const GParams *p)
{
    static const char *CNAME[8] = {"RED","GREEN","BLUE","YELLOW","MAGENTA","CYAN","WHITE","GREY"};
    static const char *CCOL[8]  = {C_RED,C_GREEN,C_BLUE,C_YELLOW,C_MAGENTA,C_CYAN,C_WHITE,C_GREY};
    static const char KEYS[8]   = {'r','g','b','y','m','c','w','k'};

    int var = gp_int(p->variant, 0);
    int ncol = (var == 1) ? 6 : (var == 2) ? 8 : 4;
    int reverse = (var == 3);
    int silent  = (var == 4);
    int show    = (var == 5) ? 240 : 480;     /* speed mode flashes briefly */
    int gap     = (var == 5) ? 90  : 180;
    char help[96];

    snprintf(help, sizeof help, "Repeat%s with %.*s%s",
             reverse ? " BACKWARDS" : "", ncol, "rgbymcwk",
             silent ? " (no sound)" : "");

    for (;;) {
        int seq[64], len = 0, alive = 1;

        while (alive && len < 64) {
            int i;
            seq[len++] = rnd(ncol);

            draw_title(p->title ? p->title : "SIMON", help);
            draw_textf(9, 30, "Round %d   ", len);
            scr_flush();
            sleep_ms(500);

            for (i = 0; i < len; i++) {
                draw_textf(12, 30, "%s%s  ######  %s", C_BOLD, CCOL[seq[i]], C_RESET);
                draw_textf(14, 30, "%s%-10s%s", CCOL[seq[i]], CNAME[seq[i]], C_RESET);
                if (!silent) beep(380 + seq[i] * 110, show / 2);
                scr_flush();
                sleep_ms(show);
                draw_text(12, 30, "            ");
                draw_text(14, 30, "          ");
                scr_flush();
                sleep_ms(gap);
            }

            draw_textf(12, 30, "%sYour turn - %d step%s%s%s", C_CYAN, len,
                       len == 1 ? "" : "s", reverse ? ", backwards" : "", C_RESET);
            scr_flush();

            for (i = 0; i < len; i++) {
                int k = tolower(key_get()), j, pick = -1;
                int want = reverse ? seq[len - 1 - i] : seq[i];
                if (k == 'q' || k == KEY_ESC) return;
                for (j = 0; j < ncol; j++) if (k == KEYS[j]) pick = j;
                if (pick < 0) { i--; continue; }
                draw_textf(14, 30, "%s%-10s%s", CCOL[pick], CNAME[pick], C_RESET);
                scr_flush();
                if (pick != want) { alive = 0; break; }
                sleep_ms(110);
            }
            if (!alive) break;
            draw_textf(16, 30, "%sCorrect!%s      ", C_GREEN, C_RESET);
            scr_flush();
            sleep_ms(450);
            draw_text(16, 30, "              ");
        }
        {
            char m[64];
            snprintf(m, sizeof m, "Wrong - you reached round %d.", len);
            draw_centered(18, 80, m);
            scr_flush();
            score_report(p->title ? p->title : "simon", len - 1);
        }
        if (!confirm("\n  Play again?")) return;
    }
}

/* -------------------------------------------------------------- race games
 * params: variant selects one of eight historical race games.
 *
 * These differ in board length, what you roll, whether you must land on the
 * final square exactly, whether a piece needs a specific roll to enter, and
 * whether some squares grant another turn. Those five switches are enough to
 * separate Senet and Ur from snakes and ladders honestly.
 */
typedef struct {
    const char *name;
    int  squares;
    int  dice;        /* 0 = one d6, 1 = four sticks (0-4), 2 = yut (1-5) */
    int  exact;       /* must land exactly on the last square             */
    int  entry;       /* roll needed to bring a piece on, 0 = none        */
    int  rosettes;    /* landing on a marked square grants another turn   */
    int  njump;
    const int *from;
    const int *to;
} Race;

static const int SL_F[] = {1,4,9,21,28,36,51,71,80,16,47,49,56,62,64,87,93,95,98};
static const int SL_T[] = {38,14,31,42,84,44,67,91,100,6,26,11,53,19,60,24,73,75,78};
static const int DX_F[] = {1,4,9,21,28,36,51,71,80,16,47,49,56,62,64,87,93,95,98,6,20,33,45,68};
static const int DX_T[] = {38,14,31,42,84,44,67,91,100,6,26,11,53,19,60,24,73,75,78,27,41,12,72,50};
static const int GO_F[] = {6,12,18,24,30,36,42,48,54,58,19,31,42,52};
static const int GO_T[] = {12,18,24,30,36,42,48,54,60,63,55,12,26,30};
static const int PA_F[] = {12,25,38,51};
static const int PA_T[] = {30,44,57,60};
static const int LU_F[] = {9,22,35,48};
static const int LU_T[] = {27,40,53,56};
static const int SE_F[] = {15,26,27,28};
static const int SE_T[] = {1,15,15,15};
static const int UR_F[] = {0};
static const int UR_T[] = {0};
static const int YU_F[] = {5,10,22};
static const int YU_T[] = {15,20,27};

static const Race RACES[8] = {
    {"Snakes and Ladders",       100, 0, 0, 0, 0, 19, SL_F, SL_T},
    {"Chutes and Ladders Deluxe",100, 0, 0, 0, 0, 24, DX_F, DX_T},
    {"Game of the Goose",         63, 0, 1, 0, 0, 14, GO_F, GO_T},
    {"Pachisi",                   60, 0, 1, 0, 0,  4, PA_F, PA_T},
    {"Ludo",                      56, 0, 1, 6, 0,  4, LU_F, LU_T},
    {"Senet",                     30, 1, 1, 0, 0,  4, SE_F, SE_T},
    {"Royal Game of Ur",          20, 1, 1, 0, 1,  1, UR_F, UR_T},
    {"Yut Nori",                  29, 2, 0, 0, 0,  3, YU_F, YU_T}
};

/* Ur's rosettes: landing on one gives another throw. */
static int is_rosette(int sq) { return sq == 4 || sq == 8 || sq == 14; }

static int race_roll(int dice)
{
    if (dice == 1) {                       /* four binary sticks */
        int i, s = 0;
        for (i = 0; i < 4; i++) s += rnd(2);
        return s == 0 ? 5 : s;             /* all-blank throws the maximum */
    }
    if (dice == 2) {                       /* yut sticks, 1-5 */
        static const int W[5] = {35, 30, 20, 10, 5};
        int r = rnd(100), i, acc = 0;
        for (i = 0; i < 5; i++) { acc += W[i]; if (r < acc) return i + 1; }
        return 1;
    }
    return rnd_range(1, 6);
}

void fam_snakes_ladders(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    Race R;
    int cols, rows;

    if (v < 0 || v > 7) v = 0;
    R = RACES[v];
    cols = (R.squares >= 60) ? 10 : (R.squares >= 29) ? 10 : 5;
    rows = (R.squares + cols - 1) / cols;

    for (;;) {
        int pos[2] = {0, 0}, live[2] = {!R.entry, !R.entry}, turn = 0, over = 0;

        while (!over) {
            int k, die, i, from, again = 0;
            char msg[110] = "";
            char sub[110];

            snprintf(sub, sizeof sub, "%s — %s%s%s", R.name,
                     R.dice == 1 ? "throw sticks" : R.dice == 2 ? "throw yut" : "roll a die",
                     R.exact ? ", exact finish" : "",
                     R.entry ? ", roll a 6 to start" : "");
            draw_title(p->title ? p->title : "RACE", sub);

            for (i = 0; i < rows; i++) {
                int c;
                scr_move(5 + i, 20);
                for (c = 0; c < cols; c++) {
                    int row = rows - 1 - i;
                    int sq = row * cols + ((row % 2 == 0) ? c + 1 : cols - c);
                    const char *col = C_GREY;
                    int j;
                    if (sq > R.squares) { printf("     "); continue; }
                    for (j = 0; j < R.njump; j++) {
                        if (R.from[j] != sq) continue;
                        col = R.to[j] > sq ? C_GREEN : C_RED;
                    }
                    if (R.rosettes && is_rosette(sq)) col = C_MAGENTA;
                    if (pos[0] == sq && pos[1] == sq) printf("%s[**]%s", C_BOLD, C_RESET);
                    else if (pos[0] == sq)            printf("%s[Y ]%s", C_CYAN, C_RESET);
                    else if (pos[1] == sq)            printf("%s[ C]%s", C_MAGENTA, C_RESET);
                    else                              printf("%s%3d %s", col, sq, C_RESET);
                }
            }
            draw_textf(6 + rows, 20, "%sYou: %-3d%s   %sCPU: %-3d%s   %s   ",
                       C_CYAN, pos[0], C_RESET, C_MAGENTA, pos[1], C_RESET,
                       turn == 0 ? "Your turn " : "CPU turn  ");
            draw_textf(8 + rows, 20, "%sGreen climbs, red falls%s%s   ", C_GREY,
                       R.rosettes ? ", magenta grants another throw" : "", C_RESET);
            scr_flush();

            if (turn == 0) {
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                if (k != KEY_ENTER && k != ' ') continue;
            } else {
                sleep_ms(220);            /* the computer's "thinking" beat */
            }

            die = race_roll(R.dice);
            from = pos[turn];

            if (!live[turn]) {
                if (die == R.entry) {
                    live[turn] = 1;
                    pos[turn] = 1;
                    snprintf(msg, sizeof msg, "%s rolled %d and enters the board.",
                             turn == 0 ? "You" : "Computer", die);
                } else {
                    snprintf(msg, sizeof msg, "%s rolled %d — still waiting for a %d.",
                             turn == 0 ? "You" : "Computer", die, R.entry);
                }
            } else if (from + die > R.squares && R.exact) {
                snprintf(msg, sizeof msg, "%s rolled %d — overshoots, no move.",
                         turn == 0 ? "You" : "Computer", die);
            } else {
                pos[turn] = from + die;
                if (pos[turn] > R.squares) pos[turn] = R.squares;
                snprintf(msg, sizeof msg, "%s rolled %d: %d -> %d",
                         turn == 0 ? "You" : "Computer", die, from, pos[turn]);
                for (i = 0; i < R.njump; i++) {
                    if (R.from[i] != pos[turn]) continue;
                    snprintf(msg, sizeof msg, "%s rolled %d: %d -> %d, then %s to %d!",
                             turn == 0 ? "You" : "Computer", die, from, pos[turn],
                             R.to[i] > pos[turn] ? "climbs" : "falls back", R.to[i]);
                    pos[turn] = R.to[i];
                    break;
                }
                if (R.rosettes && is_rosette(pos[turn])) {
                    again = 1;
                    strncat(msg, "  Rosette — throw again!", sizeof msg - strlen(msg) - 1);
                }
            }

            draw_textf(10 + rows, 20, "%s%-72s%s", C_BOLD, msg, C_RESET);
            scr_flush();
            sleep_ms(turn == 0 ? 300 : 420);

            if (pos[turn] >= R.squares) {
                draw_centered(12 + rows, 80,
                              turn == 0 ? C_BOLD C_GREEN "You reach home — you win!" C_RESET
                                        : C_BOLD C_RED "Computer reaches home first." C_RESET);
                scr_flush();
                score_report(p->title ? p->title : "race", turn == 0 ? R.squares : 0);
                over = 1;
            }
            if (!again) turn = 1 - turn;
        }
        if (!confirm("\n  Play again?")) return;
    }
}

/* ------------------------------------------------------ keyboard instrument
 * params: variant 0 free play, 1 rhythm, 2 note naming, 3 intervals,
 *         4 chords, 5 scales, 6 drums, 7 melody memory, 8 pitch test,
 *         9 metronome, 10 sequencer
 */
static const char  PKEYS[]  = "zsxdcvgbhnjm,l.;/q2w3er5t6y7ui9o0p";
static const char *PNAMES[] = {"C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4",
                               "A#4","B4","C5","C#5","D5","D#5","E5","F5","F#5","G5",
                               "G#5","A5","A#5","B5","C6","C#6","D6","D#6","E6","F6",
                               "F#6","G6","G#6","A6"};
static const int PFREQ[] = {262,277,294,311,330,349,370,392,415,440,466,494,
                            523,554,587,622,659,698,740,784,831,880,932,988,
                            1047,1109,1175,1245,1319,1397,1480,1568,1661,1760};
#define PNKEYS ((int)(sizeof(PKEYS) - 1))

static int piano_index(int k)
{
    int i;
    for (i = 0; i < PNKEYS; i++) if (PKEYS[i] == tolower(k)) return i;
    return -1;
}

static void piano_free(const GParams *p)
{
    char history[64] = "";
    for (;;) {
        int k, idx;
        draw_title(p->title ? p->title : "VIRTUAL PIANO",
                   "Play with z s x d c v g b h n j m  /  q 2 w 3 e r 5 t 6 y 7 u");
        draw_text(8, 12, "  +-++-++-+-++-++-++-+-+  +-++-++-+-++-++-++-+-+");
        draw_text(9, 12, "  | || || | || || || | |  | || || | || || || | |");
        draw_text(10,12, "  | ++ ++ | ++ ++ ++ | |  | ++ ++ | ++ ++ ++ | |");
        draw_text(11,12, "  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |");
        draw_text(12,12, "  +--+--+--+--+--+--+--+  +--+--+--+--+--+--+--+");
        draw_text(13,12, "   z  x  c  v  b  n  m     q  w  e  r  t  y  u");
        draw_textf(16, 12, "%sRecent: %-48s%s", C_GREY, history, C_RESET);
        draw_text(18, 12, "Press Q to quit");
        scr_flush();

        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
        idx = piano_index(k);
        if (idx < 0) continue;

        beep(PFREQ[idx], 200);
        draw_textf(20, 12, "%s*  %-5s  %4d Hz%s   ", C_BOLD C_YELLOW, PNAMES[idx], PFREQ[idx], C_RESET);
        scr_flush();
        {
            char add[8];
            snprintf(add, sizeof add, "%s ", PNAMES[idx]);
            if (strlen(history) + strlen(add) >= sizeof history) history[0] = '\0';
            strncat(history, add, sizeof history - strlen(history) - 1);
        }
        sleep_ms(80);
    }
}

/* Name the note, the interval or the pitch you just heard. */
static void piano_quiz(const GParams *p, int mode)
{
    static const char *IVAL[13] = {"unison","minor 2nd","major 2nd","minor 3rd",
        "major 3rd","perfect 4th","tritone","perfect 5th","minor 6th",
        "major 6th","minor 7th","major 7th","octave"};
    int score = 0, asked = 0;

    for (;;) {
        int a = rnd(21), step = rnd_range(1, 12), i, k;
        char prompt[96];

        draw_title(p->title ? p->title : "EAR TRAINING",
                   mode == 2 ? "Which note is shown? Press its key. Q quits"
                 : mode == 3 ? "Name the interval: press 1-9 then 0,-,= for 10-12"
                             : "Which note did you hear? Press its key. Q quits");

        if (mode == 2) {
            snprintf(prompt, sizeof prompt, "Find %s on the keyboard.", PNAMES[a]);
        } else if (mode == 3) {
            snprintf(prompt, sizeof prompt, "Listen to the two notes...");
        } else {
            snprintf(prompt, sizeof prompt, "Listen...");
        }
        draw_textf(10, 20, "%s%-56s%s", C_BOLD, prompt, C_RESET);
        draw_textf(12, 20, "Score %d / %d      ", score, asked);
        draw_text(14, 20, "                                                  ");
        scr_flush();

        if (mode == 3) {
            beep(PFREQ[a], 200); sleep_ms(70);
            beep(PFREQ[a + step], 200);
        } else if (mode == 8) {
            beep(PFREQ[a], 320);
        }

        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        asked++;

        if (mode == 3) {
            static const char DIG[13] = {'0','1','2','3','4','5','6','7','8','9','0','-','='};
            int want = step;
            int got = -1;
            for (i = 1; i <= 12; i++) if (k == DIG[i]) got = i;
            if (got == want) { score++; draw_textf(14, 20, "%sCorrect — %s%s      ", C_GREEN, IVAL[want], C_RESET); }
            else draw_textf(14, 20, "%sIt was a %s.%s         ", C_RED, IVAL[want], C_RESET);
        } else {
            int idx = piano_index(k);
            if (idx < 0) { asked--; continue; }
            beep(PFREQ[idx], 200);
            if (idx == a) { score++; draw_textf(14, 20, "%sCorrect — %s%s      ", C_GREEN, PNAMES[a], C_RESET); }
            else draw_textf(14, 20, "%sThat was %s; wanted %s.%s   ", C_RED, PNAMES[idx], PNAMES[a], C_RESET);
        }
        scr_flush();
        sleep_ms(420);
    }
    score_report(p->title ? p->title : "piano", score);
    pause_msg("Press any key...");
}

/* Play a named chord or scale from its root. */
static void piano_build(const GParams *p, int mode)
{
    static const char *CHORD[5] = {"major","minor","diminished","augmented","dominant 7th"};
    static const int CIVL[5][4] = {{0,4,7,-1},{0,3,7,-1},{0,3,6,-1},{0,4,8,-1},{0,4,7,10}};
    static const char *SCALE[4] = {"major","natural minor","pentatonic","blues"};
    static const int SIVL[4][8] = {{0,2,4,5,7,9,11,12},{0,2,3,5,7,8,10,12},
                                   {0,2,4,7,9,12,-1,-1},{0,3,5,6,7,10,12,-1}};
    int score = 0, asked = 0;

    for (;;) {
        int root = rnd(12), kind = rnd(mode == 4 ? 5 : 4);
        const int *want = mode == 4 ? CIVL[kind] : SIVL[kind];
        int nlen = 0, i, ok = 1;

        while (nlen < (mode == 4 ? 4 : 8) && want[nlen] >= 0) nlen++;

        draw_title(p->title ? p->title : "THEORY",
                   mode == 4 ? "Play the chord, lowest note first. Q quits"
                             : "Play the scale upwards. Q quits");
        draw_textf(10, 20, "%s%s %s%s            ", C_BOLD, PNAMES[root],
                   mode == 4 ? CHORD[kind] : SCALE[kind], C_RESET);
        draw_textf(12, 20, "Score %d / %d       ", score, asked);
        draw_text(14, 20, "                                                  ");
        scr_flush();

        for (i = 0; i < nlen; i++) {
            int k = key_get(), idx;
            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto done;
            idx = piano_index(k);
            if (idx < 0) { i--; continue; }
            beep(PFREQ[idx], 180);
            draw_textf(14, 20 + i * 6, "%s%-5s%s", C_CYAN, PNAMES[idx], C_RESET);
            scr_flush();
            if (idx != root + want[i]) ok = 0;
        }
        asked++;
        if (ok) { score++; draw_textf(16, 20, "%sCorrect!%s          ", C_GREEN, C_RESET); }
        else    draw_textf(16, 20, "%sNot quite.%s        ", C_RED, C_RESET);
        scr_flush();
        sleep_ms(420);
    }
done:
    score_report(p->title ? p->title : "piano", score);
    pause_msg("Press any key...");
}

/* Repeat a melody back, Simon-style but on the keyboard. */
static void piano_memory(const GParams *p)
{
    int score = 0;
    for (;;) {
        int seq[32], len = 0, alive = 1;
        while (alive && len < 32) {
            int i;
            seq[len++] = rnd(12);
            draw_title(p->title ? p->title : "MELODY MEMORY", "Listen, then play it back. Q quits");
            draw_textf(10, 24, "Phrase of %d note%s   ", len, len == 1 ? "" : "s");
            draw_text(12, 24, "                                     ");
            scr_flush();
            sleep_ms(400);
            for (i = 0; i < len; i++) {
                draw_textf(12, 24, "%s%-5s%s   ", C_YELLOW, PNAMES[seq[i]], C_RESET);
                scr_flush();
                beep(PFREQ[seq[i]], 260);
                sleep_ms(300);
                draw_text(12, 24, "        ");
                scr_flush();
                sleep_ms(90);
            }
            draw_textf(12, 24, "%sYour turn%s      ", C_CYAN, C_RESET);
            scr_flush();
            for (i = 0; i < len; i++) {
                int k = key_get(), idx;
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                idx = piano_index(k);
                if (idx < 0) { i--; continue; }
                beep(PFREQ[idx], 170);
                if (idx != seq[i]) { alive = 0; break; }
            }
            if (!alive) break;
            if (len > score) score = len;
            draw_textf(14, 24, "%sCorrect!%s   ", C_GREEN, C_RESET);
            scr_flush();
            sleep_ms(450);
            draw_text(14, 24, "            ");
        }
        draw_centered(16, 80, "Wrong note.");
        scr_flush();
        score_report(p->title ? p->title : "piano", score);
        if (!confirm("\n  Play again?")) return;
    }
}

/* Timing games: scrolling notes, metronome tapping, step sequencing. */
static void piano_timing(const GParams *p, int mode)
{
    int score = 0, misses = 0, tick = 0;
    int lane[8], nlane = mode == 6 ? 4 : 6, i;
    int period = mode == 9 ? 600 : 420;
    int drum_pat[4][16];

    for (i = 0; i < 8; i++) lane[i] = -1;
    for (i = 0; i < 4; i++) { int j; for (j = 0; j < 16; j++) drum_pat[i][j] = 0; }

    for (;;) {
        int k;
        draw_title(p->title ? p->title : "RHYTHM",
                   mode == 6 ? "1-4 toggle a drum on this step, Q quits"
                 : mode == 9 ? "Tap SPACE exactly on the beat, Q quits"
                 : mode == 10 ? "Press keys to record, Enter plays back, Q quits"
                              : "Hit the note key as it reaches the line, Q quits");

        if (mode == 1) {                        /* scrolling note lanes */
            for (i = 0; i < nlane; i++) {
                int x = 18 + i * 8;
                draw_textf(6, x, "%s%c%s", C_GREY, PKEYS[i], C_RESET);
                draw_textf(7 + (lane[i] < 0 ? 0 : lane[i]), x,
                           lane[i] < 0 ? "  " : (lane[i] >= 8 ? "##" : "[]"));
            }
            draw_text(15, 14, "--------------------------------------------------");
        } else if (mode == 6) {                 /* drum grid */
            static const char *DNAME[4] = {"kick","snare","hihat","clap"};
            int st;
            for (i = 0; i < 4; i++) {
                draw_textf(7 + i * 2, 16, "%s%-6s%s", C_CYAN, DNAME[i], C_RESET);
                for (st = 0; st < 16; st++)
                    draw_textf(7 + i * 2, 24 + st * 2, "%s%s%s",
                               st == tick % 16 ? C_REV : "",
                               drum_pat[i][st] ? "#" : ".", C_RESET);
            }
        } else if (mode == 9) {
            draw_textf(10, 30, "%s%s%s", C_BOLD,
                       (tick % 4 == 0) ? "  * BEAT *  " : "            ", C_RESET);
        }
        draw_textf(18, 18, "Hits %-4d  Misses %-4d   ", score, misses);
        scr_flush();

        if (mode == 1) {
            for (i = 0; i < nlane; i++) {
                if (lane[i] >= 0) lane[i]++;
                if (lane[i] > 8) { lane[i] = -1; misses++; }
            }
            if (rnd(100) < 40) lane[rnd(nlane)] = 0;
        }
        if (mode == 6) {
            for (i = 0; i < 4; i++)
                if (drum_pat[i][tick % 16]) beep(120 + i * 180, 60);
        }
        if (mode == 9 && tick % 4 == 0) beep(880, 70);

        tick++;
        k = key_poll();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;

        if (mode == 1 && k != KEY_NONE) {
            int idx = -1;
            for (i = 0; i < nlane; i++) if (PKEYS[i] == tolower(k)) idx = i;
            if (idx >= 0) {
                if (lane[idx] >= 6) { score++; beep(PFREQ[idx], 120); lane[idx] = -1; }
                else misses++;
            }
        } else if (mode == 6 && k >= '1' && k <= '4') {
            int row = k - '1';
            drum_pat[row][tick % 16] = !drum_pat[row][tick % 16];
        } else if (mode == 9 && k == ' ') {
            if (tick % 4 <= 1 || tick % 4 == 3) score++; else misses++;
        } else if (mode == 10 && k != KEY_NONE) {
            int idx = piano_index(k);
            if (idx >= 0) { beep(PFREQ[idx], 150); score++; }
        }
        sleep_ms(period / 4);
    }
    score_report(p->title ? p->title : "piano", score);
    pause_msg("Press any key...");
}

void fam_piano(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    switch (v) {
        case 2: case 3: case 8: piano_quiz(p, v);   break;
        case 4: case 5:         piano_build(p, v);  break;
        case 7:                 piano_memory(p);    break;
        case 1: case 6: case 9: case 10: piano_timing(p, v); break;
        default:                piano_free(p);      break;
    }
}
