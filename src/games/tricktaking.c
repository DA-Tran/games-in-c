/* GIC:PARAMETERISED tricktaking
 * tricktaking.c - the trick-taking family.
 *
 * params: variant selects the game
 *
 * Ten of the twelve entries are one engine: deal a hand, lead a card, everyone
 * must follow suit if they can, highest card of the led suit wins unless
 * somebody trumped. What separates Hearts from Spades from Briscola is data,
 * so it is stored as data:
 *
 *   deck      52, 40 (Italian, no 8-9-10) or 24 (Euchre) cards
 *   trump     none, a fixed suit, the turn-up, or the winner of a bid
 *   score     tricks taken, card points, or points avoided
 *   bid       none, "how many tricks", or "exactly how many"
 *
 * Sevens and Scopa are not trick games at all - one is a layout game and the
 * other is a capture game - so each has its own loop at the end.
 */
#include "engine.h"
#include "cards.h"
#include "games.h"

#define NP     4
#define MAXH  13

enum { T_NONE, T_FIXED, T_TURNUP, T_BID };
enum { S_TRICKS, S_POINTS, S_AVOID };
enum { B_NONE, B_TRICKS, B_EXACT };

typedef struct {
    const char *name;
    int players, hand, decksize, trump, trumpsuit, score, bid, target;
} TRules;

/*                     name             pl  hand deck trump     tsuit score     bid       target */
static const TRules TR[12] = {
    {"Hearts",           4, 13, 52, T_NONE,   0, S_AVOID,  B_NONE,   50},
    {"Spades",           4, 13, 52, T_FIXED,  0, S_TRICKS, B_TRICKS, 200},
    {"Whist",            4, 13, 52, T_TURNUP, 0, S_TRICKS, B_NONE,   5},
    {"Oh Hell",          4,  7, 52, T_TURNUP, 0, S_TRICKS, B_EXACT,  50},
    {"Euchre",           4,  5, 24, T_TURNUP, 0, S_TRICKS, B_NONE,   10},
    {"Briscola",         2,  3, 40, T_TURNUP, 0, S_POINTS, B_NONE,   61},
    {"Napoleon",         4,  5, 52, T_BID,    0, S_TRICKS, B_TRICKS, 20},
    {"Sevens",           4, 13, 52, T_NONE,   0, S_TRICKS, B_NONE,   0},
    {"Knockout Whist",   4,  7, 52, T_TURNUP, 0, S_TRICKS, B_NONE,   1},
    {"German Whist",     2, 13, 52, T_TURNUP, 0, S_TRICKS, B_NONE,   7},
    {"Ninety-Nine",      3,  9, 52, T_NONE,   0, S_TRICKS, B_EXACT,  0},
    {"Scopa",            2,  3, 40, T_NONE,   0, S_POINTS, B_NONE,   11}
};

static const TRules *R;
static int hand[NP][MAXH], hn[NP];
static int trump;                       /* -1 = no trump */
static int tricks[NP], points[NP], total[NP], bidv[NP];

static int rank_of(int c) { return c % 13; }
static int suit_of(int c) { return c / 13; }

/* Briscola and Scopa use the Italian forty: ace, 2-7, then the three courts. */
static int in_deck(int c)
{
    int r = rank_of(c);
    if (R->decksize == 40) return !(r >= 7 && r <= 9);
    if (R->decksize == 24) return r == 0 || r >= 8;      /* 9, 10, J, Q, K, A */
    return 1;
}

static void build_deck(int *deck, int *n)
{
    int c;
    *n = 0;
    for (c = 0; c < 52; c++) if (in_deck(c)) deck[(*n)++] = c;
    shuffle_int(deck, *n);
}

/* Briscola's card points: ace 11, three 10, then the courts. */
static int card_points(int c)
{
    switch (rank_of(c)) {
    case 0:  return 11;      /* ace   */
    case 2:  return 10;      /* three */
    case 12: return 4;       /* king  */
    case 11: return 3;       /* queen */
    case 10: return 2;       /* jack  */
    default: return 0;
    }
}

/* Hearts: each heart costs one, the queen of spades costs thirteen. */
static int penalty(int c)
{
    if (suit_of(c) == 1) return 1;
    if (c == 0 * 13 + 11) return 13;
    return 0;
}

