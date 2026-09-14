/* GIC:PARAMETERISED videopoker
 * video_poker.c - the draw-poker machine family.
 *
 * params: level = which paytable
 *
 * The paytables are the game. Bonus Poker splits four-of-a-kind by rank,
 * Double Double adds a kicker tier, Deuces Wild and Joker Poker introduce wild
 * cards (which need a different evaluator and add five-of-a-kind and wild
 * royals), Tens or Better lowers the qualifying pair, and All American pays
 * flushes and straights like full houses. So the evaluator returns a fine-
 * grained category and each table decides what that category is worth.
 */
#include "engine.h"
#include "cards.h"
#include "games.h"

/* Categories, richest first. A paytable pays 0 for ones it does not offer. */
enum {
    CAT_NAT_ROYAL, CAT_FOUR_DEUCES, CAT_WILD_ROYAL, CAT_FIVE_KIND,
    CAT_STR_FLUSH, CAT_FOUR_ACE_KICK, CAT_FOUR_ACES, CAT_FOUR_2_4,
    CAT_FOUR_FACE, CAT_FOUR_OTHER, CAT_FULL_HOUSE, CAT_FLUSH,
    CAT_STRAIGHT, CAT_THREE, CAT_TWO_PAIR, CAT_PAIR, CAT_N
};

static const char *CAT_NAME[CAT_N] = {
    "Royal Flush", "Four Deuces", "Wild Royal", "Five of a Kind",
    "Straight Flush", "Four Aces + kicker", "Four Aces", "Four 2s-4s",
    "Four J-K", "Four of a Kind", "Full House", "Flush",
    "Straight", "Three of a Kind", "Two Pair", "Pair"
};

typedef struct {
    const char *name;
    int wild;              /* 0 none, 1 deuces, 2 joker */
    int qualify;           /* lowest paying pair: 0 ace, 9 ten, 10 jack, 11 queen */
    int pay[CAT_N];
} Paytable;

static const Paytable TABLES[9] = {
 {"Jacks or Better", 0, 10,
  {800,0,0,0,50,0,0,0,0,25,9,6,4,3,2,1}},
 {"Bonus Poker", 0, 10,
  {800,0,0,0,50,0,80,40,0,25,8,5,4,3,2,1}},
 {"Double Bonus Poker", 0, 10,
  {800,0,0,0,50,0,160,80,0,50,9,7,5,3,1,1}},
 {"Double Double Bonus", 0, 10,
  {800,0,0,0,50,400,160,80,0,50,9,6,4,3,1,1}},
 {"Deuces Wild", 1, 0,
  {800,200,25,15,9,0,0,0,0,5,3,2,2,1,0,0}},
 {"Joker Poker", 2, 11,
  {800,0,100,200,50,0,0,0,0,20,7,5,3,2,1,1}},
 {"Aces and Faces", 0, 10,
  {800,0,0,0,50,0,80,0,40,25,8,5,4,3,2,1}},
 {"Tens or Better", 0, 9,
  {800,0,0,0,50,0,0,0,0,25,6,5,4,3,2,1}},
 {"All American", 0, 10,
  {800,0,0,0,200,0,0,0,0,40,8,8,8,3,1,1}}
};

static Paytable T;
static int deck[56], ndeck, top, hand[5], hold[5];
static int credits;

static int is_wild(int card)
{
    if (T.wild == 1) return card % 13 == 1;     /* every deuce */
    if (T.wild == 2) return card >= 52;         /* the joker   */
    return 0;
}

static void shuffle_deck(void)
{
    int i;
    ndeck = 52;
    for (i = 0; i < 52; i++) deck[i] = i;
    if (T.wild == 2) deck[ndeck++] = 52;        /* joker poker adds one card */
    for (i = ndeck - 1; i > 0; i--) {
        int j = rnd(i + 1), t = deck[i];
        deck[i] = deck[j]; deck[j] = t;
    }
    top = 0;
}

