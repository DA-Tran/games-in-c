/* GIC:PARAMETERISED solitaire
 * solitaire.c - the tableau patience family.
 *
 * params: variant selects the game
 *
 * Eighteen of the twenty-nine patiences in the catalogue are the same machine
 * with different rules: cards are dealt into tableau piles, built downwards by
 * some relation, and shipped to foundations. Rather than eighteen near-copies
 * this file has one engine and a Rules table, so the differences are stated
 * once and are visible side by side:
 *
 *   build      what may sit on what: alternating colour, same suit, or any
 *   cells      free cells, which also set how large a group may move
 *   stock      none, draw one, draw three, or deal a row across the tableau
 *   movegrp    whether a moved group must already be a sequence (Yukon says no)
 *   fill       what may occupy an emptied pile: anything, kings only, nothing
 *   found      foundations built A-K by suit, or complete K-A runs (Spider)
 *
 * The remaining eleven patiences are different machines - card removal and
 * speed games - and live in solitaire2.c.
 */
#include "engine.h"
#include "cards.h"
#include "games.h"

#define MAXT   18       /* La Belle Lucie's fans                      */
#define MAXC    8       /* Eight Off's cells                          */
#define MAXF    8       /* two-deck foundations                       */
#define MAXP  108       /* two decks plus slack                       */

enum { B_ALT, B_SUIT, B_ANY };
enum { S_NONE, S_W1, S_W3, S_ROW };
enum { M_SEQ, M_ANY, M_ONE };
enum { F_ANY, F_KING, F_NONE };
enum { FD_SUIT, FD_SPIDER };

typedef struct {
    const char *name;
    int decks, suits, tpiles, cells;
    int build, stock, movegrp, fill, nfound, fmode, reserve;
} Rules;

/*                      name                 dk su tp ce build   stock   grp    fill    nf fmode      res */
static const Rules RULES[] = {
    {"Klondike Draw One",     1, 4,  7, 0, B_ALT,  S_W1,   M_SEQ, F_KING,  4, FD_SUIT,    0},
    {"Klondike Draw Three",   1, 4,  7, 0, B_ALT,  S_W3,   M_SEQ, F_KING,  4, FD_SUIT,    0},
    {"Spider One Suit",       2, 1, 10, 0, B_SUIT, S_ROW,  M_SEQ, F_ANY,   8, FD_SPIDER,  0},
    {"Spider Two Suit",       2, 2, 10, 0, B_SUIT, S_ROW,  M_SEQ, F_ANY,   8, FD_SPIDER,  0},
    {"Spider Four Suit",      2, 4, 10, 0, B_SUIT, S_ROW,  M_SEQ, F_ANY,   8, FD_SPIDER,  0},
    {"FreeCell",              1, 4,  8, 4, B_ALT,  S_NONE, M_SEQ, F_ANY,   4, FD_SUIT,    0},
    {"Eight Off",             1, 4,  8, 8, B_SUIT, S_NONE, M_SEQ, F_KING,  4, FD_SUIT,    0},
    {"Seahaven Towers",       1, 4, 10, 4, B_SUIT, S_NONE, M_SEQ, F_KING,  4, FD_SUIT,    0},
    {"Penguin",               1, 4,  7, 7, B_SUIT, S_NONE, M_SEQ, F_ANY,   4, FD_SUIT,    0},
    {"Yukon",                 1, 4,  7, 0, B_ALT,  S_NONE, M_ANY, F_KING,  4, FD_SUIT,    0},
    {"Russian Solitaire",     1, 4,  7, 0, B_SUIT, S_NONE, M_ANY, F_KING,  4, FD_SUIT,    0},
    {"Canfield",              1, 4,  4, 0, B_ALT,  S_W3,   M_SEQ, F_ANY,   4, FD_SUIT,   13},
    {"Scorpion",              1, 4,  7, 0, B_SUIT, S_NONE, M_ANY, F_KING,  4, FD_SPIDER,  0},
    {"Forty Thieves",         2, 4, 10, 0, B_SUIT, S_W1,   M_ONE, F_ANY,   8, FD_SUIT,    0},
    {"Baker's Dozen",         1, 4, 13, 0, B_ANY,  S_NONE, M_ONE, F_NONE,  4, FD_SUIT,    0},
    {"Beleaguered Castle",    1, 4,  8, 0, B_ANY,  S_NONE, M_ONE, F_ANY,   4, FD_SUIT,    0},
    {"La Belle Lucie",        1, 4, 18, 0, B_SUIT, S_NONE, M_ONE, F_NONE,  4, FD_SUIT,    0},
    {"Napoleon at St Helena", 2, 4, 10, 0, B_SUIT, S_W1,   M_ONE, F_ANY,   8, FD_SUIT,    0}
};
#define NRULES ((int)(sizeof RULES / sizeof RULES[0]))