/* In Euchre the jack of trumps and its same-colour partner outrank the ace. */
static int euchre_value(int c, int led)
{
    int s = suit_of(c), r = rank_of(c);
    int samecolour = (trump == 0 ? 3 : trump == 3 ? 0 : trump == 1 ? 2 : 1);
    if (r == 10 && s == trump)      return 100;         /* right bower */
    if (r == 10 && s == samecolour) return 99;          /* left bower  */
    if (s == trump) return 50 + r;
    if (s == led)   return r;
    return -1;
}

static int card_value(int c, int led)
{
    if (R - TR == 4) return euchre_value(c, led);
    if (suit_of(c) == trump && trump >= 0) return 50 + rank_of(c);
    if (suit_of(c) == led) return rank_of(c) == 0 ? 20 : rank_of(c);
    return -1;
}

static int has_suit(int pl, int s)
{
    int i;
    for (i = 0; i < hn[pl]; i++) if (suit_of(hand[pl][i]) == s) return 1;
    return 0;
}

static int legal(int pl, int idx, int led, int trickno)
{
    int c = hand[pl][idx];
    if (led < 0) {
        /* Hearts cannot be led until one has been discarded, and the first
         * trick never opens with a penalty card. */
        if (R->score == S_AVOID && trickno == 0 && penalty(c)) return 0;
        return 1;
    }
    if (has_suit(pl, led) && suit_of(c) != led) return 0;
    if (R->score == S_AVOID && trickno == 0 && penalty(c) && has_suit(pl, led) == 0) {
        int i, only = 1;
        for (i = 0; i < hn[pl]; i++) if (!penalty(hand[pl][i])) only = 0;
        if (!only) return 0;
    }
    return 1;
}

/* The opponents play a simple but coherent game: win the trick cheaply when
 * winning is good, otherwise throw the least useful card. */
static int choose_card(int pl, int led, int trickno, int bestval)
{
    int i, pickidx = -1, pickval = -1;
    int wantwin = (R->score != S_AVOID);

    if (R->bid == B_EXACT)
        wantwin = tricks[pl] < bidv[pl];

    for (i = 0; i < hn[pl]; i++) {
        int v;
        if (!legal(pl, i, led, trickno)) continue;
        v = card_value(hand[pl][i], led < 0 ? suit_of(hand[pl][i]) : led);
        if (pickidx < 0) { pickidx = i; pickval = v; continue; }
        if (wantwin) {
            /* cheapest winner, else cheapest card */
            int beats  = v > bestval, pbeats = pickval > bestval;
            if (beats && (!pbeats || v < pickval)) { pickidx = i; pickval = v; }
            else if (!beats && !pbeats && v < pickval) { pickidx = i; pickval = v; }
        } else {
            int loses  = v < bestval || v < 0, ploses = pickval < bestval || pickval < 0;
            if (loses && (!ploses || v > pickval)) { pickidx = i; pickval = v; }
            else if (!loses && !ploses && v < pickval) { pickidx = i; pickval = v; }
        }
    }
    if (pickidx < 0) pickidx = 0;
    return pickidx;
}

static void remove_card(int pl, int idx)
{
    int i;
    for (i = idx; i < hn[pl] - 1; i++) hand[pl][i] = hand[pl][i + 1];
    hn[pl]--;
}

static void draw_hand(int row, int cur, int led, int trickno)
{
    int i;
    char nm[8];
    for (i = 0; i < hn[0]; i++) {
        int ok = legal(0, i, led, trickno);
        card_name(hand[0][i], nm, sizeof nm);
        draw_textf(row, 6 + i * 5, "%s%s%-4s%s", i == cur ? BG_BLUE : "",
                   ok ? suit_colour(hand[0][i]) : C_GREY, nm, C_RESET);
    }
}

