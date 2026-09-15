/* GIC:PARAMETERISED betting
 * betting.c - the games of pure chance.
 *
 * params: variant selects the game
 *
 * Eighteen games where the only decision is what to stake and on what. They
 * share a bankroll, a stake control and a settle-up step, so all of that is
 * written once in play_betting(); each variant supplies three things:
 *
 *   setup    what you may bet on, drawn as a list or a board
 *   resolve  one random event and the payout it produces
 *   reveal   whatever animation makes the result readable
 *
 * The wheels are modelled properly rather than as a flat random number: a
 * European wheel has one zero and an American has two, which is the entire
 * difference between a 2.7% and a 5.3% house edge, and the game shows it.
 */
#include "engine.h"
#include "cards.h"
#include "games.h"

/* Roulette wheels differ in their zeroes, and French rules return half a
 * losing even-money stake when the ball lands on zero. */
typedef struct {
    const char *name;
    int slots;        /* 37 European/French, 38 American, 13 Mini */
    int dbl_zero;     /* American's 00 */
    int la_partage;   /* French: half back on zero for even-money bets */
} Wheel;

static const Wheel WHEELS[4] = {
    {"European", 37, 0, 0},
    {"American", 38, 1, 0},
    {"French",   37, 0, 1},
    {"Mini",     13, 0, 0}
};

static const char *BETS[7] = {
    "Red", "Black", "Odd", "Even", "Low half", "High half", "Straight up"
};

/* Standard European colouring; anything outside the table is treated as red,
 * which only matters on the Mini wheel where 1-12 alternate. */
static int is_red(int n)
{
    static const int RED[18] = {1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36};
    int i;
    if (n <= 0) return 0;
    for (i = 0; i < 18; i++) if (RED[i] == n) return 1;
    return 0;
}

/* ------------------------------------------------------------- the wheel */

static int spin_roulette(const Wheel *w, int choice, int straight, int bet, int *paid)
{
    int slot, n, half = w->slots == 13 ? 6 : 18;
    slot = rnd(w->slots);
    n = slot;
    if (w->dbl_zero && slot == 37) n = -1;              /* -1 stands for 00 */

    *paid = 0;
    if (n <= 0) {
        /* Zero: even-money bets lose, except under la partage. */
        if (choice < 6 && w->la_partage) { *paid = bet / 2; return n; }
        if (choice == 6 && straight == 0) { *paid = bet * (w->slots - 1); return n; }
        return n;
    }
    switch (choice) {
    case 0: if (is_red(n))      *paid = bet * 2; break;
    case 1: if (!is_red(n))     *paid = bet * 2; break;
    case 2: if (n % 2)          *paid = bet * 2; break;
    case 3: if (n % 2 == 0)     *paid = bet * 2; break;
    case 4: if (n <= half)      *paid = bet * 2; break;
    case 5: if (n >  half)      *paid = bet * 2; break;
    case 6: if (n == straight)  *paid = bet * w->slots; break;
    }
    return n;
}

/* --------------------------------------------------------------- the rest */

/* Each of these returns the amount paid back for a stake of `bet`; zero means
 * the stake is lost. `detail` receives a line describing what happened. */

static int play_keno(int bet, int *pick, int npick, char *detail, int n)
{
    int drawn[80], i, hits = 0;
    static const int PAYS[11] = {0, 0, 0, 1, 3, 12, 36, 100, 300, 1000, 5000};
    for (i = 0; i < 80; i++) drawn[i] = i + 1;
    shuffle_int(drawn, 80);
    for (i = 0; i < npick; i++) {
        int j;
        for (j = 0; j < 20; j++) if (drawn[j] == pick[i]) hits++;
    }
    snprintf(detail, n, "%d of your %d numbers came up", hits, npick);
    return bet * PAYS[hits > 10 ? 10 : hits];
}

static int play_bingo(int bet, char *detail, int n)
{
    /* A 5x5 card against 30 calls; the payout climbs steeply with lines. */
    int card[25], called[76], i, j, lines = 0;
    for (i = 0; i < 25; i++) card[i] = 1 + rnd(75);
    for (i = 0; i < 76; i++) called[i] = 0;
    for (i = 0; i < 30; i++) called[1 + rnd(75)] = 1;
    card[12] = 0;                                    /* the free square */
    for (i = 0; i < 5; i++) {
        int row = 1, col = 1;
        for (j = 0; j < 5; j++) {
            if (card[i * 5 + j] && !called[card[i * 5 + j]]) row = 0;
            if (card[j * 5 + i] && !called[card[j * 5 + i]]) col = 0;
        }
        lines += row + col;
    }
    snprintf(detail, n, "%d line%s after thirty calls", lines, lines == 1 ? "" : "s");
    return lines ? bet * (lines + 1) : 0;
}

