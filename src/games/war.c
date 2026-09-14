/* GIC:PARAMETERISED war
 * war.c - the pure-chance card duels.
 *
 * params: variant = 0 War, 1 Casino War, 2 Red Dog
 *
 * These share a deck and nothing else. War is an attrition race for all 52
 * cards; Casino War is a single-hand bet where a tie forces you to double or
 * forfeit half; Red Dog pays on the spread between two cards, which is a
 * different bet entirely.
 */
#include "engine.h"
#include "cards.h"
#include "games.h"

#define MAX 52

static int pq[MAX], pn, ph, cq[MAX], cn, ch;  /* circular queues */

static int pop(int *q, int *n, int *head)
{
    int v;
    if (*n == 0) return -1;
    v = q[*head];
    *head = (*head + 1) % MAX;
    (*n)--;
    return v;
}

static void push(int *q, int *n, int head, int card)
{
    if (*n >= MAX) return;
    q[(head + *n) % MAX] = card;
    (*n)++;
}

static int rank_of(int card) { int r = card % 13; return r == 0 ? 13 : r; }

/* ------------------------------------------------------------------- war */
static void play_war(const GParams *p)
{
    int deck[52], i, rounds = 0;
    deck_init(deck);
    pn = ph = cn = ch = 0;
    for (i = 0; i < 26; i++) push(pq, &pn, ph, deck[i]);
    for (i = 26; i < 52; i++) push(cq, &cn, ch, deck[i]);

    while (pn > 0 && cn > 0 && rounds < 400) {
        int a, b, k, pot[32], np = 0;
        draw_title(p->title ? p->title : "WAR", "Enter plays a card, Q quits");
        draw_textf(6, 20, "%sYou: %-2d cards%s     %sComputer: %-2d cards%s   ",
                   C_CYAN, pn, C_RESET, C_RED, cn, C_RESET);
        scr_flush();
        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
        if (k != KEY_ENTER && k != ' ') continue;

        a = pop(pq, &pn, &ph);
        b = pop(cq, &cn, &ch);
        if (a < 0 || b < 0) break;
        pot[np++] = a; pot[np++] = b;
        draw_card(9, 20, a, 1);
        draw_card(9, 34, b, 1);
        scr_flush();

        while (rank_of(a) == rank_of(b) && pn > 1 && cn > 1 && np < 28) {
            int j;
            draw_textf(13, 20, "%sWAR! Three down, one up.%s   ", C_BOLD C_YELLOW, C_RESET);
            scr_flush();
            sleep_ms(500);
            for (j = 0; j < 3 && pn > 1 && cn > 1; j++) {
                pot[np++] = pop(pq, &pn, &ph);
                pot[np++] = pop(cq, &cn, &ch);
            }
            a = pop(pq, &pn, &ph);
            b = pop(cq, &cn, &ch);
            if (a < 0 || b < 0) return;
            pot[np++] = a; pot[np++] = b;
            draw_card(9, 20, a, 1);
            draw_card(9, 34, b, 1);
            scr_flush();
        }

        if (rank_of(a) > rank_of(b)) {
            draw_textf(13, 20, "%sYou take %d card%s.%s        ", C_GREEN, np, np == 1 ? "" : "s", C_RESET);
            for (i = 0; i < np; i++) push(pq, &pn, ph, pot[i]);
        } else {
            draw_textf(13, 20, "%sComputer takes %d card%s.%s  ", C_RED, np, np == 1 ? "" : "s", C_RESET);
            for (i = 0; i < np; i++) push(cq, &cn, ch, pot[i]);
        }
        scr_flush();
        sleep_ms(400);
        rounds++;
    }
    scr_clear();
    draw_centered(12, 80, pn > cn ? C_BOLD C_GREEN "You hold the most cards - you win!" C_RESET
                                  : C_BOLD C_RED "The computer takes the deck." C_RESET);
    score_report(p->title ? p->title : "war", pn);
    pause_msg("Press any key...");
}

