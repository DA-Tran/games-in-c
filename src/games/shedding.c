/* GIC:PARAMETERISED shedding
 * shedding.c - the shedding family: games about running out of cards.
 *
 * params: variant selects the game
 *
 * Three mechanisms, twelve games:
 *
 *   match    Crazy Eights, President, Cheat and Durak. You play onto a pile
 *            under some matching rule, or you cannot and you pick up. The
 *            differences - what matches, whether you may lie, whether the
 *            defender must beat rather than match - are a Rules row each.
 *   pair     Old Maid, Go Fish and Rummy. Cards leave in matched sets, and
 *            the interest is in what you ask for or draw.
 *   reflex   Snap, Slapjack, Beggar My Neighbour and Egyptian Ratscrew. The
 *            cards fall on their own and the whole game is when you slap.
 */
#include "engine.h"
#include "cards.h"
#include "games.h"

#define NPL   4
#define MAXH 52

enum { M_RANKSUIT, M_HIGHER, M_ANY };

typedef struct {
    const char *name;
    int players, hand;
    int match;        /* what may be played on the pile      */
    int wild;         /* rank that plays on anything, -1 none */
    int drawpen;      /* cards drawn when you cannot play     */
    int bluff;        /* may you lie about what you played    */
} SRules;

/*                    name                pl hand match       wild draw bluff */
static const SRules SR[5] = {
    {"Crazy Eights",           4, 7, M_RANKSUIT,  7, 1, 0},
    {"Crazy Eights Wild Draw", 4, 7, M_RANKSUIT,  7, 2, 0},
    {"President",              4,13, M_HIGHER,   -1, 0, 0},
    {"Cheat",                  4,13, M_ANY,      -1, 0, 1},
    {"Durak",                  2, 6, M_HIGHER,   -1, 0, 0}
};

static int hand[NPL][MAXH], hn[NPL];
static int rank_of(int c) { return c % 13; }
static int suit_of(int c) { return c / 13; }

static void remove_card(int pl, int i)
{
    for (; i < hn[pl] - 1; i++) hand[pl][i] = hand[pl][i + 1];
    hn[pl]--;
}

static void show_hand(int row, int cur, int n, const int *h, const int *ok)
{
    int i;
    char nm[8];
    for (i = 0; i < n && i < 18; i++) {
        card_name(h[i], nm, sizeof nm);
        draw_textf(row, 5 + i * 5, "%s%s%-4s%s", i == cur ? BG_BLUE : "",
                   (ok && !ok[i]) ? C_GREY : suit_colour(h[i]), nm, C_RESET);
    }
    if (n > 18) draw_textf(row, 5 + 18 * 5, "%s+%d%s", C_GREY, n - 18, C_RESET);
}

/* ================================================================== match */