static int play_lottery(int bet, char *detail, int n)
{
    int mine[6], draw[49], i, j, hits = 0;
    static const int PAYS[7] = {0, 0, 0, 3, 25, 500, 50000};
    for (i = 0; i < 49; i++) draw[i] = i + 1;
    shuffle_int(draw, 49);
    for (i = 0; i < 6; i++) mine[i] = 1 + rnd(49);
    for (i = 0; i < 6; i++) for (j = 0; j < 6; j++) if (mine[i] == draw[j]) hits++;
    snprintf(detail, n, "%d matching number%s", hits, hits == 1 ? "" : "s");
    return bet * PAYS[hits];
}

static int play_scratch(int bet, char *detail, int n)
{
    int sym[9], i, best = 0, f;
    for (i = 0; i < 9; i++) sym[i] = rnd(6);
    for (f = 0; f < 6; f++) {
        int c = 0;
        for (i = 0; i < 9; i++) if (sym[i] == f) c++;
        if (c > best) best = c;
    }
    snprintf(detail, n, "best match: %d of a kind", best);
    return best >= 3 ? bet * (best - 1) : 0;
}

static int play_plinko(int bet, char *detail, int n)
{
    static const int SLOT[9] = {9, 4, 2, 1, 0, 1, 2, 4, 9};
    int pos = 4, i;
    for (i = 0; i < 8; i++) { pos += rnd(2) ? 1 : -1; if (pos < 0) pos = 0; if (pos > 8) pos = 8; }
    snprintf(detail, n, "landed in slot %d, paying %dx", pos + 1, SLOT[pos]);
    return bet * SLOT[pos];
}

static int play_horse(int bet, int pick, char *detail, int n)
{
    /* Six horses with published odds; the favourite wins most often. */
    static const int ODDS[6] = {2, 3, 4, 6, 10, 16};
    int weight[6], total = 0, r, i, winner = 0;
    for (i = 0; i < 6; i++) { weight[i] = 100 / ODDS[i]; total += weight[i]; }
    r = rnd(total);
    for (i = 0; i < 6; i++) { if (r < weight[i]) { winner = i; break; } r -= weight[i]; }
    snprintf(detail, n, "horse %d came home at %d-1", winner + 1, ODDS[winner]);
    return winner == pick ? bet * ODDS[pick] : 0;
}

static int play_monte(int bet, int pick, char *detail, int n)
{
    /* The dealer's shuffle is not quite fair, which is the point of the game. */
    int card = rnd(3);
    if (rnd(100) < 25) card = (card + 1) % 3;
    snprintf(detail, n, "the queen was under cup %d", card + 1);
    return pick == card ? bet * 2 : 0;
}

static int play_crash(int bet, int cashout, char *detail, int n)
{
    /* The multiplier grows until it busts; you set the exit beforehand. */
    int mult = 100, bust;
    for (bust = 100; ; bust += 7 + rnd(20)) if (rnd(100) < 4) break;
    mult = bust;
    if (cashout <= mult) {
        snprintf(detail, n, "crashed at %d.%02dx, you left at %d.%02dx",
                 mult / 100, mult % 100, cashout / 100, cashout % 100);
        return bet * cashout / 100;
    }
    snprintf(detail, n, "crashed at %d.%02dx before your %d.%02dx",
             mult / 100, mult % 100, cashout / 100, cashout % 100);
    return 0;
}

/* ------------------------------------------------------------------ main */