/* Catalogue variant -> index into RULES. -1 means "not a tableau game"; those
 * are handled by solitaire2.c. */
static const int VMAP[29] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8, -1,
    -1, -1,  9, 10, 11, 12, 13, -1, -1, 14,
    15, -1, 16, -1, 17, -1, -1, -1, -1
};

static const Rules *R;

static int tab[MAXT][MAXP], tn[MAXT], tup[MAXT];  /* tup = face-up count   */
static int cell[MAXC], ncellused;
static int found[MAXF][14], fn[MAXF];
static int stock[MAXP], sn, waste[MAXP], wn, redeals;
static int reserve[MAXP], rn;
static int spider_done;
static long moves;

static int rank_of(int c) { return c % 13; }
static int suit_of(int c) { return (c / 13) % 4; }
static int red_of(int c)  { int s = suit_of(c); return s == 1 || s == 2; }

/* ------------------------------------------------------------------ rules */

static int can_stack(int lower, int upper)     /* may `lower` sit on `upper`? */
{
    if (rank_of(upper) != rank_of(lower) + 1) return 0;
    if (R->build == B_ALT)  return red_of(upper) != red_of(lower);
    if (R->build == B_SUIT) return suit_of(upper) == suit_of(lower);
    return 1;                                            /* B_ANY */
}

static int may_fill_empty(int card)
{
    if (R->fill == F_NONE) return 0;
    if (R->fill == F_KING) return rank_of(card) == 12;
    return 1;
}

/* The longest run at the top of pile p that is legal to lift as a unit. */
static int liftable(int p)
{
    int i, n = tn[p], up = tup[p];
    if (n == 0) return 0;
    if (R->movegrp == M_ONE) return 1;
    if (R->movegrp == M_ANY) return up;      /* Yukon: any face-up group */
    for (i = n - 1; i > n - up; i--)
        if (!can_stack(tab[p][i], tab[p][i - 1])) break;
    return n - i;
}

/* FreeCell's supermove allowance: a group of k needs k-1 resting places. */
static int move_limit(int to_empty)
{
    int i, free_cells = 0, empties = 0;
    for (i = 0; i < R->cells; i++) if (cell[i] < 0) free_cells++;
    for (i = 0; i < R->tpiles; i++) if (tn[i] == 0) empties++;
    if (to_empty && empties > 0) empties--;
    return (free_cells + 1) * (empties + 1);
}

/* --------------------------------------------------------------- dealing */

static int pile_target(int i)
{
    switch (R - RULES) {
    case 0: case 1: return i + 1;                       /* Klondike staircase */
    case 2: case 3: case 4: return i < 4 ? 6 : 5;       /* Spider             */
    case 5: return i < 4 ? 7 : 6;                       /* FreeCell           */
    case 6: return 6;                                   /* Eight Off          */
    case 7: return 5;                                   /* Seahaven           */
    case 8: return 7;                                   /* Penguin            */
    case 9: case 10: return i == 0 ? 1 : i + 5;         /* Yukon / Russian    */
    case 11: return 1;                                  /* Canfield           */
    case 12: return 7;                                  /* Scorpion           */
    case 13: return 4;                                  /* Forty Thieves      */
    case 14: return 4;                                  /* Baker's Dozen      */
    case 15: return 6;                                  /* Beleaguered Castle */
    case 16: return i < 17 ? 3 : 1;                     /* La Belle Lucie     */
    default: return 4;                                  /* Napoleon           */
    }
}

static int hidden_target(int i)
{
    switch (R - RULES) {
    case 0: case 1: return i;                           /* Klondike           */
    case 2: case 3: case 4: return pile_target(i) - 1;  /* Spider             */
    case 9: case 10: return i == 0 ? 0 : i;             /* Yukon / Russian    */
    case 12: return i < 3 ? 3 : 0;                      /* Scorpion           */
    default: return 0;                                  /* everything visible */
    }
}