/* Best category this hand can reach, wilds filling whatever helps most. */
static int evaluate(void)
{
    int counts[13] = {0}, suits[4] = {0}, i, r;
    int wilds = 0, maxcount = 0, distinct = 0, pairs = 0, three = 0;
    int flush = 0, straight = 0, royal = 0;
    int ranks[5], nr = 0, suit_of_flush = -1;

    for (i = 0; i < 5; i++) {
        if (is_wild(hand[i])) { wilds++; continue; }
        counts[hand[i] % 13]++;
        suits[hand[i] / 13]++;
        ranks[nr++] = hand[i] % 13;
    }
    for (i = 0; i < 13; i++) {
        if (counts[i] > maxcount) maxcount = counts[i];
        if (counts[i]) distinct++;
        if (counts[i] == 2) pairs++;
        if (counts[i] >= 3) three = 1;
    }
    for (i = 0; i < 4; i++) if (suits[i] + wilds >= 5) { flush = 1; suit_of_flush = i; }

    /* A straight needs five distinct ranks spanning at most five, with wilds
     * covering the gaps. The ace can sit high or low. */
    if (distinct + wilds >= 5 && distinct == nr) {
        int lo, hi_ok = 0;
        for (lo = 0; lo < 13 && !hi_ok; lo++) {
            int need = 0, k;
            for (k = 0; k < 5; k++) {
                int want = (lo + k) % 13;
                if (lo + k > 12) break;
                if (!counts[want]) need++;
            }
            if (lo + 4 <= 12 && need <= wilds) hi_ok = 1;
        }
        /* ace-high run A,10,J,Q,K */
        {
            int need = 0;
            if (!counts[0]) need++;
            if (!counts[9]) need++;
            if (!counts[10]) need++;
            if (!counts[11]) need++;
            if (!counts[12]) need++;
            if (need <= wilds) { straight = 2; }
        }
        if (hi_ok && !straight) straight = 1;
    }
    if (straight == 2 && flush) {
        int ok = 1;
        for (i = 0; i < nr; i++) if (hand[i] / 13 != suit_of_flush) { }
        for (i = 0; i < 5; i++)
            if (!is_wild(hand[i]) && hand[i] / 13 != suit_of_flush) ok = 0;
        if (ok) royal = 1;
    }

    if (royal && wilds == 0)               return CAT_NAT_ROYAL;
    if (T.wild == 1 && wilds == 4)         return CAT_FOUR_DEUCES;
    if (royal)                             return CAT_WILD_ROYAL;
    if (maxcount + wilds >= 5)             return CAT_FIVE_KIND;
    if (straight && flush)                 return CAT_STR_FLUSH;

    if (maxcount + wilds >= 4) {
        /* Which rank makes the four matters for the bonus tables. */
        int quad = -1;
        for (i = 0; i < 13; i++) if (counts[i] + wilds >= 4) { quad = i; break; }
        if (quad == 0) {
            if (T.pay[CAT_FOUR_ACE_KICK]) {
                for (i = 0; i < nr; i++)
                    if (ranks[i] >= 1 && ranks[i] <= 3) return CAT_FOUR_ACE_KICK;
            }
            if (T.pay[CAT_FOUR_ACES]) return CAT_FOUR_ACES;
        }
        if (quad >= 1 && quad <= 3 && T.pay[CAT_FOUR_2_4]) return CAT_FOUR_2_4;
        if (quad >= 10 && quad <= 12 && T.pay[CAT_FOUR_FACE]) return CAT_FOUR_FACE;
        return CAT_FOUR_OTHER;
    }
    if ((three && pairs >= 1) || (pairs == 2 && wilds >= 1)) return CAT_FULL_HOUSE;
    if (flush)                             return CAT_FLUSH;
    if (straight)                          return CAT_STRAIGHT;
    if (maxcount + wilds >= 3)             return CAT_THREE;
    if (pairs == 2)                        return CAT_TWO_PAIR;
    if (pairs == 1) {
        for (r = 0; r < 13; r++) {
            if (counts[r] != 2) continue;
            /* Ace counts as the highest card, so it always qualifies. */
            if (r == 0 || r >= T.qualify) return CAT_PAIR;
        }
    }
    return -1;
}

static void render(int cur, int phase, const char *msg)
{
    int i, row = 4;
    char sub[110];
    snprintf(sub, sizeof sub, "%s%s — Left/Right select, Space holds, Enter draws",
             T.name, T.wild == 1 ? " (deuces are wild)" : T.wild == 2 ? " (joker is wild)" : "");
    draw_title("VIDEO POKER", sub);
    for (i = 0; i < CAT_N; i++) {
        if (!T.pay[i]) continue;
        draw_textf(row++, 50, "%s%-20s %4d%s", C_GREY, CAT_NAME[i], T.pay[i], C_RESET);
    }
    for (i = 0; i < 5; i++) {
        draw_card(9, 10 + i * 7, hand[i], 1);
        draw_textf(12, 10 + i * 7, "%s%s%s",
                   hold[i] ? C_BOLD C_YELLOW : C_GREY,
                   hold[i] ? " HOLD " : "      ", C_RESET);
        draw_textf(13, 10 + i * 7, "%s%s%s", i == cur ? C_REV : "",
                   i == cur ? "  ^^  " : "      ", C_RESET);
    }
    draw_textf(16, 10, "Credits: %-6d   %s", credits, phase ? "Draw phase " : "Hold phase ");
    draw_text(18, 6, "                                                        ");
    if (msg) draw_textf(18, 10, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_video_poker(const GParams *p)
{
    int lv = gp_int(p->level, 0);
    if (lv < 0 || lv > 8) lv = 0;
    T = TABLES[lv];
    credits = 100;
    shuffle_deck();

    while (credits > 0) {
        int cur = 0, i, result;
        const int bet = 5;

        if (top > ndeck - 12) shuffle_deck();
        for (i = 0; i < 5; i++) { hand[i] = deck[top++]; hold[i] = 0; }
        credits -= bet;

        for (;;) {
            int k;
            render(cur, 0, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  cur = (cur + 4) % 5;
            if (k == KEY_RIGHT) cur = (cur + 1) % 5;
            if (k == ' ')       hold[cur] = !hold[cur];
            if (k == KEY_ENTER) break;
        }
        for (i = 0; i < 5; i++)
            if (!hold[i]) {
                if (top >= ndeck) shuffle_deck();
                hand[i] = deck[top++];
            }

        result = evaluate();
        if (result >= 0 && T.pay[result]) {
            char m[80];
            credits += T.pay[result] * bet;
            snprintf(m, sizeof m, "%s - pays %d!", CAT_NAME[result], T.pay[result] * bet);
            render(cur, 1, m);
        } else {
            render(cur, 1, "No win.");
        }
        score_report(p->title ? p->title : "video-poker", credits);
        pause_msg("Press any key for the next hand...");
    }
    scr_clear();
    draw_centered(12, 80, C_BOLD C_RED "Out of credits." C_RESET);
    pause_msg("Press any key...");
}