static void play_match(const GParams *p, int ri)
{
    const SRules *R = &SR[ri];
    int deck[52], pos, pile, pilesuit, cur = 0, i, pl, claimed = 0, stack = 0;
    const char *msg = "";

    for (;;) {
        int over = 0, winner = -1;
        deck_init(deck); shuffle_int(deck, 52);
        pos = 0;
        for (pl = 0; pl < R->players; pl++) {
            hn[pl] = 0;
            for (i = 0; i < R->hand && pos < 52; i++) hand[pl][hn[pl]++] = deck[pos++];
        }
        /* Cheat and President deal the whole pack, so the pile starts empty. */
        pile = (pos < 52) ? deck[pos++] : -1;
        pilesuit = (pile >= 0) ? suit_of(pile) : -1;
        cur = 0; claimed = 0; stack = 0; msg = "";

        while (!over) {
            for (pl = 0; pl < R->players && !over; pl++) {
                int ok[MAXH], any = 0, chosen = -1;

                for (i = 0; i < hn[pl]; i++) {
                    int c = hand[pl][i];
                    if (pile < 0)                   ok[i] = 1;   /* empty pile */
                    else if (R->match == M_ANY)     ok[i] = 1;
                    else if (R->wild >= 0 && rank_of(c) == R->wild) ok[i] = 1;
                    else if (R->match == M_HIGHER)  ok[i] = rank_of(c) > rank_of(pile);
                    else ok[i] = (rank_of(c) == rank_of(pile) || suit_of(c) == pilesuit);
                    if (ok[i]) any = 1;
                }

                if (pl == 0) {
                    for (;;) {
                        int k;
                        char nm[8], sub[150];
                        snprintf(sub, sizeof sub,
                                 "%s — arrows pick, Enter plays, D draws or passes%s, Q quits",
                                 R->name, R->bluff ? ", C calls a bluff" : "");
                        draw_title("SHEDDING", sub);
                        if (pile < 0)
                            draw_textf(4, 6, "%spile: empty — anything may go down%s", C_GREY, C_RESET);
                        else {
                            card_name(pile, nm, sizeof nm);
                            draw_textf(4, 6, "%spile: %s%s%s   suit in play %s%s",
                                       C_GREY, suit_colour(pile), nm, C_GREY, SUIT_SYM[pilesuit], C_RESET);
                        }
                        if (R->bluff && stack)
                            draw_textf(5, 6, "%slast player claimed %d x %s%s", C_YELLOW,
                                       stack, RANK_NAME[claimed], C_RESET);
                        for (i = 1; i < R->players; i++)
                            draw_textf(7 + i, 6, "%sPlayer %d holds %d%s", C_GREY, i + 1, hn[i], C_RESET);
                        draw_textf(12, 6, "%syour hand (grey will not go down):%s", C_GREY, C_RESET);
                        show_hand(13, cur, hn[0], hand[0], R->bluff ? NULL : ok);
                        draw_textf(15, 6, "%s%-68s%s", C_YELLOW, msg, C_RESET);
                        scr_flush();
                        msg = "";

                        k = key_get();
                        if (k == 'q' || k == 'Q' || k == KEY_ESC) goto done;
                        if (k == KEY_LEFT)  { if (hn[0]) cur = (cur + hn[0] - 1) % hn[0]; continue; }
                        if (k == KEY_RIGHT) { if (hn[0]) cur = (cur + 1) % hn[0]; continue; }
                        if (k == 'c' && R->bluff) {
                            /* Calling the bluff: the liar takes the pile back. */
                            msg = (rank_of(pile) == claimed) ? "They were telling the truth — you pick up."
                                                             : "Caught! They pick up.";
                            if (pile >= 0) {
                                if (rank_of(pile) == claimed) hand[0][hn[0]++] = pile;
                                else                          hand[1][hn[1]++] = pile;
                            }
                            pile = -1;          /* the pile is carried away */
                            pilesuit = -1;
                            stack = 0;
                            break;
                        }
                        if (k == 'd' || k == 'D') {
                            for (i = 0; i < R->drawpen && pos < 52; i++) hand[0][hn[0]++] = deck[pos++];
                            msg = R->drawpen ? "Drew from the stock." : "Passed.";
                            chosen = -1;
                            break;
                        }
                        if (k != KEY_ENTER) continue;
                        if (!hn[0]) break;
                        if (!R->bluff && !ok[cur]) { msg = "That card will not go down."; continue; }
                        chosen = cur;
                        break;
                    }
                } else if (any) {
                    for (i = 0; i < hn[pl]; i++) if (ok[i]) { chosen = i; break; }
                } else if (R->drawpen && pos < 52) {
                    for (i = 0; i < R->drawpen && pos < 52; i++) hand[pl][hn[pl]++] = deck[pos++];
                }

                if (chosen >= 0 && hn[pl] > 0) {
                    pile = hand[pl][chosen];
                    claimed = R->bluff ? rank_of(pile) : rank_of(pile);
                    if (R->bluff && rnd(100) < 30) claimed = rnd(13);   /* the lie */
                    stack = R->bluff ? stack + 1 : 0;
                    pilesuit = (R->wild >= 0 && rank_of(pile) == R->wild)
                               ? rnd(4) : suit_of(pile);
                    remove_card(pl, chosen);
                    if (pl == 0 && cur >= hn[0] && cur > 0) cur--;
                }
                if (hn[pl] == 0) { over = 1; winner = pl; }
            }
            if (pos >= 52 && !over) {
                /* Stock exhausted: shortest hand wins. */
                int bestn = 99;
                for (i = 0; i < R->players; i++) if (hn[i] < bestn) { bestn = hn[i]; winner = i; }
                over = 1;
            }
        }
        scr_clear();
        draw_centered(12, 80, winner == 0 ? "You went out first." : "Somebody else went out.");
        score_report(p->title ? p->title : "shedding", winner == 0 ? 500 : 100);
        if (!confirm("\n  Another deal?")) return;
    }
done:
    score_report(p->title ? p->title : "shedding", 100);
}

