/* blackjack.c - hit, stand, double; dealer stands on 17. */
#include "engine.h"
#include "cards.h"
#include "games.h"

static int deck[52], top;
static int phand[12], pn, dhand[12], dn;
static int chips, bet;

static int deal(void)
{
    if (top >= 52) { deck_init(deck); top = 0; }
    return deck[top++];
}

static int hand_value(const int *h, int n)
{
    int i, total = 0, aces = 0;
    for (i = 0; i < n; i++) {
        int r = h[i] % 13;
        if (r == 0)      { total += 11; aces++; }
        else if (r >= 9) total += 10;
        else             total += r + 1;
    }
    while (total > 21 && aces > 0) { total -= 10; aces--; }
    return total;
}

static void render(int hide_dealer, const char *msg)
{
    int i;
    draw_title("BLACKJACK", "H hit, S stand, D double, Q quit");
    draw_textf(6, 20, "%sDealer%s", C_RED, C_RESET);
    for (i = 0; i < dn; i++) draw_card(7, 20 + i * 7, dhand[i], !(hide_dealer && i == 1));
    if (!hide_dealer) draw_textf(10, 20, "Total: %-3d   ", hand_value(dhand, dn));
    else              draw_textf(10, 20, "Total: ?    ");

    draw_textf(13, 20, "%sYou%s", C_CYAN, C_RESET);
    for (i = 0; i < pn; i++) draw_card(14, 20 + i * 7, phand[i], 1);
    draw_textf(17, 20, "Total: %-3d   ", hand_value(phand, pn));

    draw_textf(20, 20, "Chips: %-6d  Bet: %-5d   ", chips, bet);
    draw_text(22, 16, "                                                        ");
    if (msg) draw_textf(22, 20, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_blackjack(const GParams *p)
{
    (void)p;
    deck_init(deck);
    top = 0;
    chips = 100;

    while (chips > 0) {
        int done = 0, doubled = 0, pv, dv;
        bet = chips >= 10 ? 10 : chips;

        /* Bet selection. */
        for (;;) {
            int k;
            scr_clear();
            draw_title("BLACKJACK", "Up/Down change bet, Enter deals, Q quits");
            draw_textf(10, 30, "Chips: %-6d", chips);
            draw_textf(12, 30, "Bet:   %s%-6d%s", C_YELLOW, bet, C_RESET);
            scr_flush();
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP   && bet + 10 <= chips) bet += 10;
            if (k == KEY_DOWN && bet > 10)          bet -= 10;
            if (k == KEY_ENTER || k == ' ') break;
        }

        pn = dn = 0;
        phand[pn++] = deal(); dhand[dn++] = deal();
        phand[pn++] = deal(); dhand[dn++] = deal();

        if (hand_value(phand, pn) == 21) {
            render(0, "Blackjack! Pays 3:2.");
            chips += bet * 3 / 2;
            pause_msg("Press any key...");
            continue;
        }

        while (!done) {
            int k;
            render(1, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == 'h' || k == 'H') {
                phand[pn++] = deal();
                if (hand_value(phand, pn) > 21) done = 1;
                if (pn >= 11) done = 1;
            }
            if (k == 's' || k == 'S') done = 1;
            if ((k == 'd' || k == 'D') && pn == 2 && chips >= bet * 2) {
                bet *= 2;
                doubled = 1;
                phand[pn++] = deal();
                done = 1;
            }
        }
        (void)doubled;

        pv = hand_value(phand, pn);
        if (pv > 21) {
            render(0, "Bust - you lose.");
            chips -= bet;
        } else {
            while (hand_value(dhand, dn) < 17 && dn < 11) {
                dhand[dn++] = deal();
                render(0, "Dealer draws...");
                sleep_ms(600);
            }
            dv = hand_value(dhand, dn);
            if (dv > 21)      { render(0, "Dealer busts - you win!"); chips += bet; }
            else if (dv > pv) { render(0, "Dealer wins.");            chips -= bet; }
            else if (dv < pv) { render(0, "You win!");                chips += bet; }
            else                render(0, "Push.");
        }
        score_report("blackjack", chips);
        pause_msg("Press any key...");
    }
    scr_clear();
    draw_centered(12, 80, C_BOLD C_RED "Out of chips - game over." C_RESET);
    pause_msg("Press any key...");
}