void fam_betting(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    int purse = 100, bet = 10, choice = 0, straight = 1, cashout = 200, best = 100;
    int pick[6] = {7, 14, 21, 28, 35, 42};
    const Wheel *wheel = &WHEELS[v < 4 ? v : 0];
    char detail[120] = "";
    const char *name;

    if (v < 0 || v > 17) v = 0;
    name = v <= 3 ? "ROULETTE" : v == 4 ? "KENO" : v == 5 ? "BINGO" :
           v == 6 ? "LOTTERY" : v == 7 ? "COIN STREAK" : v == 8 ? "WHEEL OF FORTUNE" :
           v == 9 ? "SCRATCH CARD" : v == 10 ? "THREE CARD MONTE" : v == 11 ? "PLINKO" :
           v == 12 ? "HORSE RACE" : v == 13 ? "ODDS AND EVENS" : v == 14 ? "MATCHING PENNIES" :
           v == 15 ? "DICE DUEL" : v == 16 ? "HIGH CARD DRAW" : "CRASH";

    for (;;) {
        int k, i, paid = 0, nopt = 2;
        char sub[140];

        if (v <= 3)       nopt = 7;
        else if (v == 12) nopt = 6;
        else if (v == 10) nopt = 3;
        else if (v == 8)  nopt = 4;

        snprintf(sub, sizeof sub,
                 "Purse %d — arrows choose, +/- stakes %d, Enter plays, Q quits", purse, bet);
        draw_title(name, sub);

        if (v <= 3) {
            draw_textf(4, 8, "%s%s wheel: %d slots%s%s", C_GREY, wheel->name, wheel->slots,
                       wheel->dbl_zero ? ", zero and double zero" :
                       wheel->la_partage ? ", half back on zero" : ", single zero", C_RESET);
            for (i = 0; i < 7; i++)
                draw_textf(6 + i, 10, "%s%s%-16s%s", i == choice ? BG_BLUE : "", C_WHITE, BETS[i], C_RESET);
            if (choice == 6)
                draw_textf(6 + 7, 10, "%snumber %d (left/right with the number chosen)%s",
                           C_YELLOW, straight, C_RESET);
        } else if (v == 4) {
            draw_textf(4, 8, "%sYour six numbers:%s", C_GREY, C_RESET);
            for (i = 0; i < 6; i++)
                draw_textf(6, 10 + i * 5, "%s%s %2d %s", i == choice ? BG_BLUE : "", C_WHITE, pick[i], C_RESET);
            draw_textf(8, 8, "%sUp and down change the highlighted number.%s", C_GREY, C_RESET);
        } else if (v == 12) {
            static const int ODDS[6] = {2, 3, 4, 6, 10, 16};
            for (i = 0; i < 6; i++)
                draw_textf(5 + i, 10, "%s%shorse %d at %2d-1%s", i == choice ? BG_BLUE : "",
                           C_WHITE, i + 1, ODDS[i], C_RESET);
        } else if (v == 10) {
            for (i = 0; i < 3; i++)
                draw_textf(6, 12 + i * 8, "%s%s cup %d %s", i == choice ? BG_BLUE : "", C_WHITE, i + 1, C_RESET);
        } else if (v == 17) {
            draw_textf(5, 8, "%sAuto cash-out at %d.%02dx — up and down adjust it.%s",
                       C_WHITE, cashout / 100, cashout % 100, C_RESET);
        } else if (v == 7 || v == 13 || v == 14 || v == 15 || v == 16 || v == 8) {
            static const char *TWO[4] = {"Heads / Odd / High", "Tails / Even / Low",
                                         "Double", "Jackpot"};
            for (i = 0; i < nopt; i++)
                draw_textf(6 + i, 10, "%s%s%-22s%s", i == choice ? BG_BLUE : "", C_WHITE, TWO[i], C_RESET);
        } else {
            draw_textf(6, 10, "%sPress Enter to play a ticket.%s", C_GREY, C_RESET);
        }

        draw_textf(16, 8, "%s%-70s%s", C_YELLOW, detail, C_RESET);
        draw_textf(18, 8, "%sbest purse %d%s", C_GREY, best, C_RESET);
        scr_flush();

        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        if (k == KEY_UP) {
            if (v == 4) { pick[choice] = pick[choice] % 80 + 1; continue; }
            if (v == 17) { if (cashout < 2000) cashout += 25; continue; }
            choice = (choice + nopt - 1) % nopt;
            continue;
        }
        if (k == KEY_DOWN) {
            if (v == 4) { pick[choice] = (pick[choice] + 78) % 80 + 1; continue; }
            if (v == 17) { if (cashout > 110) cashout -= 25; continue; }
            choice = (choice + 1) % nopt;
            continue;
        }
        if (k == KEY_LEFT) {
            if (v <= 3 && choice == 6) { straight = straight > 1 ? straight - 1 : wheel->slots - 1; continue; }
            choice = (choice + nopt - 1) % nopt;
            continue;
        }
        if (k == KEY_RIGHT) {
            if (v <= 3 && choice == 6) { straight = straight % (wheel->slots - 1) + 1; continue; }
            choice = (choice + 1) % nopt;
            continue;
        }
        if (k == '+' || k == '=') { if (bet + 5 <= purse) bet += 5; continue; }
        if (k == '-')             { if (bet > 5) bet -= 5; continue; }
        if (k != KEY_ENTER && k != ' ') continue;
        if (bet > purse) bet = purse;
        if (bet <= 0) break;

        switch (v) {
        case 0: case 1: case 2: case 3: {
            int n = spin_roulette(wheel, choice, straight, bet, &paid);
            /* Show the ball settling rather than snapping to the answer. */
            for (i = 0; i < 12; i++) {
                int fake = rnd(wheel->slots);
                draw_textf(14, 8, "%s  %2d  %s", is_red(fake) ? BG_RED : BG_BLUE, fake, C_RESET);
                scr_flush();
                sleep_ms(30 + i * 8);
            }
            if (n < 0) snprintf(detail, sizeof detail, "the ball landed on double zero");
            else snprintf(detail, sizeof detail, "the ball landed on %d (%s)", n,
                          n == 0 ? "zero" : is_red(n) ? "red" : "black");
            draw_textf(14, 8, "%s  %2d  %s", n <= 0 ? BG_GREEN : is_red(n) ? BG_RED : BG_BLUE,
                       n < 0 ? 0 : n, C_RESET);
            break;
        }
        case 4:  paid = play_keno(bet, pick, 6, detail, sizeof detail); break;
        case 5:  paid = play_bingo(bet, detail, sizeof detail); break;
        case 6:  paid = play_lottery(bet, detail, sizeof detail); break;
        case 9:  paid = play_scratch(bet, detail, sizeof detail); break;
        case 10: paid = play_monte(bet, choice, detail, sizeof detail); break;
        case 11: paid = play_plinko(bet, detail, sizeof detail); break;
        case 12: paid = play_horse(bet, choice, detail, sizeof detail); break;
        case 17: paid = play_crash(bet, cashout, detail, sizeof detail); break;
        case 8: {
            static const int SEG[4] = {2, 3, 5, 20};
            int seg = rnd(24), hit = seg < 12 ? 0 : seg < 18 ? 1 : seg < 23 ? 2 : 3;
            snprintf(detail, sizeof detail, "the wheel stopped on the %dx segment", SEG[hit]);
            paid = (hit == choice) ? bet * SEG[hit] : 0;
            break;
        }
        case 7: {
            /* Coin streak: keep flipping while you call it right. */
            int run = 0;
            while (rnd(2) == choice) { run++; if (run >= 8) break; }
            snprintf(detail, sizeof detail, "a run of %d before the coin turned", run);
            paid = run ? bet * (1 << (run - 1)) : 0;
            break;
        }
        case 13: case 15: {
            int a = 1 + rnd(6), b = 1 + rnd(6);
            if (v == 15) {
                snprintf(detail, sizeof detail, "you rolled %d, the bank rolled %d", a, b);
                paid = (choice == 0 ? a > b : a < b) ? bet * 2 : 0;
            } else {
                snprintf(detail, sizeof detail, "the two dice totalled %d", a + b);
                paid = (((a + b) % 2) == choice) ? bet * 2 : 0;
            }
            break;
        }
        case 14: {
            /* The opponent leans slightly towards whatever you chose last. */
            static int last = 0;
            int theirs = (rnd(100) < 58) ? last : rnd(2);
            snprintf(detail, sizeof detail, "they showed %s", theirs ? "tails" : "heads");
            paid = (theirs == choice) ? bet * 2 : 0;
            last = choice;
            break;
        }
        case 16: {
            int deck[52], mine, bank;
            deck_init(deck); shuffle_int(deck, 52);
            mine = deck[0] % 13; bank = deck[1] % 13;
            snprintf(detail, sizeof detail, "you drew %s, the bank drew %s",
                     RANK_NAME[mine], RANK_NAME[bank]);
            paid = (choice == 0 ? mine > bank : mine < bank) ? bet * 2 : 0;
            break;
        }
        default: break;
        }

        purse += paid - bet;
        if (purse > best) best = purse;
        draw_textf(16, 8, "%s%-70s%s", paid ? C_GREEN : C_RED, detail, C_RESET);
        draw_textf(17, 8, "%s%s %d — purse now %d%s", paid ? C_GREEN : C_RED,
                   paid ? "won" : "lost", paid ? paid - bet : bet, purse, C_RESET);
        scr_flush();
        sleep_ms(520);

        if (purse <= 0) {
            score_report(p->title ? p->title : "betting", best);
            scr_clear();
            draw_centered(12, 80, "The purse is empty.");
            if (!confirm("\n  Stake up again?")) return;
            purse = 100; bet = 10;
        }
        if (bet > purse) bet = purse;
    }
    score_report(p->title ? p->title : "betting", best);
}
