/* GIC:PARAMETERISED blackjack
 * blackjack.c - the twenty-one family.
 *
 * params: variant = which house rules
 *
 * The variants move the actual edge rather than the wallpaper: shoe size,
 * whether the hole card is exposed, whether the dealer hits soft 17, what a
 * natural pays, who takes ties, whether the tens are stripped out, and whether
 * a five-card hand wins outright. Those switches are the difference between
 * Vegas Strip and Pontoon.
 */
#include "engine.h"
#include "cards.h"
#include "games.h"

#define MAXDECK 8
#define SHOE (MAXDECK * 52)

typedef struct {
    int decks;
    int hole_up;       /* dealer's second card is dealt face up  */
    int hit_soft17;    /* dealer draws on soft 17                */
    int bj_num, bj_den;/* what a natural pays                    */
    int ties_dealer;   /* dealer takes pushes                    */
    int no_tens;       /* the four plain tens are removed        */
    int five_card;     /* five cards under 22 wins immediately   */
    int surrender;     /* may fold for half the stake            */
    const char *name;
} BJRules;

static const BJRules BJ[9] = {
    {2, 0, 0, 3, 2, 0, 0, 0, 0, "Blackjack"},
    {1, 0, 1, 3, 2, 0, 0, 0, 0, "Single Deck"},
    {6, 0, 0, 3, 2, 0, 0, 0, 0, "Six Deck"},
    {8, 1, 1, 1, 1, 1, 0, 0, 0, "Double Exposure"},
    {6, 0, 1, 3, 2, 0, 1, 1, 0, "Spanish 21"},
    {4, 0, 0, 3, 2, 0, 0, 0, 0, "Vegas Strip"},
    {8, 0, 0, 3, 2, 0, 0, 0, 1, "Atlantic City"},
    {8, 1, 0, 1, 1, 1, 0, 0, 0, "Face Up 21"},
    {1, 0, 1, 2, 1, 1, 0, 1, 0, "Pontoon"}
};

static BJRules R;
static int deck[SHOE], top, ncards;
static int phand[16], pn, dhand[16], dn;
static int chips, bet;

static void shoe_init(void)
{
    int d, r, s;
    ncards = 0;
    for (d = 0; d < R.decks; d++)
        for (s = 0; s < 4; s++)
            for (r = 0; r < 13; r++) {
                if (R.no_tens && r == 9) continue;   /* Spanish 21 strips tens */
                deck[ncards++] = s * 13 + r;
            }
    for (d = ncards - 1; d > 0; d--) {
        int j = rnd(d + 1), t = deck[d];
        deck[d] = deck[j]; deck[j] = t;
    }
    top = 0;
}

static int deal(void)
{
    if (top >= ncards) shoe_init();
    return deck[top++];
}

/* Total, and whether an ace is still counted as eleven. */
static int hand_value(const int *h, int n, int *soft)
{
    int i, total = 0, aces = 0;
    for (i = 0; i < n; i++) {
        int r = h[i] % 13;
        if (r == 0)      { total += 11; aces++; }
        else if (r >= 9) total += 10;
        else             total += r + 1;
    }
    while (total > 21 && aces > 0) { total -= 10; aces--; }
    if (soft) *soft = aces > 0;
    return total;
}

static int is_natural(const int *h, int n)
{
    return n == 2 && hand_value(h, n, NULL) == 21;
}