/* =================================================================== pair */

static void play_pair(const GParams *p, int variant)
{
    /* 5 Old Maid, 10 Go Fish, 11 Rummy */
    int deck[52], pos, cur = 0, i, j, pl, ask = 0;
    int books[NPL];
    const char *name = variant == 5 ? "OLD MAID" : variant == 10 ? "GO FISH" : "RUMMY";
    int players = variant == 5 ? 3 : variant == 10 ? 3 : 2;
    const char *msg = "";

    for (;;) {
        int over = 0;
        deck_init(deck); shuffle_int(deck, 52);
        pos = 0;
        for (i = 0; i < players; i++) books[i] = 0;

        if (variant == 5) {
            /* Old Maid: one queen is removed, so one queen has no partner. */
            for (i = 0; i < 52; i++) if (deck[i] == 0 * 13 + 11) { deck[i] = deck[51]; break; }
            for (pl = 0; pl < players; pl++) hn[pl] = 0;
            for (i = 0; i < 51; i++) hand[i % players][hn[i % players]++] = deck[i];
            /* Discard the pairs each player starts with. */
            for (pl = 0; pl < players; pl++)
                for (i = 0; i < hn[pl]; i++)
                    for (j = i + 1; j < hn[pl]; j++)
                        if (rank_of(hand[pl][i]) == rank_of(hand[pl][j])) {
                            remove_card(pl, j); remove_card(pl, i);
                            books[pl]++; i = -1;
                            break;
                        }
        } else {
            int per = variant == 10 ? 7 : 10;
            for (pl = 0; pl < players; pl++) { hn[pl] = 0; for (i = 0; i < per; i++) hand[pl][hn[pl]++] = deck[pos++]; }
        }
        cur = 0; msg = "";

        while (!over) {
            for (pl = 0; pl < players && !over; pl++) {
                if (pl == 0) {
                    for (;;) {
                        int k;
                        char sub[150];
                        snprintf(sub, sizeof sub,
                                 variant == 5 ? "%s — arrows pick a card to take from the next player, Enter takes, Q quits"
                               : variant == 10 ? "%s — arrows pick a rank to ask for, Enter asks, Q quits"
                                               : "%s — arrows pick, Enter melds a set of three, D draws, Q quits",
                                 name);
                        draw_title("SHEDDING", sub);
                        for (i = 1; i < players; i++)
                            draw_textf(4 + i, 6, "%sPlayer %d holds %-2d  books %d%s",
                                       C_GREY, i + 1, hn[i], books[i], C_RESET);
                        draw_textf(9, 6, "%syour books: %d%s", C_WHITE, books[0], C_RESET);
                        draw_textf(11, 6, "%syour hand:%s", C_GREY, C_RESET);
                        show_hand(12, cur, hn[0], hand[0], NULL);
                        if (variant == 5 && hn[1])
                            draw_textf(14, 6, "%sPlayer 2 offers %d hidden cards — pick position %d%s",
                                       C_GREY, hn[1], ask + 1, C_RESET);
                        draw_textf(16, 6, "%s%-68s%s", C_YELLOW, msg, C_RESET);
                        scr_flush();
                        msg = "";

                        k = key_get();
                        if (k == 'q' || k == 'Q' || k == KEY_ESC) goto done2;
                        if (k == KEY_LEFT)  { if (hn[0]) cur = (cur + hn[0] - 1) % hn[0]; if (ask) ask--; continue; }
                        if (k == KEY_RIGHT) { if (hn[0]) cur = (cur + 1) % hn[0]; if (hn[1]) ask = (ask + 1) % hn[1]; continue; }
                        if (k == 'd' && variant == 11) {
                            if (pos < 52) hand[0][hn[0]++] = deck[pos++];
                            break;
                        }
                        if (k != KEY_ENTER) continue;

                        if (variant == 5) {
                            if (!hn[1]) break;
                            hand[0][hn[0]++] = hand[1][ask % hn[1]];
                            remove_card(1, ask % hn[1]);
                            ask = 0;
                            msg = "Took a card from Player 2.";
                        } else if (variant == 10) {
                            int want = rank_of(hand[0][cur]), got = 0;
                            for (i = 1; i < players; i++)
                                for (j = hn[i] - 1; j >= 0; j--)
                                    if (rank_of(hand[i][j]) == want) { hand[0][hn[0]++] = hand[i][j]; remove_card(i, j); got++; }
                            if (!got) { if (pos < 52) hand[0][hn[0]++] = deck[pos++]; msg = "Go fish."; }
                            else msg = "They handed them over.";
                        } else {
                            int want = rank_of(hand[0][cur]), c = 0;
                            for (i = 0; i < hn[0]; i++) if (rank_of(hand[0][i]) == want) c++;
                            if (c < 3) { msg = "You need three of a rank to meld."; continue; }
                            for (i = hn[0] - 1; i >= 0; i--) if (rank_of(hand[0][i]) == want) remove_card(0, i);
                            books[0]++;
                            msg = "Melded.";
                        }
                        break;
                    }
                } else {
                    /* The opponents follow the same rules, played simply. */
                    if (variant == 5 && hn[(pl + 1) % players]) {
                        int src = (pl + 1) % players, take = rnd(hn[src]);
                        hand[pl][hn[pl]++] = hand[src][take];
                        remove_card(src, take);
                    } else if (variant == 10 && hn[pl]) {
                        int want = rank_of(hand[pl][rnd(hn[pl])]), got = 0;
                        for (i = 0; i < players; i++) {
                            if (i == pl) continue;
                            for (j = hn[i] - 1; j >= 0; j--)
                                if (rank_of(hand[i][j]) == want) { hand[pl][hn[pl]++] = hand[i][j]; remove_card(i, j); got++; }
                        }
                        if (!got && pos < 52) hand[pl][hn[pl]++] = deck[pos++];
                    } else if (variant == 11) {
                        int r;
                        if (pos < 52) hand[pl][hn[pl]++] = deck[pos++];
                        for (r = 0; r < 13; r++) {
                            int c = 0;
                            for (i = 0; i < hn[pl]; i++) if (rank_of(hand[pl][i]) == r) c++;
                            if (c >= 3) {
                                for (i = hn[pl] - 1; i >= 0; i--) if (rank_of(hand[pl][i]) == r) remove_card(pl, i);
                                books[pl]++;
                                break;
                            }
                        }
                    }
                }

                /* Books complete themselves as soon as the cards are together. */
                if (variant != 11)
                    for (i = 0; i < hn[pl]; i++)
                        for (j = i + 1; j < hn[pl]; j++)
                            if (rank_of(hand[pl][i]) == rank_of(hand[pl][j])) {
                                remove_card(pl, j); remove_card(pl, i);
                                books[pl]++; i = -1;
                                break;
                            }
                if (hn[pl] == 0) over = 1;
                if (pl == 0 && cur >= hn[0] && cur > 0) cur--;
            }
            if (pos >= 52 && variant != 5) {
                int allempty = 1;
                for (i = 0; i < players; i++) if (hn[i]) allempty = 0;
                if (allempty) over = 1;
            }
        }
        scr_clear();
        if (variant == 5) {
            int loser = 0;
            for (i = 0; i < players; i++) if (hn[i] > hn[loser]) loser = i;
            draw_centered(12, 80, loser == 0 ? "You were left with the old maid."
                                             : "Somebody else was left with the old maid.");
            score_report(p->title ? p->title : "shedding", loser == 0 ? 100 : 500);
        } else {
            int bestpl = 0;
            for (i = 0; i < players; i++) if (books[i] > books[bestpl]) bestpl = i;
            draw_centered(12, 80, bestpl == 0 ? "You collected the most." : "Somebody else collected more.");
            score_report(p->title ? p->title : "shedding", books[0] * 100);
        }
        if (!confirm("\n  Another deal?")) return;
    }
done2:
    score_report(p->title ? p->title : "shedding", books[0] * 100);
}