static void deal(void)
{
    int deck[MAXP], nd = 0, i, j, k, d, pos = 0;

    /* Reduced-suit Spider uses repeated copies of the same suits. */
    for (d = 0; d < R->decks; d++)
        for (i = 0; i < 4; i++)
            for (j = 0; j < 13; j++)
                deck[nd++] = (i % R->suits) * 13 + j;
    shuffle_int(deck, nd);

    memset(tn, 0, sizeof tn);
    memset(tup, 0, sizeof tup);
    memset(fn, 0, sizeof fn);
    for (i = 0; i < MAXC; i++) cell[i] = -1;
    ncellused = 0; sn = wn = rn = 0; redeals = 0; spider_done = 0; moves = 0;

    for (i = 0; i < R->reserve; i++) reserve[rn++] = deck[pos++];

    for (i = 0; i < R->tpiles; i++) {
        int want = pile_target(i), hide = hidden_target(i);
        for (k = 0; k < want && pos < nd; k++) tab[i][tn[i]++] = deck[pos++];
        tup[i] = tn[i] - hide;
        if (tup[i] < 1 && tn[i] > 0) tup[i] = 1;
    }

    /* Eight Off deals four cards straight into cells; Seahaven deals two. */
    if (R - RULES == 6) for (i = 0; i < 4 && pos < nd; i++) { cell[i] = deck[pos++]; ncellused++; }
    if (R - RULES == 7) for (i = 0; i < 2 && pos < nd; i++) { cell[i] = deck[pos++]; ncellused++; }
    /* Penguin's three spare cards go to cells as well. */
    if (R - RULES == 8) for (i = 0; i < 3 && pos < nd; i++) { cell[i] = deck[pos++]; ncellused++; }

    while (pos < nd) stock[sn++] = deck[pos++];

    /* Beleaguered Castle starts with its aces already up. */
    if (R - RULES == 15) {
        for (i = 0; i < R->tpiles; i++)
            for (j = 0; j < tn[i]; j++)
                if (rank_of(tab[i][j]) == 0) {
                    int s = suit_of(tab[i][j]);
                    found[s][fn[s]++] = tab[i][j];
                    for (k = j; k < tn[i] - 1; k++) tab[i][k] = tab[i][k + 1];
                    tn[i]--; tup[i]--; j--;
                }
        for (i = 0; i < R->tpiles; i++) if (tup[i] < tn[i]) tup[i] = tn[i];
    }
    /* Baker's Dozen sinks kings to the bottom of their pile. */
    if (R - RULES == 14) {
        for (i = 0; i < R->tpiles; i++)
            for (j = tn[i] - 1; j > 0; j--)
                if (rank_of(tab[i][j]) == 12) {
                    int c = tab[i][j];
                    for (k = j; k > 0; k--) tab[i][k] = tab[i][k - 1];
                    tab[i][0] = c;
                }
    }
    /* Scorpion's last three cards land on the first three piles. */
    if (R - RULES == 12)
        for (i = 0; i < 3 && sn > 0; i++) { tab[i][tn[i]++] = stock[--sn]; tup[i]++; }

    for (i = 0; i < R->tpiles; i++) if (tup[i] > tn[i]) tup[i] = tn[i];
}

/* ---------------------------------------------------------- foundations */

static int found_accepts(int f, int card)
{
    if (R->fmode != FD_SUIT) return 0;
    if (fn[f] == 0) return suit_of(card) == f % 4 && rank_of(card) == 0;
    return suit_of(card) == suit_of(found[f][0]) &&
           rank_of(card) == rank_of(found[f][fn[f] - 1]) + 1;
}

/* Spider and Scorpion have no foundation piles to drop onto: a complete
 * king-to-ace run inside the tableau lifts itself out. */
static void harvest_runs(void)
{
    int p, i;
    if (R->fmode != FD_SPIDER) return;
    for (p = 0; p < R->tpiles; p++) {
        int n = tn[p];
        if (n < 13 || tup[p] < 13) continue;
        if (rank_of(tab[p][n - 1]) != 0 || rank_of(tab[p][n - 13]) != 12) continue;
        for (i = n - 12; i < n; i++)
            if (!can_stack(tab[p][i], tab[p][i - 1])) break;
        if (i != n) continue;
        tn[p] -= 13;
        tup[p] -= 13;
        if (tup[p] < 1 && tn[p] > 0) tup[p] = 1;
        spider_done++;
    }
}

static int won(void)
{
    int f, total = 0;
    if (R->fmode == FD_SPIDER) return spider_done >= R->nfound;
    for (f = 0; f < R->nfound; f++) total += fn[f];
    return total >= 52 * R->decks;
}