static void render(int hide_dealer, const char *msg)
{
    int i, soft;
    char sub[110];
    snprintf(sub, sizeof sub, "%s — %d deck%s, dealer %s soft 17, natural pays %d:%d%s%s",
             R.name, R.decks, R.decks == 1 ? "" : "s",
             R.hit_soft17 ? "hits" : "stands", R.bj_num, R.bj_den,
             R.no_tens ? ", no tens" : "", R.five_card ? ", five-card trick" : "");
    draw_title("TWENTY-ONE", sub);
    draw_textf(5, 16, "%sDealer%s", C_RED, C_RESET);
    for (i = 0; i < dn; i++) draw_card(6, 16 + i * 7, dhand[i], !(hide_dealer && i == 1));
    if (!hide_dealer) draw_textf(9, 16, "Total: %-3d   ", hand_value(dhand, dn, NULL));
    else              draw_textf(9, 16, "Total: ?     ");

    draw_textf(12, 16, "%sYou%s", C_CYAN, C_RESET);
    for (i = 0; i < pn; i++) draw_card(13, 16 + i * 7, phand[i], 1);
    hand_value(phand, pn, &soft);
    draw_textf(16, 16, "Total: %-3d%s   ", hand_value(phand, pn, NULL), soft ? " (soft)" : "      ");

    draw_textf(19, 16, "Chips: %-6d  Bet: %-5d   ", chips, bet);
    draw_text(21, 12, "                                                            ");
    if (msg) draw_textf(21, 16, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_blackjack(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    if (v < 0 || v > 8) v = 0;
    R = BJ[v];
    shoe_init();
    chips = 100;

    while (chips > 0) {
        int done = 0, pv, dv, dsoft, surrendered = 0, fivecard = 0;
        char keys[96];
        bet = chips >= 10 ? 10 : chips;

        for (;;) {
            int k;
            scr_clear();
            draw_title("TWENTY-ONE", "Up/Down change bet, Enter deals, Q quits");
            draw_textf(10, 30, "%s", R.name);
            draw_textf(12, 30, "Chips: %-6d", chips);
            draw_textf(14, 30, "Bet:   %s%-6d%s", C_YELLOW, bet, C_RESET);
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

        snprintf(keys, sizeof keys, "H hit, S stand, D double%s, Q quit",
                 R.surrender ? ", R surrender" : "");

        if (is_natural(phand, pn)) {
            int win = bet * R.bj_num / R.bj_den;
            /* Double Exposure and Face Up still let a dealer natural push. */
            if (R.hole_up && is_natural(dhand, dn)) {
                render(0, "Both naturals — dealer takes the tie.");
                chips -= bet;
            } else {
                char m[64];
                snprintf(m, sizeof m, "Natural! Pays %d:%d.", R.bj_num, R.bj_den);
                render(0, m);
                chips += win;
            }
            score_report(p->title ? p->title : "blackjack", chips);
            pause_msg("Press any key...");
            continue;
        }

        while (!done) {
            int k;
            render(R.hole_up ? 0 : 1, keys);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == 'h' || k == 'H') {
                phand[pn++] = deal();
                if (hand_value(phand, pn, NULL) > 21) done = 1;
                if (R.five_card && pn >= 5 && hand_value(phand, pn, NULL) <= 21) { fivecard = 1; done = 1; }
                if (pn >= 15) done = 1;
            }
            if (k == 's' || k == 'S') done = 1;
            if ((k == 'r' || k == 'R') && R.surrender && pn == 2) { surrendered = 1; done = 1; }
            if ((k == 'd' || k == 'D') && pn == 2 && chips >= bet * 2) {
                bet *= 2;
                phand[pn++] = deal();
                done = 1;
            }
        }

        if (surrendered) {
            render(0, "Surrendered — half the stake back.");
            chips -= bet / 2;
            score_report(p->title ? p->title : "blackjack", chips);
            pause_msg("Press any key...");
            continue;
        }
        if (fivecard) {
            render(0, "Five-card trick — you win!");
            chips += bet;
            score_report(p->title ? p->title : "blackjack", chips);
            pause_msg("Press any key...");
            continue;
        }

        pv = hand_value(phand, pn, NULL);
        if (pv > 21) {
            render(0, "Bust — you lose.");
            chips -= bet;
        } else {
            for (;;) {
                dv = hand_value(dhand, dn, &dsoft);
                if (dn >= 15) break;
                if (dv < 17) { dhand[dn++] = deal(); render(0, "Dealer draws..."); sleep_ms(420); continue; }
                if (dv == 17 && dsoft && R.hit_soft17) {
                    dhand[dn++] = deal();
                    render(0, "Dealer hits soft 17...");
                    sleep_ms(420);
                    continue;
                }
                break;
            }
            dv = hand_value(dhand, dn, NULL);
            if (dv > 21)      { render(0, "Dealer busts — you win!"); chips += bet; }
            else if (dv > pv) { render(0, "Dealer wins.");            chips -= bet; }
            else if (dv < pv) { render(0, "You win!");                chips += bet; }
            else if (R.ties_dealer) { render(0, "Tie — dealer takes it."); chips -= bet; }
            else                render(0, "Push.");
        }
        score_report(p->title ? p->title : "blackjack", chips);
        pause_msg("Press any key...");
    }
    scr_clear();
    draw_centered(12, 80, C_BOLD C_RED "Out of chips - game over." C_RESET);
    pause_msg("Press any key...");
}