/* ------------------------------------------------------------ casino war */
static void play_casino(const GParams *p)
{
    int deck[52], top = 52, chips = 100, bet = 10;
    while (chips > 0) {
        int a, b, k;
        if (top > 46) { deck_init(deck); top = 0; }
        for (;;) {
            draw_title(p->title ? p->title : "CASINO WAR",
                       "Up/Down change bet, Enter deals, Q quits");
            draw_textf(9, 28, "Chips: %-6d", chips);
            draw_textf(11, 28, "Bet:   %s%-6d%s", C_YELLOW, bet, C_RESET);
            scr_flush();
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP   && bet + 10 <= chips) bet += 10;
            if (k == KEY_DOWN && bet > 10)          bet -= 10;
            if (k == KEY_ENTER || k == ' ') break;
        }
        a = deck[top++]; b = deck[top++];
        draw_card(14, 24, a, 1);
        draw_card(14, 38, b, 1);
        scr_flush();

        if (rank_of(a) > rank_of(b)) {
            draw_textf(18, 24, "%sYour card is higher - you win %d.%s   ", C_GREEN, bet, C_RESET);
            chips += bet;
        } else if (rank_of(a) < rank_of(b)) {
            draw_textf(18, 24, "%sDealer takes it.%s                    ", C_RED, C_RESET);
            chips -= bet;
        } else {
            /* The tie is the whole game: double the stake or forfeit half. */
            draw_textf(18, 24, "%sTIE. W to go to war (double), S to surrender half.%s", C_YELLOW, C_RESET);
            scr_flush();
            for (;;) {
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                if (k == 's' || k == 'S') { chips -= bet / 2; break; }
                if (k == 'w' || k == 'W') {
                    if (chips < bet * 2) { chips -= bet / 2; break; }
                    if (top > 48) { deck_init(deck); top = 0; }
                    a = deck[top++]; b = deck[top++];
                    draw_card(14, 24, a, 1);
                    draw_card(14, 38, b, 1);
                    if (rank_of(a) >= rank_of(b)) {
                        draw_textf(20, 24, "%sYou win the war - %d.%s      ", C_GREEN, bet, C_RESET);
                        chips += bet;
                    } else {
                        draw_textf(20, 24, "%sYou lose the war - %d.%s     ", C_RED, bet * 2, C_RESET);
                        chips -= bet * 2;
                    }
                    break;
                }
            }
        }
        if (bet > chips) bet = chips > 10 ? chips : 10;
        score_report(p->title ? p->title : "casino-war", chips);
        scr_flush();
        pause_msg("Press any key...");
    }
    scr_clear();
    draw_centered(12, 80, C_BOLD C_RED "Out of chips." C_RESET);
    pause_msg("Press any key...");
}

/* ---------------------------------------------------------------- red dog */
static void play_red_dog(const GParams *p)
{
    int deck[52], top = 52, chips = 100, bet = 10;
    /* The narrower the gap, the bigger the payout. */
    static const int SPREAD_PAY[4] = {5, 4, 2, 1};   /* gap 1, 2, 3, 4+ */

    while (chips > 0) {
        int a, b, c, k, lo, hi, gap, pay;
        if (top > 46) { deck_init(deck); top = 0; }
        for (;;) {
            draw_title(p->title ? p->title : "RED DOG",
                       "Up/Down change bet, Enter deals, Q quits");
            draw_textf(8, 26, "Chips: %-6d", chips);
            draw_textf(10, 26, "Bet:   %s%-6d%s", C_YELLOW, bet, C_RESET);
            draw_textf(12, 26, "%sGap 1 pays 5:1, 2 pays 4:1, 3 pays 2:1, 4+ pays 1:1%s", C_GREY, C_RESET);
            scr_flush();
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP   && bet + 10 <= chips) bet += 10;
            if (k == KEY_DOWN && bet > 10)          bet -= 10;
            if (k == KEY_ENTER || k == ' ') break;
        }
        a = deck[top++]; b = deck[top++];
        lo = rank_of(a) < rank_of(b) ? rank_of(a) : rank_of(b);
        hi = rank_of(a) > rank_of(b) ? rank_of(a) : rank_of(b);
        draw_card(15, 22, a, 1);
        draw_card(15, 36, b, 1);
        gap = hi - lo - 1;

        if (hi == lo) {
            c = deck[top++];
            draw_card(15, 50, c, 1);
            if (rank_of(c) == lo) {
                draw_textf(19, 22, "%sThree of a kind - pays 11:1!%s   ", C_GREEN, C_RESET);
                chips += bet * 11;
            } else {
                draw_textf(19, 22, "%sPair, no third match - push.%s   ", C_GREY, C_RESET);
            }
        } else if (gap <= 0) {
            draw_textf(19, 22, "%sConsecutive cards - push.%s       ", C_GREY, C_RESET);
        } else {
            draw_textf(19, 22, "%sSpread of %d. Enter draws.%s      ", C_CYAN, gap, C_RESET);
            scr_flush();
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            c = deck[top++];
            draw_card(15, 50, c, 1);
            pay = SPREAD_PAY[gap > 4 ? 3 : gap - 1];
            if (rank_of(c) > lo && rank_of(c) < hi) {
                draw_textf(21, 22, "%sInside! Pays %d:1 - %d.%s     ", C_GREEN, pay, bet * pay, C_RESET);
                chips += bet * pay;
            } else {
                draw_textf(21, 22, "%sOutside the spread - you lose.%s ", C_RED, C_RESET);
                chips -= bet;
            }
        }
        if (bet > chips) bet = chips > 10 ? chips : 10;
        score_report(p->title ? p->title : "red-dog", chips);
        scr_flush();
        pause_msg("Press any key...");
    }
    scr_clear();
    draw_centered(12, 80, C_BOLD C_RED "Out of chips." C_RESET);
    pause_msg("Press any key...");
}

void fam_war(const GParams *p)
{
    switch (gp_int(p->variant, 0)) {
        case 1:  play_casino(p);  break;
        case 2:  play_red_dog(p); break;
        default: play_war(p);     break;
    }
}