static void play_tricks(const GParams *p)
{
    int deck[52], nd, i, pl, cur = 0, hs;
    char nm[8];

    for (;;) {
        int leader = 0, done = 0;
        for (i = 0; i < R->players; i++) { total[i] = 0; bidv[i] = 0; }

        while (!done) {
            int trickno, upcard = -1;
            build_deck(deck, &nd);
            hs = R->hand;
            if (hs * R->players > nd) hs = nd / R->players;
            for (pl = 0; pl < R->players; pl++) {
                hn[pl] = 0;
                for (i = 0; i < hs; i++) hand[pl][hn[pl]++] = deck[pl * hs + i];
                tricks[pl] = 0; points[pl] = 0;
            }
            if (R->trump == T_FIXED)  trump = R->trumpsuit;
            else if (R->trump == T_TURNUP) {
                /* Whist deals the whole pack, so there is no spare card to
                 * turn - and reading one ran off the end of the deck. The
                 * real rule is that the dealer exposes their own last card,
                 * which is exactly the last one dealt. */
                int up = R->players * hs;
                if (up >= nd) up = nd - 1;
                upcard = deck[up];
                trump = suit_of(upcard);
            }
            else if (R->trump == T_BID)    trump = rnd(4);
            else trump = -1;

            /* Bidding, where the game has any. */
            if (R->bid != B_NONE) {
                int mybid = hs / 3;
                for (;;) {
                    int k;
                    char sub[140];
                    snprintf(sub, sizeof sub,
                             "%s — up and down set your bid of %d trick%s, Enter confirms, Q quits",
                             R->name, mybid, mybid == 1 ? "" : "s");
                    draw_title("TRICK TAKING", sub);
                    draw_textf(4, 6, "%strump: %s%s", C_GREY,
                               trump < 0 ? "none" : SUIT_SYM[trump], C_RESET);
                    draw_hand(6, -1, -1, 0);
                    draw_textf(8, 6, "%sYour bid: %s%d%s", C_WHITE, C_YELLOW, mybid, C_RESET);
                    scr_flush();
                    k = key_get();
                    if (k == 'q' || k == 'Q' || k == KEY_ESC) goto finished;
                    if (k == KEY_UP   && mybid < hs) mybid++;
                    if (k == KEY_DOWN && mybid > 0)  mybid--;
                    if (k == KEY_ENTER) break;
                }
                bidv[0] = mybid;
                for (pl = 1; pl < R->players; pl++) {
                    int strong = 0;
                    for (i = 0; i < hn[pl]; i++)
                        if (rank_of(hand[pl][i]) >= 10 || suit_of(hand[pl][i]) == trump) strong++;
                    bidv[pl] = strong / 2;
                }
            }

            for (trickno = 0; trickno < hs; trickno++) {
                int played[NP], led = -1, bestpl = leader, bestval = -1;
                for (i = 0; i < R->players; i++) played[i] = -1;

                for (i = 0; i < R->players; i++) {
                    pl = (leader + i) % R->players;
                    if (pl == 0) {
                        for (;;) {
                            int k;
                            char sub[140];
                            snprintf(sub, sizeof sub,
                                     "%s — trick %d of %d, arrows pick a card, Enter plays, Q quits",
                                     R->name, trickno + 1, hs);
                            draw_title("TRICK TAKING", sub);
                            draw_textf(4, 6, "%strump %s   led %s%s", C_GREY,
                                       trump < 0 ? "none" : SUIT_SYM[trump],
                                       led < 0 ? "-" : SUIT_SYM[led], C_RESET);
                            {
                                int j;
                                for (j = 0; j < R->players; j++) {
                                    if (played[j] < 0) continue;
                                    card_name(played[j], nm, sizeof nm);
                                    draw_textf(6, 6 + j * 12, "%sP%d %s%-4s%s", C_GREY, j + 1,
                                               suit_colour(played[j]), nm, C_RESET);
                                }
                            }
                            draw_textf(9, 6, "%syour hand (grey cards are not legal):%s", C_GREY, C_RESET);
                            draw_hand(10, cur, led, trickno);
                            for (i = 0; i < R->players; i++)
                                draw_textf(13 + i, 6, "%sP%d  tricks %-2d  points %-3d  total %-3d%s%s",
                                           C_WHITE, i + 1, tricks[i], points[i], total[i],
                                           R->bid != B_NONE ? "  bid " : "", C_RESET);
                            if (R->bid != B_NONE)
                                for (i = 0; i < R->players; i++)
                                    draw_textf(13 + i, 46, "%s%d%s", C_YELLOW, bidv[i], C_RESET);
                            scr_flush();

                            k = key_get();
                            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto finished;
                            if (k == KEY_LEFT)  { cur = (cur + hn[0] - 1) % hn[0]; continue; }
                            if (k == KEY_RIGHT) { cur = (cur + 1) % hn[0]; continue; }
                            if (k != KEY_ENTER) continue;
                            if (!legal(0, cur, led, trickno)) continue;
                            break;
                        }
                        played[0] = hand[0][cur];
                        remove_card(0, cur);
                        if (cur >= hn[0] && cur > 0) cur--;
                    } else {
                        int idx = choose_card(pl, led, trickno, bestval);
                        played[pl] = hand[pl][idx];
                        remove_card(pl, idx);
                    }
                    if (led < 0) led = suit_of(played[pl]);
                    {
                        int v = card_value(played[pl], led);
                        if (v > bestval) { bestval = v; bestpl = pl; }
                    }
                }

                tricks[bestpl]++;
                for (i = 0; i < R->players; i++) {
                    if (R->score == S_POINTS) points[bestpl] += card_points(played[i]);
                    if (R->score == S_AVOID)  points[bestpl] += penalty(played[i]);
                }
                leader = bestpl;

                draw_textf(18, 6, "%sPlayer %d takes the trick.%s", C_GREEN, bestpl + 1, C_RESET);
                scr_flush();
                sleep_ms(420);
            }

            /* Settle the hand according to this game's scoring. */
            for (i = 0; i < R->players; i++) {
                if (R->score == S_AVOID)       total[i] += points[i];
                else if (R->score == S_POINTS) total[i] += points[i];
                else if (R->bid == B_EXACT)    total[i] += (tricks[i] == bidv[i]) ? 10 + tricks[i] : -2;
                else if (R->bid == B_TRICKS)   total[i] += (tricks[i] >= bidv[i]) ? bidv[i] * 10 + (tricks[i] - bidv[i]) : -bidv[i] * 10;
                else                           total[i] += tricks[i];
            }
            for (i = 0; i < R->players; i++)
                if (R->target && total[i] >= R->target) done = 1;
            if (!R->target) done = 1;

            scr_clear();
            draw_textf(8, 6, "%sHand complete.%s", C_BOLD, C_RESET);
            for (i = 0; i < R->players; i++)
                draw_textf(10 + i, 6, "%sPlayer %d: %d trick%s, running total %d%s",
                           i ? C_WHITE : C_GREEN, i + 1, tricks[i], tricks[i] == 1 ? "" : "s",
                           total[i], C_RESET);
            scr_flush();
            if (!confirm("\n\n  Another hand?")) goto finished;
        }

        score_report(p->title ? p->title : "tricktaking",
                     R->score == S_AVOID ? 200 - total[0] : total[0] * 10);
        if (!confirm("\n  Play again?")) return;
    }
finished:
    score_report(p->title ? p->title : "tricktaking",
                 R->score == S_AVOID ? 200 - total[0] : total[0] * 10);
}

