/* GIC:PARAMETERISED memory
 * memory.c - concentration with a configurable number of pairs.
 * params: count = pairs (6..32); the grid shape is derived from it.
 */
#include "engine.h"
#include "games.h"

#define MAXPAIRS 32
#define MAXCELL (MAXPAIRS * 2)

static int PAIRS, ROWS, COLS;
static int card[MAXCELL], found[MAXCELL];

static const char *SYM[MAXPAIRS] = {
 "A","B","C","D","E","F","G","H","J","K","L","M","N","P","R","S",
 "T","U","V","W","X","Y","Z","2","3","4","5","6","7","8","9","0"};
static const char *SCOL[8] = {C_RED, C_GREEN, C_YELLOW, C_BLUE,
                              C_MAGENTA, C_CYAN, C_WHITE, C_GREEN};

static void shape(void)
{
    int total = PAIRS * 2, c;
    for (c = 8; c >= 4; c--)
        if (total % c == 0) { COLS = c; ROWS = total / c; return; }
    COLS = 8;
    ROWS = (total + 7) / 8;
}

static void render(int cr, int cc, int a, int b, int turns, const char *msg)
{
    int r, c, left = 40 - COLS * 2;
    char sub[80];
    snprintf(sub, sizeof sub, "%d pairs - arrows move, Enter flips, Q quits", PAIRS);
    draw_title("MEMORY MATCH", sub);
    for (r = 0; r < ROWS; r++) {
        scr_move(6 + r * 2, left);
        for (c = 0; c < COLS; c++) {
            int idx = r * COLS + c;
            int sel = (r == cr && c == cc);
            if (idx >= PAIRS * 2) { printf("    "); continue; }
            if (found[idx] || idx == a || idx == b)
                printf("%s%s %s %s", sel ? C_REV : "", SCOL[card[idx] % 8],
                       SYM[card[idx]], C_RESET);
            else
                printf("%s%s . %s", sel ? C_REV : "", C_GREY, C_RESET);
        }
    }
    draw_textf(6 + ROWS * 2 + 1, left, "Turns: %d    ", turns);
    draw_text(6 + ROWS * 2 + 3, 20, "                                          ");
    if (msg) draw_textf(6 + ROWS * 2 + 3, left, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_memory(const GParams *p)
{
    PAIRS = gp_int(p->count, 12);
    if (PAIRS < 4) PAIRS = 4;
    if (PAIRS > MAXPAIRS) PAIRS = MAXPAIRS;
    shape();

    for (;;) {
        int deck[MAXCELL], i, cr = 0, cc = 0, turns = 0, matched = 0;
        for (i = 0; i < PAIRS * 2; i++) deck[i] = i / 2;
        shuffle_int(deck, PAIRS * 2);
        for (i = 0; i < PAIRS * 2; i++) card[i] = deck[i];
        memset(found, 0, sizeof found);

        while (matched < PAIRS) {
            int first = -1, second = -1, k;
            while (first < 0 || second < 0) {
                render(cr, cc, first, second, turns, NULL);
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                if (k == KEY_UP    && cr > 0)        cr--;
                if (k == KEY_DOWN  && cr < ROWS - 1) cr++;
                if (k == KEY_LEFT  && cc > 0)        cc--;
                if (k == KEY_RIGHT && cc < COLS - 1) cc++;
                if (k != KEY_ENTER && k != ' ') continue;
                {
                    int idx = cr * COLS + cc;
                    if (idx >= PAIRS * 2 || found[idx] || first == idx) continue;
                    if (first < 0) first = idx; else second = idx;
                }
            }
            turns++;
            render(cr, cc, first, second, turns, NULL);
            sleep_ms(650);
            if (card[first] == card[second]) {
                found[first] = found[second] = 1;
                matched++;
            }
        }
        {
            char m[64];
            snprintf(m, sizeof m, "All %d pairs found in %d turns.", PAIRS, turns);
            render(cr, cc, -1, -1, turns, m);
            score_report(p->title ? p->title : "memory", PAIRS * 200 / turns);
        }
        if (!confirm("\n  Play again?")) return;
    }
}