/* Send whatever will obviously go, repeatedly, until nothing moves. */
static int autoplay(void)
{
    int done = 0, again = 1, p, f, i;
    if (R->fmode == FD_SPIDER) { harvest_runs(); return 0; }
    while (again) {
        again = 0;
        for (p = 0; p < R->tpiles; p++) {
            if (tn[p] == 0) continue;
            for (f = 0; f < R->nfound; f++)
                if (found_accepts(f, tab[p][tn[p] - 1])) {
                    found[f][fn[f]++] = tab[p][--tn[p]];
                    if (tup[p] > tn[p]) tup[p] = tn[p];
                    if (tup[p] == 0 && tn[p] > 0) tup[p] = 1;
                    again = 1; done++; moves++;
                    break;
                }
        }
        for (i = 0; i < R->cells; i++) {
            if (cell[i] < 0) continue;
            for (f = 0; f < R->nfound; f++)
                if (found_accepts(f, cell[i])) {
                    found[f][fn[f]++] = cell[i];
                    cell[i] = -1; ncellused--;
                    again = 1; done++; moves++;
                    break;
                }
        }
        if (wn > 0)
            for (f = 0; f < R->nfound; f++)
                if (found_accepts(f, waste[wn - 1])) {
                    found[f][fn[f]++] = waste[--wn];
                    again = 1; done++; moves++;
                    break;
                }
    }
    return done;
}

/* ------------------------------------------------------------- the stock */

static const char *draw_stock(void)
{
    int i, n;
    if (R->stock == S_NONE) return "This game has no stock.";
    if (R->stock == S_ROW) {
        for (i = 0; i < R->tpiles; i++) if (tn[i] == 0) return "Fill every empty pile first.";
        if (sn == 0) return "The stock is empty.";
        for (i = 0; i < R->tpiles && sn > 0; i++) {
            tab[i][tn[i]++] = stock[--sn];
            tup[i]++;
        }
        harvest_runs();
        moves++;
        return NULL;
    }
    if (sn == 0) {
        if (wn == 0) return "Stock and waste are both empty.";
        if (R->stock == S_W1 && redeals >= 2) return "No redeals left.";
        while (wn > 0) stock[sn++] = waste[--wn];
        redeals++;
        return NULL;
    }
    n = (R->stock == S_W3) ? 3 : 1;
    for (i = 0; i < n && sn > 0; i++) waste[wn++] = stock[--sn];
    moves++;
    return NULL;
}

/* ------------------------------------------------------------ the moving */

/* Slots are addressed uniformly: 0..cells-1 are free cells, then the
 * foundations, then the stock/waste pair, then the tableau piles. */
static int slot_cells(void)  { return R->cells; }
static int slot_found(void)  { return slot_cells() + R->nfound; }
static int slot_stock(void)  { return slot_found() + (R->stock == S_NONE ? 0 : 1); }
static int slot_res(void)    { return slot_stock() + (R->reserve ? 1 : 0); }
static int nslots(void)      { return slot_res() + R->tpiles; }

static int top_of(int s)
{
    if (s < slot_cells()) return cell[s];
    if (s < slot_found()) { int f = s - slot_cells(); return fn[f] ? found[f][fn[f] - 1] : -1; }
    if (s < slot_stock()) return wn ? waste[wn - 1] : -1;
    if (s < slot_res())   return rn ? reserve[rn - 1] : -1;
    { int p = s - slot_res(); return tn[p] ? tab[p][tn[p] - 1] : -1; }
}

/* Take `want` cards off slot `s` into buf; returns how many, without
 * committing - the caller calls commit_take() once the move is known legal. */
static int peek_take(int s, int want, int *buf)
{
    int c, p, i, avail;
    if (s < slot_cells()) { c = cell[s]; if (c < 0) return 0; buf[0] = c; return 1; }
    if (s < slot_found()) {
        int f = s - slot_cells();
        if (!fn[f]) return 0;
        buf[0] = found[f][fn[f] - 1];
        return 1;
    }
    if (s < slot_stock()) { if (!wn) return 0; buf[0] = waste[wn - 1]; return 1; }
    if (s < slot_res())   { if (!rn) return 0; buf[0] = reserve[rn - 1]; return 1; }
    p = s - slot_res();
    avail = liftable(p);
    if (avail == 0) return 0;
    if (want > 0 && want < avail) avail = want;
    for (i = 0; i < avail; i++) buf[i] = tab[p][tn[p] - avail + i];
    return avail;
}