/* ================================================================= reflex */

static void play_reflex(const GParams *p, int variant)
{
    /* 6 Snap, 7 Slapjack, 8 Beggar My Neighbour, 9 Egyptian Ratscrew */
    int deck[52], pile[52], pn, mine, theirs, score = 0, best = 0;
    const char *name = variant == 6 ? "SNAP" : variant == 7 ? "SLAPJACK" :
                       variant == 8 ? "BEGGAR MY NEIGHBOUR" : "EGYPTIAN RATSCREW";

    for (;;) {
        long shown = 0;
        int turn = 0, live = 0, pay = 0;
        deck_init(deck); shuffle_int(deck, 52);
        mine = 26; theirs = 26; pn = 0; score = 0;

        for (;;) {
            int k;
            char nm[8];

            /* A card falls roughly twice a second unless a slap is pending. */
            if (!live && now_ms() - shown > 700) {
                int who = turn % 2, c;
                if ((who == 0 && mine == 0) || (who == 1 && theirs == 0)) break;
                c = deck[(turn * 7) % 52];
                if (who == 0) mine--; else theirs--;
                pile[pn++] = c;
                if (pn >= 52) pn = 52 - 1;
                shown = now_ms();
                turn++;

                /* Is this a slappable position? */
                if (variant == 7)      live = (rank_of(c) == 10);
                else if (variant == 6) live = (pn >= 2 && rank_of(pile[pn - 1]) == rank_of(pile[pn - 2]));
                else if (variant == 9) live = (pn >= 2 && rank_of(pile[pn - 1]) == rank_of(pile[pn - 2])) ||
                                              (pn >= 3 && rank_of(pile[pn - 1]) == rank_of(pile[pn - 3]));
                else {
                    /* Beggar My Neighbour: a court card sets a forfeit. */
                    int r = rank_of(c);
                    pay = r == 10 ? 1 : r == 11 ? 2 : r == 12 ? 3 : r == 0 ? 4 : 0;
                    live = pay > 0;
                }
            }

            draw_title(name, "Press Space when the pile is slappable — Q quits");
            if (pn) {
                card_name(pile[pn - 1], nm, sizeof nm);
                draw_textf(6, 30, "%s%s   %s   %s", BG_BLUE, suit_colour(pile[pn - 1]), nm, C_RESET);
            }
            draw_textf(9,  10, "%syour stock %-3d    their stock %-3d   pile %-3d%s",
                       C_GREY, mine, theirs, pn, C_RESET);
            draw_textf(11, 10, "%sscore %d  (best %d)%s", C_WHITE, score, best, C_RESET);
            if (variant == 8 && pay)
                draw_textf(13, 10, "%sa forfeit of %d card%s is owed%s", C_YELLOW, pay,
                           pay == 1 ? "" : "s", C_RESET);
            else if (live)
                draw_textf(13, 10, "%sSLAP!%s", C_GREEN, C_RESET);
            scr_flush();

            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out;
            if (k == ' ' || k == KEY_ENTER) {
                if (live) {
                    mine += pn; pn = 0; live = 0; pay = 0;
                    score += 50;
                    if (score > best) best = score;
                } else {
                    /* A false slap costs a card, which is the real rule. */
                    if (mine > 0) { mine--; theirs++; }
                    score -= 20;
                    if (score < 0) score = 0;
                }
                shown = now_ms();
            }
            /* The opponent slaps too, a little slower than a person. */
            if (live && now_ms() - shown > 520) {
                theirs += pn; pn = 0; live = 0; pay = 0;
            }
            if (mine <= 0 || theirs <= 0) break;
            sleep_ms(14);
        }
        scr_clear();
        draw_centered(12, 80, mine > theirs ? "You took most of the pack." : "They took most of the pack.");
        score_report(p->title ? p->title : "shedding", best + mine * 10);
        if (!confirm("\n  Another game?")) return;
    }
out:
    score_report(p->title ? p->title : "shedding", best);
}

void fam_shedding(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    if (v < 0 || v > 11) v = 0;
    switch (v) {
    case 0: play_match(p, 0); break;
    case 1: play_match(p, 1); break;
    case 2: play_match(p, 2); break;
    case 3: play_match(p, 3); break;
    case 4: play_match(p, 4); break;
    case 5: case 10: case 11: play_pair(p, v); break;
    default: play_reflex(p, v); break;
    }
}