/* ================================================================= Sevens */
/* Also called Fan Tan: build each suit outwards from the seven, or pass. */

static void play_sevens(const GParams *p)
{
    int deck[52], lo[4], hi[4], i, pl, cur = 0, passes[NP];

    for (;;) {
        int over = 0;
        shuffle_int(deck, 52);
        for (i = 0; i < 52; i++) deck[i] = i;
        shuffle_int(deck, 52);
        for (i = 0; i < 4; i++) { lo[i] = -1; hi[i] = -1; passes[i] = 0; }
        for (pl = 0; pl < 4; pl++) { hn[pl] = 0; for (i = 0; i < 13; i++) hand[pl][hn[pl]++] = deck[pl * 13 + i]; }
        cur = 0;

        while (!over) {
            for (pl = 0; pl < 4 && !over; pl++) {
                int placed = -1;
                if (pl == 0) {
                    for (;;) {
                        int k, s, r;
                        char nm[8];
                        draw_title("SEVENS",
                                   "Build outwards from each seven — arrows pick a card, "
                                   "Enter plays, P passes, Q quits");
                        for (i = 0; i < 4; i++)
                            draw_textf(4 + i, 8, "%s%s%s  %s%s%s", C_WHITE, SUIT_SYM[i], C_RESET,
                                       C_GREY, lo[i] < 0 ? "  (needs the seven)" : "", C_RESET);
                        for (i = 0; i < 4; i++)
                            if (lo[i] >= 0)
                                draw_textf(4 + i, 16, "%s%s .. %s%s", C_YELLOW,
                                           RANK_NAME[lo[i]], RANK_NAME[hi[i]], C_RESET);
                        for (i = 1; i < 4; i++)
                            draw_textf(9 + i, 8, "%sPlayer %d holds %d%s", C_GREY, i + 1, hn[i], C_RESET);
                        draw_textf(14, 8, "%syour hand:%s", C_GREY, C_RESET);
                        for (i = 0; i < hn[0]; i++) {
                            s = suit_of(hand[0][i]); r = rank_of(hand[0][i]);
                            card_name(hand[0][i], nm, sizeof nm);
                            draw_textf(15, 6 + i * 5, "%s%s%-4s%s", i == cur ? BG_BLUE : "",
                                       (lo[s] < 0 ? r == 6 : (r == lo[s] - 1 || r == hi[s] + 1))
                                         ? suit_colour(hand[0][i]) : C_GREY, nm, C_RESET);
                        }
                        scr_flush();
                        k = key_get();
                        if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "sevens", 52 - hn[0]); return; }
                        if (k == KEY_LEFT)  { cur = (cur + hn[0] - 1) % hn[0]; continue; }
                        if (k == KEY_RIGHT) { cur = (cur + 1) % hn[0]; continue; }
                        if (k == 'p' || k == 'P') { placed = -1; break; }
                        if (k != KEY_ENTER) continue;
                        s = suit_of(hand[0][cur]); r = rank_of(hand[0][cur]);
                        if (lo[s] < 0 ? r != 6 : (r != lo[s] - 1 && r != hi[s] + 1)) continue;
                        placed = cur;
                        break;
                    }
                } else {
                    for (i = 0; i < hn[pl]; i++) {
                        int s = suit_of(hand[pl][i]), r = rank_of(hand[pl][i]);
                        if (lo[s] < 0 ? r == 6 : (r == lo[s] - 1 || r == hi[s] + 1)) { placed = i; break; }
                    }
                }
                if (placed >= 0) {
                    int c = hand[pl][placed], s = suit_of(c), r = rank_of(c);
                    if (lo[s] < 0) { lo[s] = hi[s] = 6; }
                    else if (r == lo[s] - 1) lo[s] = r;
                    else hi[s] = r;
                    remove_card(pl, placed);
                    passes[pl] = 0;
                    if (pl == 0 && cur >= hn[0] && cur > 0) cur--;
                } else passes[pl]++;
                if (hn[pl] == 0) over = 1;
            }
        }
        scr_clear();
        draw_centered(12, 80, hn[0] == 0 ? "You went out first." : "Somebody else went out.");
        score_report(p->title ? p->title : "sevens", 200 - hn[0] * 10);
        if (!confirm("\n  Another deal?")) return;
    }
}


