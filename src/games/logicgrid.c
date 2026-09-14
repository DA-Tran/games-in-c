/* GIC:PARAMETERISED logicgrid
 * logicgrid.c - the logic-grid deduction family.
 *
 * params: theme (or variant) selects the cast, difficulty sets the size
 *
 * A solved assignment is generated first, then clues are emitted that are all
 * true of it. Because the clues are derived from a known solution the puzzle
 * is always consistent; the player deduces the same mapping by elimination.
 *
 * Difficulty is the grid size (4 or 5 per category), which changes the
 * deduction depth rather than just the reading time.
 */
#include "engine.h"
#include <stdarg.h>
#include "games.h"

#define MAXN 5
#define NCAT 3
#define MAXCLUE 14

typedef struct {
    const char *name;
    const char *label[NCAT];
    const char *item[NCAT][MAXN];
} Theme;

static const Theme THEMES[10] = {
 {"Detective", {"Suspect","Weapon","Room"},
  {{"Green","Scarlet","Plum","Peacock","Mustard"},
   {"rope","candlestick","spanner","dagger","pistol"},
   {"library","kitchen","study","cellar","ballroom"}}},
 {"Dinner Party", {"Guest","Dish","Drink"},
  {{"Aisha","Boris","Clara","Dmitri","Elena"},
   {"risotto","curry","paella","ramen","tagine"},
   {"water","cider","wine","juice","tea"}}},
 {"Race Results", {"Runner","Place","Colour"},
  {{"Nadia","Omar","Petra","Quinn","Rosa"},
   {"first","second","third","fourth","fifth"},
   {"red","blue","green","yellow","white"}}},
 {"Office", {"Worker","Desk","Project"},
  {{"Fern","Gus","Hana","Ivo","Jules"},
   {"window","corner","corridor","atrium","mezzanine"},
   {"audit","launch","migration","rebrand","rollout"}}},
 {"School", {"Pupil","Subject","Day"},
  {{"Kai","Lena","Mo","Nils","Opal"},
   {"physics","history","music","biology","latin"},
   {"Monday","Tuesday","Wednesday","Thursday","Friday"}}},
 {"Zoo", {"Keeper","Animal","Enclosure"},
  {{"Pia","Rafe","Sasha","Tomas","Uma"},
   {"otter","lemur","tapir","ibis","okapi"},
   {"north","south","east","west","central"}}},
 {"Festival", {"Act","Stage","Slot"},
  {{"Vera","Wes","Xan","Yara","Zeke"},
   {"pyramid","meadow","barn","dome","quarry"},
   {"noon","dusk","midnight","dawn","teatime"}}},
 {"Hotel", {"Guest","Room","Request"},
  {{"Ada","Bram","Cleo","Dev","Esme"},
   {"attic","suite","annexe","tower","garden"},
   {"extra pillows","late supper","an early call","a quiet floor","a sea view"}}},
 {"Voyage", {"Passenger","Port","Cabin"},
  {{"Kalo","Lira","Mikk","Nuri","Oona"},
   {"Bergen","Valletta","Hoorn","Cadiz","Split"},
   {"forward","aft","upper","lower","midships"}}},
 {"Bakery", {"Baker","Bake","Hour"},
  {{"Fritz","Greta","Hugo","Ines","Jonas"},
   {"brioche","sourdough","stollen","baguette","focaccia"},
   {"four","five","six","seven","eight"}}}
};

static int N;
static const Theme *T;
static int truth[NCAT][MAXN];        /* truth[cat][person] = item index */
static int guess[NCAT][MAXN];        /* -1 = unset                      */
static char clues[MAXCLUE][120];
static int nclue;

static void add_clue(const char *fmt, ...)
{
    va_list ap;
    if (nclue >= MAXCLUE) return;
    va_start(ap, fmt);
    vsnprintf(clues[nclue], sizeof clues[0], fmt, ap);
    va_end(ap);
    nclue++;
}