static void commit_take(int s, int k)
{
    if (s < slot_cells()) { cell[s] = -1; ncellused--; return; }
    if (s < slot_found()) { fn[s - slot_cells()]--; return; }
    if (s < slot_stock()) { wn--; return; }
    if (s < slot_res())   { rn--; return; }
    {
        int p = s - slot_res();
        tn[p] -= k;
        tup[p] -= k;
        if (tup[p] < 1 && tn[p] > 0) tup[p] = 1;   /* turn the new top card */
    }
}

static const char *try_move(int from, int to, int want)
{
    int buf[MAXP], k, i;
    if (from == to) return NULL;

    k = peek_take(from, want, buf);
    if (k == 0) return "Nothing to move from there.";

    if (to < slot_cells()) {
        if (k != 1) { k = peek_take(from, 1, buf); if (k != 1) return "Only one card fits a cell."; }
        if (cell[to] >= 0) return "That cell is occupied.";
        commit_take(from, 1);
        cell[to] = buf[0]; ncellused++;
        moves++; harvest_runs(); return NULL;
    }
    if (to < slot_found()) {
        int f = to - slot_cells();
        if (R->fmode == FD_SPIDER) return "Complete a king-to-ace run in the tableau instead.";
        if (k != 1) { k = peek_take(from, 1, buf); if (k != 1) return "One card at a time."; }
        if (!found_accepts(f, buf[0])) return "That card will not go up there.";
        commit_take(from, 1);
        found[f][fn[f]++] = buf[0];
        moves++; return NULL;
    }
    if (to < slot_res()) return "You cannot move cards onto the stock.";

    {
        int p = to - slot_res();
        int limit;
        /* Take the largest legal group we can: shrink until it fits. */
        while (k > 0) {
            int ok = 1;
            if (tn[p] == 0) { if (!may_fill_empty(buf[0])) ok = 0; }
            else if (!can_stack(buf[0], tab[p][tn[p] - 1])) ok = 0;
            limit = move_limit(tn[p] == 0);
            if (ok && k > limit && R->movegrp == M_SEQ) ok = 0;
            if (ok) break;
            k--;
            if (k > 0) k = peek_take(from, k, buf);
        }
        if (k == 0) {
            if (tn[p] == 0) return R->fill == F_NONE ? "Empty piles stay empty in this game."
                                                     : "Only a king may start an empty pile.";
            return "That card will not sit there.";
        }
        commit_take(from, k);
        for (i = 0; i < k; i++) tab[p][tn[p]++] = buf[i];
        tup[p] += k;
        if (tup[p] > tn[p]) tup[p] = tn[p];
        moves++;
        harvest_runs();
        return NULL;
    }
}

/* ------------------------------------------------------------- rendering */

static void draw_slot(int row, int col, int s, int sel, int cur)
{
    char nm[8];
    int c = top_of(s);
    const char *bg = cur ? BG_BLUE : (sel ? BG_GREEN : "");
    if (c < 0) {
        draw_textf(row, col, "%s%s[  ]%s", bg, C_GREY, C_RESET);
        return;
    }
    card_name(c, nm, sizeof nm);
    draw_textf(row, col, "%s%s%-4s%s", bg, suit_colour(c), nm, C_RESET);
}