/* Scopa is not scored on the number of cards alone. Four points are settled at
 * the end of a game: most cards, most coins, the seven of coins, and primiera
 * (the best card in each suit, on a scale that ranks sevens highest). Sweeps
 * are added on top. Suit 2 stands in for coins. */
static int primiera_value(int c)
{
    static const int V[13] = {16, 12, 13, 14, 15, 18, 21, 10, 10, 10, 10, 10, 10};
    return V[rank_of(c)];
}

static int scopa_points(const int *cards, int n, const int *other, int on, int sweeps)
{
    int i, s, pts = sweeps, coins = 0, ocoins = 0;
    int bestsuit[4] = {0, 0, 0, 0}, obest[4] = {0, 0, 0, 0};
    for (i = 0; i < n; i++) {
        if (suit_of(cards[i]) == 2) coins++;
        if (cards[i] == 2 * 13 + 6) pts++;             /* the seven of coins */
        s = suit_of(cards[i]);
        if (primiera_value(cards[i]) > bestsuit[s]) bestsuit[s] = primiera_value(cards[i]);
    }
    for (i = 0; i < on; i++) {
        if (suit_of(other[i]) == 2) ocoins++;
        s = suit_of(other[i]);
        if (primiera_value(other[i]) > obest[s]) obest[s] = primiera_value(other[i]);
    }
    if (n > on)         pts++;                          /* most cards */
    if (coins > ocoins) pts++;                          /* most coins */
    {
        int mine = 0, yours = 0;
        for (s = 0; s < 4; s++) { mine += bestsuit[s]; yours += obest[s]; }
        if (mine > yours) pts++;                        /* primiera */
    }
    return pts;
}