static void build(void)
{
    int cat, i, perm[MAXN], tries;

    for (cat = 0; cat < NCAT; cat++) {
        for (i = 0; i < N; i++) perm[i] = i;
        shuffle_int(perm, N);
        for (i = 0; i < N; i++) truth[cat][i] = perm[i];
    }
    for (cat = 0; cat < NCAT; cat++) for (i = 0; i < N; i++) guess[cat][i] = -1;
    nclue = 0;

    /* Every clue below is a true statement about the generated assignment,
     * so the puzzle cannot be contradictory. */
    for (i = 1; i < NCAT; i++) {
        int person = rnd(N);
        add_clue("%s's %s is the %s.",
                 T->item[0][person], T->label[i], T->item[i][truth[i][person]]);
    }
    for (tries = 0; tries < 40 && nclue < N * 2 + 2; tries++) {
        int kind = rnd(3), a = rnd(N), b = rnd(N), c1 = 1 + rnd(NCAT - 1);
        if (a == b) continue;
        if (kind == 0) {
            add_clue("%s's %s is not the %s.",
                     T->item[0][a], T->label[c1], T->item[c1][truth[c1][b]]);
        } else if (kind == 1) {
            int c2 = 1 + rnd(NCAT - 1);
            if (c1 == c2) continue;
            add_clue("Whoever has the %s (%s) does not have the %s (%s).",
                     T->item[c1][truth[c1][a]], T->label[c1],
                     T->item[c2][truth[c2][b]], T->label[c2]);
        } else {
            add_clue("The %s and the %s belong to the same person.",
                     T->item[1][truth[1][a]], T->item[2][truth[2][a]]);
        }
    }
}

static int solved(void)
{
    int cat, i;
    for (cat = 1; cat < NCAT; cat++)
        for (i = 0; i < N; i++)
            if (guess[cat][i] != truth[cat][i]) return 0;
    return 1;
}

void fam_logicgrid(const GParams *p)
{
    int ti = -1, i, cr = 0, cc = 1;

    for (i = 0; i < 10; i++) if (p->theme && strcmp(p->theme, THEMES[i].name) == 0) ti = i;
    if (ti < 0) ti = gp_int(p->variant, 0) % 10;
    T = &THEMES[ti];
    N = gp_int(p->difficulty, 2) >= 4 ? 5 : 4;

    for (;;) {
        int won = 0;
        build();

        for (;;) {
            int k, r, c;
            char sub[110];
            snprintf(sub, sizeof sub,
                     "%s, %d each — arrows move, 1-%d assigns, 0 clears, Q quits",
                     T->name, N, N);
            draw_title("LOGIC GRID", sub);

            for (c = 1; c < NCAT; c++)
                draw_textf(5, 22 + (c - 1) * 22, "%s%-20s%s", C_YELLOW, T->label[c], C_RESET);
            for (r = 0; r < N; r++) {
                draw_textf(6 + r, 8, "%s%-12s%s", C_WHITE, T->item[0][r], C_RESET);
                for (c = 1; c < NCAT; c++) {
                    int v = guess[c][r];
                    draw_textf(6 + r, 22 + (c - 1) * 22, "%s%s%-18s%s",
                               (r == cr && c == cc) ? BG_BLUE : "",
                               v < 0 ? C_GREY : C_CYAN,
                               v < 0 ? "?" : T->item[c][v], C_RESET);
                }
            }
            draw_textf(7 + N, 8, "%sClues:%s", C_BOLD, C_RESET);
            for (i = 0; i < nclue && i < 10; i++)
                draw_textf(8 + N + i, 8, "%s%-70s%s", C_GREY, clues[i], C_RESET);
            if (won) draw_textf(9 + N + nclue, 8, "%sSolved! Press any key.%s", C_BOLD C_GREEN, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (won) break;
            if (k == KEY_UP    && cr > 0)        cr--;
            if (k == KEY_DOWN  && cr < N - 1)    cr++;
            if (k == KEY_LEFT  && cc > 1)        cc--;
            if (k == KEY_RIGHT && cc < NCAT - 1) cc++;
            if (k == '0') guess[cc][cr] = -1;
            if (k >= '1' && k <= '0' + N) guess[cc][cr] = k - '1';
            if (solved()) {
                won = 1;
                score_report(p->title ? p->title : "logicgrid", N * 200);
            }
        }
        if (!confirm("\n  Another puzzle?")) return;
    }
}