static void render(int cur, int sel, int nsel, const char *msg)
{
    char sub[120], nm[8];
    int i, p, row, maxrows = 0;

    snprintf(sub, sizeof sub, "%s — arrows move, Enter picks up and drops, "
             "D draws, A auto-plays, N new deal, Q quits", R->name);
    draw_title("PATIENCE", sub);

    row = 4;
    for (i = 0; i < R->cells; i++)
        draw_slot(row, 6 + i * 5, i, sel == i, cur == i);
    for (i = 0; i < R->nfound; i++) {
        int s = slot_cells() + i;
        if (R->fmode == FD_SPIDER)
            draw_textf(row, 50 + i * 4, "%s%s%s", i < spider_done ? C_GREEN : C_GREY,
                       i < spider_done ? "[##]" : "[  ]", C_RESET);
        else
            draw_slot(row, 50 + i * 4, s, sel == s, cur == s);
    }
    if (R->stock != S_NONE && R->stock != S_ROW) {
        int s = slot_found();
        draw_textf(row + 1, 6, "%sstock %-3d%s", C_GREY, sn, C_RESET);
        draw_slot(row + 1, 17, s, sel == s, cur == s);
    } else if (R->stock == S_ROW) {
        draw_textf(row + 1, 6, "%sstock %-3d (deals a row)%s", C_GREY, sn, C_RESET);
    }
    if (R->reserve) {
        int s = slot_stock();
        draw_textf(row + 1, 30, "%sreserve %-3d%s", C_GREY, rn, C_RESET);
        draw_slot(row + 1, 44, s, sel == s, cur == s);
    }

    row = 7;
    for (p = 0; p < R->tpiles; p++) if (tn[p] > maxrows) maxrows = tn[p];
    for (p = 0; p < R->tpiles; p++) {
        int s = slot_res() + p, col = 4 + p * 5;
        int lift = (sel == s) ? nsel : 0;
        draw_textf(row, col, "%s%s %c %s", cur == s ? BG_BLUE : "", C_BOLD,
                   p < 9 ? '1' + p : 'a' + p - 9, C_RESET);
        if (tn[p] == 0) {
            draw_textf(row + 1, col, "%s%s[  ]%s", cur == s ? BG_BLUE : "", C_GREY, C_RESET);
            continue;
        }
        for (i = 0; i < tn[p] && i < 20; i++) {
            int c = tab[p][i], down = i < tn[p] - tup[p];
            int inlift = lift && i >= tn[p] - lift;
            if (down) { draw_textf(row + 1 + i, col, "%s##  %s", C_GREY, C_RESET); continue; }
            card_name(c, nm, sizeof nm);
            draw_textf(row + 1 + i, col, "%s%s%-4s%s", inlift ? BG_GREEN : "",
                       suit_colour(c), nm, C_RESET);
        }
        if (tn[p] > 20) draw_textf(row + 21, col, "%s+%-3d%s", C_GREY, tn[p] - 20, C_RESET);
    }

    draw_textf(row + (maxrows > 20 ? 23 : maxrows + 2), 4,
               "%sMoves %-5ld%s  %s%-62s%s", C_GREY, moves, C_RESET,
               C_YELLOW, msg ? msg : "", C_RESET);
    scr_flush();
}

/* ------------------------------------------------------------------ main */

static int solitaire_tableau(const GParams *p, int ri)
{
    int cur = 0, sel = -1, nsel = 0, want = 0;
    const char *msg = NULL;

    R = &RULES[ri];
    deal();

    for (;;) {
        int k;
        if (won()) {
            render(cur, -1, 0, "Solved!");
            score_report(p->title ? p->title : "solitaire",
                         (int)(10000 - (moves > 9000 ? 9000 : moves)));
            if (!confirm("\n  Solved! Deal another?")) return 0;
            deal();
            cur = 0; sel = -1; msg = NULL;
            continue;
        }
        render(cur, sel, nsel, msg);
        msg = NULL;

        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        if (k == KEY_LEFT)  { cur = (cur + nslots() - 1) % nslots(); continue; }
        if (k == KEY_RIGHT) { cur = (cur + 1) % nslots(); continue; }
        if (k == KEY_UP)    { cur = cur >= slot_res() ? 0 : slot_res(); continue; }
        if (k == KEY_DOWN)  { cur = cur < slot_res() ? slot_res() : 0; continue; }
        if (k >= '1' && k <= '9') { want = k - '0'; msg = "Group size set."; continue; }
        if (k == 'd' || k == 'D' || k == ' ') { msg = draw_stock(); continue; }
        if (k == 'a' || k == 'A') {
            int n = autoplay();
            msg = n ? NULL : "Nothing to send up.";
            continue;
        }
        if (k == 'n' || k == 'N') { deal(); sel = -1; cur = 0; continue; }
        if (k == KEY_ENTER) {
            if (sel < 0) {
                int buf[MAXP];
                nsel = peek_take(cur, want, buf);
                if (nsel == 0) { msg = "Nothing to pick up there."; continue; }
                sel = cur;
            } else {
                msg = try_move(sel, cur, want);
                sel = -1; nsel = 0; want = 0;
            }
            continue;
        }
        if (k == 'x' || k == 'X') { sel = -1; nsel = 0; want = 0; }
    }
    score_report(p->title ? p->title : "solitaire", (int)moves);
    return 1;
}

void fam_solitaire(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    if (v < 0 || v > 28) v = 0;
    if (VMAP[v] >= 0) { solitaire_tableau(p, VMAP[v]); return; }
    solitaire_other(p, v);
}