/* ================================================================== Scopa */
/* Capture table cards by matching one, or by matching a sum. A sweep - taking
 * the whole table - is worth a point on its own. */

static void play_scopa(const GParams *p)
{
    int deck[40], nd, table[20], tn, cur = 0, pos;
    int mine[40], mn, theirs[40], tn2, sweeps[2], turn;

    for (;;) {
        int i, over = 0;
        R = &TR[11];
        build_deck(deck, &nd);
        pos = 0; tn = 0; mn = 0; tn2 = 0; sweeps[0] = sweeps[1] = 0; turn = 0;
        for (i = 0; i < 4; i++) table[tn++] = deck[pos++];

        while (!over) {
            int pl;
            for (pl = 0; pl < 2; pl++) { hn[pl] = 0; for (i = 0; i < 3 && pos < nd; i++) hand[pl][hn[pl]++] = deck[pos++]; }
            if (hn[0] == 0) break;

            for (i = 0; i < 3 && hn[0] > 0; i++) {
                for (pl = 0; pl < 2; pl++) {
                    int take = -1, c, j;
                    if (hn[pl] == 0) continue;
                    if (pl == 0) {
                        for (;;) {
                            int k;
                            char nm[8];
                            draw_title("SCOPA",
                                       "Capture a table card of the same rank — arrows pick, "
                                       "Enter plays, Q quits");
                            draw_textf(4, 8, "%stable:%s", C_GREY, C_RESET);
                            for (j = 0; j < tn; j++) {
                                card_name(table[j], nm, sizeof nm);
                                draw_textf(5, 8 + j * 5, "%s%-4s%s", suit_colour(table[j]), nm, C_RESET);
                            }
                            draw_textf(8, 8, "%syou have captured %d, they %d   sweeps %d-%d%s",
                                       C_GREY, mn, tn2, sweeps[0], sweeps[1], C_RESET);
                            draw_textf(10, 8, "%syour hand:%s", C_GREY, C_RESET);
                            for (j = 0; j < hn[0]; j++) {
                                card_name(hand[0][j], nm, sizeof nm);
                                draw_textf(11, 8 + j * 5, "%s%s%-4s%s", j == cur ? BG_BLUE : "",
                                           suit_colour(hand[0][j]), nm, C_RESET);
                            }
                            scr_flush();
                            k = key_get();
                            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "scopa", mn * 5); return; }
                            if (k == KEY_LEFT)  { cur = (cur + hn[0] - 1) % hn[0]; continue; }
                            if (k == KEY_RIGHT) { cur = (cur + 1) % hn[0]; continue; }
                            if (k == KEY_ENTER) break;
                        }
                        c = hand[0][cur];
                        remove_card(0, cur);
                        if (cur >= hn[0] && cur > 0) cur--;
                    } else {
                        c = hand[1][0];
                        remove_card(1, 0);
                    }
                    for (j = 0; j < tn; j++) if (rank_of(table[j]) == rank_of(c)) { take = j; break; }
                    if (take >= 0) {
                        if (pl == 0) { mine[mn++] = c; mine[mn++] = table[take]; }
                        else         { theirs[tn2++] = c; theirs[tn2++] = table[take]; }
                        for (j = take; j < tn - 1; j++) table[j] = table[j + 1];
                        tn--;
                        if (tn == 0) sweeps[pl]++;
                    } else table[tn++] = c;
                    turn++;
                }
            }
            if (pos >= nd && hn[0] == 0 && hn[1] == 0) over = 1;
        }
        scr_clear();
        {
            int a = scopa_points(mine, mn, theirs, tn2, sweeps[0]);
            int b = scopa_points(theirs, tn2, mine, mn, sweeps[1]);
            draw_textf(9, 6, "%sYou %d - %d them  (cards, coins, seven of coins, primiera, sweeps)%s",
                       C_BOLD, a, b, C_RESET);
            draw_centered(12, 80, a > b ? "You took the most points." : a == b ? "A tie." : "They took the most points.");
            score_report(p->title ? p->title : "scopa", a * 100 + mn);
        }
        if (!confirm("\n  Another game?")) return;
    }
}

void fam_tricktaking(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    if (v < 0 || v > 11) v = 0;
    R = &TR[v];
    if (v == 7)  { play_sevens(p); return; }
    if (v == 11) { play_scopa(p);  return; }
    play_tricks(p);
}
