/* GIC:PARAMETERISED dice
 * dice.c - the dice family.
 *
 * params: variant selects the game
 *
 * Twenty dice games that fall into six mechanisms, and the file is arranged
 * by mechanism rather than by name so the shared parts are obvious:
 *
 *   scoresheet   Yacht, Balut and Kismet fill a card of categories. They share
 *                one loop and differ only in the category table, so the table
 *                is data and the loop is written once.
 *   press        Farkle, Zonk, Pig, Drop Dead and Threes are push-your-luck:
 *                bank what you have or roll again and risk it.
 *   wager        Craps, Chuck-a-Luck, Sic Bo and Cee-lo are bets against a
 *                bank with published odds.
 *   bluff        Liar's Dice and Dudo hide the dice and bid on the total.
 *   race         Bunco, Beetle, Mexico, Going to Boston and Klondike Dice are
 *                first-past-the-post.
 *   shutbox      Shut the Box is its own thing.
 */
#include "engine.h"
#include "games.h"

#define MAXD 6

static int die[MAXD], keep[MAXD], nd;

static void roll_free(void)
{
    int i;
    for (i = 0; i < nd; i++) if (!keep[i]) die[i] = 1 + rnd(6);
}

/* Dice drawn as pips so a held die reads differently at a glance. */
static void show_dice(int row, int col, int n, const int *d, const int *held, int cursor)
{
    static const char *FACE[7] = {"   ", " . ", ": .", " : ", "::.", ":.:", ":::"};
    int i;
    for (i = 0; i < n; i++) {
        const char *bg = (i == cursor) ? BG_BLUE : "";
        const char *fg = (held && held[i]) ? C_GREEN : C_WHITE;
        draw_textf(row,     col + i * 6, "%s%s+---+%s", bg, C_GREY, C_RESET);
        draw_textf(row + 1, col + i * 6, "%s%s|%s|%s", bg, fg, FACE[d[i] < 1 ? 0 : d[i]], C_RESET);
        draw_textf(row + 2, col + i * 6, "%s%s+---+%s", bg, C_GREY, C_RESET);
        draw_textf(row + 3, col + i * 6, "%s %d %s", (held && held[i]) ? C_GREEN : C_GREY,
                   d[i], C_RESET);
    }
}

static int count_face(int f) { int i, n = 0; for (i = 0; i < nd; i++) if (die[i] == f) n++; return n; }
static int sum_all(void)     { int i, s = 0; for (i = 0; i < nd; i++) s += die[i]; return s; }

/* ====================================================== the score sheets */

enum { CAT_NUM, CAT_NKIND, CAT_FULL, CAT_SMALL, CAT_LARGE, CAT_CHANCE, CAT_YACHT,
       CAT_HIGH, CAT_LOW, CAT_TWOPAIR };

typedef struct { const char *name; int kind; int arg; int value; } Cat;

/* Yacht: the 1938 shipboard game - straights and a full house at fixed values. */
static const Cat YACHT[12] = {
    {"Ones",          CAT_NUM,   1, 0}, {"Twos",        CAT_NUM,   2, 0},
    {"Threes",        CAT_NUM,   3, 0}, {"Fours",       CAT_NUM,   4, 0},
    {"Fives",         CAT_NUM,   5, 0}, {"Sixes",       CAT_NUM,   6, 0},
    {"Full House",    CAT_FULL,  0, 0}, {"Four of a Kind", CAT_NKIND, 4, 0},
    {"Little Straight", CAT_SMALL, 0, 30}, {"Big Straight", CAT_LARGE, 0, 30},
    {"Choice",        CAT_CHANCE, 0, 0}, {"Yacht",       CAT_YACHT, 0, 50}
};

/* Balut: a Danish/Thai variant scored in points rather than pips. */
static const Cat BALUT[7] = {
    {"Fours",  CAT_NUM, 4, 0}, {"Fives", CAT_NUM, 5, 0}, {"Sixes", CAT_NUM, 6, 0},
    {"Straight", CAT_LARGE, 0, 20}, {"Full House", CAT_FULL, 0, 0},
    {"Choice", CAT_CHANCE, 0, 0}, {"Balut", CAT_YACHT, 0, 20}
};

/* Kismet: coloured dice give a two-pair category and a higher five-kind. */
static const Cat KISMET[13] = {
    {"Ones",   CAT_NUM, 1, 0}, {"Twos",  CAT_NUM, 2, 0}, {"Threes", CAT_NUM, 3, 0},
    {"Fours",  CAT_NUM, 4, 0}, {"Fives", CAT_NUM, 5, 0}, {"Sixes",  CAT_NUM, 6, 0},
    {"Two Pairs Same Colour", CAT_TWOPAIR, 0, 0},
    {"Three of a Kind", CAT_NKIND, 3, 0}, {"Straight", CAT_LARGE, 0, 30},
    {"Flush", CAT_FULL, 0, 0}, {"Full House", CAT_FULL, 0, 0},
    {"Four of a Kind", CAT_NKIND, 4, 0}, {"Yarborough", CAT_CHANCE, 0, 0}
};

static int cat_score(const Cat *c)
{
    int i, f, cnt[7] = {0,0,0,0,0,0,0}, pairs = 0, three = 0;
    for (i = 0; i < nd; i++) cnt[die[i]]++;
    switch (c->kind) {
    case CAT_NUM:    return count_face(c->arg) * c->arg;
    case CAT_CHANCE: return sum_all();
    case CAT_NKIND:
        for (f = 1; f <= 6; f++) if (cnt[f] >= c->arg) return sum_all();
        return 0;
    case CAT_FULL:
        for (f = 1; f <= 6; f++) { if (cnt[f] == 2) pairs++; if (cnt[f] == 3) three++; }
        return (pairs && three) ? sum_all() + 2 : 0;
    case CAT_TWOPAIR:
        for (f = 1; f <= 6; f++) if (cnt[f] >= 2) pairs++;
        return pairs >= 2 ? sum_all() : 0;
    case CAT_SMALL:  return (cnt[1] && cnt[2] && cnt[3] && cnt[4] && cnt[5]) ? c->value : 0;
    case CAT_LARGE:  return (cnt[2] && cnt[3] && cnt[4] && cnt[5] && cnt[6]) ? c->value : 0;
    case CAT_YACHT:
        for (f = 1; f <= 6; f++) if (cnt[f] == nd) return c->value;
        return 0;
    }
    return 0;
}

static void play_sheet(const GParams *p, const Cat *cats, int ncat, const char *title)
{
    int used[13], got[13], i, cur = 0, rolls, turn, total;

    for (;;) {
        for (i = 0; i < ncat; i++) { used[i] = 0; got[i] = 0; }
        nd = 5;
        turn = 0; total = 0;

        while (turn < ncat) {
            int k;
            for (i = 0; i < nd; i++) { keep[i] = 0; die[i] = 0; }
            roll_free();
            rolls = 1;
            cur = 0;

            for (;;) {
                char sub[160];
                snprintf(sub, sizeof sub,
                         "Roll %d of 3 — arrows pick a line, 1-5 hold dice, R rerolls, "
                         "Enter scores the highlighted line, Q quits", rolls);
                draw_title(title, sub);
                show_dice(4, 10, nd, die, keep, -1);
                for (i = 0; i < ncat; i++) {
                    int s = used[i] ? got[i] : cat_score(&cats[i]);
                    draw_textf(9 + i, 8, "%s%s%-24s %s%4d%s",
                               i == cur ? BG_BLUE : "", used[i] ? C_GREY : C_WHITE,
                               cats[i].name, used[i] ? C_GREY : C_YELLOW, s, C_RESET);
                    if (used[i]) draw_textf(9 + i, 38, "%staken%s", C_GREY, C_RESET);
                }
                draw_textf(9 + ncat + 1, 8, "%sTotal %-5d  turn %d/%d%s",
                           C_BOLD, total, turn + 1, ncat, C_RESET);
                scr_flush();

                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "dice", total); return; }
                if (k == KEY_UP)    { cur = (cur + ncat - 1) % ncat; continue; }
                if (k == KEY_DOWN)  { cur = (cur + 1) % ncat; continue; }
                if (k >= '1' && k <= '5') { int d2 = k - '1'; keep[d2] = !keep[d2]; continue; }
                if (k == ' ')  { keep[0] = !keep[0]; continue; }
                if (k == 'r' || k == 'R') {
                    if (rolls >= 3) continue;
                    roll_free();
                    rolls++;
                    continue;
                }
                if (k == KEY_ENTER) {
                    if (used[cur]) continue;
                    got[cur] = cat_score(&cats[cur]);
                    used[cur] = 1;
                    total += got[cur];
                    turn++;
                    break;
                }
            }
        }
        score_report(p->title ? p->title : "dice", total);
        scr_clear();
        draw_centered(12, 80, "Sheet complete.");
        if (!confirm("\n  Another card?")) return;
    }
}

/* ========================================================= push your luck */

/* Farkle and Zonk score the same set; the difference is only the target. */
static int farkle_score(const int *d, int n, int *used)
{
    int cnt[7] = {0,0,0,0,0,0,0}, i, f, s = 0;
    for (i = 0; i < n; i++) cnt[d[i]]++;
    *used = 0;
    for (f = 1; f <= 6; f++) {
        if (cnt[f] >= 3) {
            s += (f == 1) ? 1000 : f * 100;
            s += (cnt[f] - 3) * (f == 1 ? 1000 : f * 100);   /* each extra doubles the set */
            *used += cnt[f];
            cnt[f] = 0;
        }
    }
    s += cnt[1] * 100 + cnt[5] * 50;
    *used += cnt[1] + cnt[5];
    return s;
}

static void play_press(const GParams *p, int variant)
{
    /* 0 Farkle, 9 Zonk, 16 Pig, 13 Drop Dead, 14 Threes */
    int target = (variant == 0) ? 10000 : (variant == 9) ? 5000 : 100;
    int you = 0, cpu = 0, turnpts, alive, i;
    const char *name = variant == 0 ? "FARKLE" : variant == 9 ? "ZONK" :
                       variant == 16 ? "PIG DICE" : variant == 13 ? "DROP DEAD" : "THREES";

    for (;;) {
        int k, mine = 1;
        you = cpu = 0;

        while (you < target && cpu < target) {
            turnpts = 0;
            alive = (variant == 13 || variant == 14) ? 5 : (variant == 16 ? 1 : 6);
            nd = alive;

            for (;;) {
                char sub[120];
                int bust = 0, used = 0, gain = 0;

                for (i = 0; i < nd; i++) keep[i] = 0;
                roll_free();

                if (variant == 16) {                       /* Pig: a one wipes the turn */
                    if (die[0] == 1) bust = 1; else gain = die[0];
                } else if (variant == 13) {                /* Drop Dead: fives and twos die */
                    int surv = 0, hasbad = 0;
                    for (i = 0; i < nd; i++) if (die[i] == 2 || die[i] == 5) hasbad = 1;
                    if (!hasbad) for (i = 0; i < nd; i++) gain += die[i];
                    for (i = 0; i < nd; i++) if (die[i] != 2 && die[i] != 5) die[surv++] = die[i];
                    nd = surv;
                    if (nd == 0) bust = 1;
                } else if (variant == 14) {                /* Threes: threes count zero, low wins */
                    int lowest = 0, li = 0;
                    for (i = 0; i < nd; i++) { int v = die[i] == 3 ? 0 : die[i];
                                               if (i == 0 || v > lowest) { lowest = v; li = i; } }
                    gain = lowest;
                    for (i = li; i < nd - 1; i++) die[i] = die[i + 1];
                    nd--;
                    if (nd == 0) bust = 0;
                } else {
                    gain = farkle_score(die, nd, &used);
                    if (gain == 0) bust = 1;
                    else { nd -= used; if (nd == 0) nd = 6; }   /* hot dice: all six score */
                }

                snprintf(sub, sizeof sub,
                         "%s — Space rolls again, Enter banks %d, Q quits",
                         mine ? "Your turn" : "Opponent", turnpts + gain);
                draw_title(name, sub);
                show_dice(4, 10, nd > 0 ? nd : 1, die, NULL, -1);
                draw_textf(9,  8, "%sthis roll %-5d   turn total %-6d%s", C_GREY, gain, turnpts + gain, C_RESET);
                draw_textf(11, 8, "%syou %-6d   opponent %-6d   target %d%s", C_WHITE, you, cpu, target, C_RESET);
                if (bust) draw_textf(13, 8, "%s%s%s", C_RED,
                                     variant == 16 ? "A one — the turn is lost." :
                                     variant == 13 ? "Every die dropped dead." :
                                     "Nothing scores — the turn is lost.", C_RESET);
                scr_flush();

                if (bust) { turnpts = 0; sleep_ms(500); break; }
                turnpts += gain;

                if (!mine) {
                    /* The opponent banks once it has enough to be worth keeping. */
                    sleep_ms(350);
                    if (turnpts >= (variant == 16 ? 20 : 350) || (variant != 16 && nd <= 2)) break;
                    continue;
                }
                if (variant == 14 && nd == 0) break;

                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "dice", you); return; }
                if (k == KEY_ENTER) break;
            }

            if (mine) you += turnpts; else cpu += turnpts;
            mine = !mine;
        }
        score_report(p->title ? p->title : "dice", you);
        scr_clear();
        draw_centered(12, 80, you >= target ? "You reached the target first." : "The opponent got there first.");
        if (!confirm("\n  Play again?")) return;
    }
}

/* ============================================================== the bank */

static void play_wager(const GParams *p, int variant)
{
    /* 1 Craps, 6 Chuck-a-Luck, 18 Sic Bo, 5 Cee-lo */
    int purse = 100, bet = 10, choice = 0, best = 100;
    const char *name = variant == 1 ? "CRAPS" : variant == 6 ? "CHUCK-A-LUCK" :
                       variant == 18 ? "SIC BO" : "CEE-LO";
    static const char *CRAPS[3] = {"Pass line", "Don't pass", "Field (2-4, 9-12)"};
    static const char *SICBO[4] = {"Small (4-10)", "Big (11-17)", "Any triple", "Total is 9"};
    const char **opts = variant == 1 ? CRAPS : SICBO;
    int nopt = variant == 1 ? 3 : variant == 18 ? 4 : 6;

    for (;;) {
        int k, i, win = 0, payout = 0;
        char sub[120];
        snprintf(sub, sizeof sub,
                 "Purse %d — arrows choose, +/- stakes %d, Enter rolls, Q quits", purse, bet);
        draw_title(name, sub);

        if (variant == 6) {
            draw_textf(4, 8, "%sPick a number; you are paid once for each die that shows it.%s", C_GREY, C_RESET);
            for (i = 0; i < 6; i++)
                draw_textf(6, 10 + i * 6, "%s%s  %d  %s", i == choice ? BG_BLUE : "", C_WHITE, i + 1, C_RESET);
        } else if (variant == 5) {
            draw_textf(4, 8, "%sThree dice: 4-5-6 wins, 1-2-3 loses, a pair sets your point.%s", C_GREY, C_RESET);
            draw_textf(6, 8, "%sPress Enter to throw.%s", C_GREY, C_RESET);
        } else {
            for (i = 0; i < nopt; i++)
                draw_textf(5 + i, 10, "%s%s%-22s%s", i == choice ? BG_BLUE : "", C_WHITE, opts[i], C_RESET);
        }
        draw_textf(14, 8, "%sbest purse %d%s", C_GREY, best, C_RESET);
        scr_flush();

        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        if (k == KEY_UP)   { choice = (choice + nopt - 1) % nopt; continue; }
        if (k == KEY_DOWN) { choice = (choice + 1) % nopt; continue; }
        if (k == KEY_LEFT) { choice = (choice + nopt - 1) % nopt; continue; }
        if (k == KEY_RIGHT){ choice = (choice + 1) % nopt; continue; }
        if (k == '+' || k == '=') { if (bet + 5 <= purse) bet += 5; continue; }
        if (k == '-')             { if (bet > 5) bet -= 5; continue; }
        if (k != KEY_ENTER) continue;
        if (bet > purse) { bet = purse; continue; }

        nd = (variant == 1) ? 2 : 3;
        for (i = 0; i < nd; i++) keep[i] = 0;
        roll_free();
        {
            int total = sum_all();
            draw_title(name, "The throw");
            show_dice(4, 10, nd, die, NULL, -1);
            draw_textf(9, 8, "%stotal %d%s", C_WHITE, total, C_RESET);

            if (variant == 1) {
                /* One roll of the pass line, resolved with a point if needed. */
                int point = 0;
                if (choice == 0) {
                    if (total == 7 || total == 11) { win = 1; payout = bet; }
                    else if (total == 2 || total == 3 || total == 12) win = 0;
                    else point = total;
                } else if (choice == 1) {
                    if (total == 2 || total == 3) { win = 1; payout = bet; }
                    else if (total == 7 || total == 11 || total == 12) win = 0;
                    else point = total;
                } else {
                    if (total <= 4 || total >= 9) { win = 1; payout = bet; }
                }
                while (point) {
                    sleep_ms(400);
                    for (i = 0; i < nd; i++) keep[i] = 0;
                    roll_free();
                    total = sum_all();
                    show_dice(4, 10, nd, die, NULL, -1);
                    draw_textf(9, 8, "%spoint %d, rolled %-2d%s", C_WHITE, point, total, C_RESET);
                    scr_flush();
                    if (total == point) { win = (choice == 0); payout = bet; point = 0; }
                    else if (total == 7) { win = (choice == 1); payout = bet; point = 0; }
                }
            } else if (variant == 6) {
                int hits = count_face(choice + 1);
                win = hits > 0;
                payout = bet * hits;
            } else if (variant == 18) {
                int trip = (die[0] == die[1] && die[1] == die[2]);
                if (choice == 0) { win = !trip && total >= 4  && total <= 10; payout = bet; }
                if (choice == 1) { win = !trip && total >= 11 && total <= 17; payout = bet; }
                if (choice == 2) { win = trip; payout = bet * 30; }
                if (choice == 3) { win = total == 9; payout = bet * 6; }
            } else {
                int a = die[0], b = die[1], c = die[2], lo, hi;
                lo = a < b ? (a < c ? a : c) : (b < c ? b : c);
                hi = a > b ? (a > c ? a : c) : (b > c ? b : c);
                if (lo == 4 && hi == 6 && a + b + c == 15) { win = 1; payout = bet * 2;
                    draw_textf(11, 8, "%s4-5-6 — an outright win.%s", C_GREEN, C_RESET); }
                else if (lo == 1 && hi == 3 && a + b + c == 6) { win = 0;
                    draw_textf(11, 8, "%s1-2-3 — an outright loss.%s", C_RED, C_RESET); }
                else if (a == b || b == c || a == c) {
                    int pt = (a == b) ? c : (b == c) ? a : b;
                    win = pt >= 4; payout = bet;
                    draw_textf(11, 8, "%sYour point is %d.%s", C_YELLOW, pt, C_RESET);
                } else { win = 0; draw_textf(11, 8, "%sNo score — a push against the bank.%s", C_GREY, C_RESET); }
            }

            purse += win ? payout : -bet;
            if (purse > best) best = purse;
            draw_textf(13, 8, "%s%s%s  purse now %d%s", win ? C_GREEN : C_RED,
                       win ? "Won " : "Lost", C_RESET, purse);
            scr_flush();
            sleep_ms(650);
        }
        if (purse <= 0) {
            scr_clear();
            draw_centered(12, 80, "The purse is empty.");
            score_report(p->title ? p->title : "dice", best);
            if (!confirm("\n  Stake up again?")) return;
            purse = 100; bet = 10;
        }
    }
    score_report(p->title ? p->title : "dice", best);
}

/* =============================================================== bluffing */

static void play_bluff(const GParams *p, int dudo)
{
    int mine[5], theirs[5], nmine = 5, ntheirs = 5, i;
    int bidn = 0, bidf = 0, myturn, wins = 0;

    for (;;) {
        int k;
        nmine = ntheirs = 5; wins = 0;

        while (nmine > 0 && ntheirs > 0) {
            for (i = 0; i < nmine; i++)   mine[i] = 1 + rnd(6);
            for (i = 0; i < ntheirs; i++) theirs[i] = 1 + rnd(6);
            bidn = 0; bidf = 0;
            myturn = 1;

            for (;;) {
                int actual = 0, chal = 0;
                char sub[120];
                for (i = 0; i < nmine; i++)   if (mine[i] == bidf || (dudo && mine[i] == 1)) actual++;
                for (i = 0; i < ntheirs; i++) if (theirs[i] == bidf || (dudo && theirs[i] == 1)) actual++;

                snprintf(sub, sizeof sub,
                         "Your dice %d, theirs %d — arrows raise the bid, Enter bids, C challenges, Q quits",
                         nmine, ntheirs);
                draw_title(dudo ? "DUDO" : "LIAR'S DICE", sub);
                show_dice(4, 10, nmine, mine, NULL, -1);
                draw_textf(9, 8, "%scurrent bid: %s%d x %d%s", C_GREY,
                           C_YELLOW, bidn, bidf, C_RESET);
                if (dudo) draw_textf(10, 8, "%sones are wild%s", C_GREY, C_RESET);
                draw_textf(12, 8, "%s%s to bid%s", C_WHITE, myturn ? "You" : "They", C_RESET);
                scr_flush();

                if (!myturn) {
                    int mycount = 0;
                    sleep_ms(500);
                    for (i = 0; i < ntheirs; i++) if (theirs[i] == bidf || (dudo && theirs[i] == 1)) mycount++;
                    /* It challenges when the bid outruns what it holds plus a
                     * fair share of the dice it cannot see. */
                    if (bidn > mycount + nmine / 2 + 1) chal = 1;
                    else { bidn++; if (bidn > nmine + ntheirs) chal = 1; }
                    if (!chal) { myturn = 1; continue; }
                    draw_textf(14, 8, "%sThey challenge.%s", C_YELLOW, C_RESET);
                } else {
                    k = key_get();
                    if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "dice", wins); return; }
                    if (k == KEY_UP)    { bidn++; continue; }
                    if (k == KEY_DOWN)  { if (bidn > 0) bidn--; continue; }
                    if (k == KEY_RIGHT) { bidf = bidf % 6 + 1; continue; }
                    if (k == KEY_LEFT)  { bidf = (bidf + 4) % 6 + 1; continue; }
                    if (k == KEY_ENTER) {
                        if (bidn == 0 || bidf == 0) continue;
                        myturn = 0;
                        continue;
                    }
                    if (k != 'c' && k != 'C') continue;
                    draw_textf(14, 8, "%sYou challenge.%s", C_YELLOW, C_RESET);
                }

                /* Someone called: reveal and take a die off the loser. */
                show_dice(16, 10, ntheirs, theirs, NULL, -1);
                draw_textf(21, 8, "%sbid %d x %d, actual %d%s", C_WHITE, bidn, bidf, actual, C_RESET);
                {
                    int bidder_right = (actual >= bidn);
                    int challenger_is_me = myturn;
                    int i_lose = challenger_is_me ? bidder_right : !bidder_right;
                    if (i_lose) { nmine--;   draw_textf(22, 8, "%sYou lose a die.%s", C_RED, C_RESET); }
                    else        { ntheirs--; wins++; draw_textf(22, 8, "%sThey lose a die.%s", C_GREEN, C_RESET); }
                }
                scr_flush();
                sleep_ms(800);
                break;
            }
        }
        score_report(p->title ? p->title : "dice", wins * 100 + nmine * 50);
        scr_clear();
        draw_centered(12, 80, nmine > 0 ? "You took the last die." : "You ran out of dice.");
        if (!confirm("\n  Play again?")) return;
    }
}

/* ================================================================= races */

static void play_race(const GParams *p, int variant)
{
    /* 3 Bunco, 8 Beetle, 7 Mexico, 15 Going to Boston, 19 Klondike Dice */
    int you = 0, cpu = 0, round = 1, target;
    const char *name = variant == 3 ? "BUNCO" : variant == 8 ? "BEETLE" :
                       variant == 7 ? "MEXICO" : variant == 15 ? "GOING TO BOSTON" : "KLONDIKE DICE";
    static const char *PARTS[6] = {"body", "head", "leg", "leg", "wing", "eye"};
    int mypart[6], cpupart[6], i;

    target = (variant == 3) ? 21 : (variant == 8) ? 6 : 5;
    for (i = 0; i < 6; i++) { mypart[i] = 0; cpupart[i] = 0; }

    for (;;) {
        int k, side;
        you = cpu = 0; round = 1;
        for (i = 0; i < 6; i++) { mypart[i] = 0; cpupart[i] = 0; }

        for (;;) {
            char sub[120];
            snprintf(sub, sizeof sub, "Round %d — Space throws, Q quits", round);
            draw_title(name, sub);
            draw_textf(11, 8, "%syou %-4d   opponent %-4d%s", C_WHITE, you, cpu, C_RESET);
            if (variant == 8) {
                for (i = 0; i < 6; i++)
                    draw_textf(13 + i, 8, "%s%-6s you %s  them %s%s", C_GREY, PARTS[i],
                               mypart[i] ? "yes" : " - ", cpupart[i] ? "yes" : " - ", C_RESET);
            }
            scr_flush();

            if ((variant == 8 && (you >= target || cpu >= target)) ||
                (variant != 8 && (you >= target || cpu >= target))) break;

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "dice", you); return; }
            if (k != ' ' && k != KEY_ENTER) continue;

            for (side = 0; side < 2; side++) {
                int gain = 0;
                nd = (variant == 3 || variant == 7) ? (variant == 7 ? 2 : 3) : 1;
                if (variant == 15) nd = 2;
                for (i = 0; i < nd; i++) keep[i] = 0;
                roll_free();

                if (variant == 3) {                    /* Bunco: matches of the round number */
                    int hits = count_face(round > 6 ? 6 : round);
                    gain = (hits == 3) ? 21 : hits;
                } else if (variant == 7) {             /* Mexico: 2-1 beats everything */
                    int a = die[0], b = die[1];
                    if ((a == 2 && b == 1) || (a == 1 && b == 2)) gain = 21;
                    else if (a == b) gain = a * 100 / 10;
                    else gain = (a > b) ? a * 10 + b : b * 10 + a;
                    gain /= 10;
                } else if (variant == 15) {            /* Going to Boston: keep the higher */
                    gain = die[0] > die[1] ? die[0] : die[1];
                } else if (variant == 19) {            /* Klondike Dice: high roll takes it */
                    gain = die[0];
                } else {                               /* Beetle: build the parts in order */
                    int *parts = side ? cpupart : mypart, f = die[0] - 1;
                    if (f == 0 || parts[0]) { if (!parts[f]) { parts[f] = 1; gain = 1; } }
                }
                if (side) cpu += gain; else you += gain;

                draw_title(name, side ? "Opponent throws" : "You throw");
                show_dice(4, 10, nd, die, NULL, -1);
                draw_textf(9, 8, "%s%s scores %d%s", side ? C_YELLOW : C_GREEN,
                           side ? "Opponent" : "You", gain, C_RESET);
                scr_flush();
                sleep_ms(420);
            }
            round++;
        }
        score_report(p->title ? p->title : "dice", you);
        scr_clear();
        draw_centered(12, 80, you >= cpu ? "You win the race." : "The opponent wins.");
        if (!confirm("\n  Play again?")) return;
    }
}

/* =========================================================== Shut the Box */

static void play_shutbox(const GParams *p)
{
    int shut[9], cur, i, best = 99;

    for (;;) {
        int k, total, open;
        for (i = 0; i < 9; i++) shut[i] = 0;
        cur = 0;

        for (;;) {
            /* Two dice while high numbers remain, one once they are shut. */
            int high = 0;
            for (i = 6; i < 9; i++) if (!shut[i]) high = 1;
            nd = high ? 2 : 1;
            for (i = 0; i < nd; i++) keep[i] = 0;
            roll_free();
            total = sum_all();

            for (;;) {
                int chosen = 0, any = 0;
                draw_title("SHUT THE BOX",
                           "Shut any set of flaps adding up to the throw — "
                           "arrows move, Space flips, Enter confirms, Q quits");
                show_dice(4, 14, nd, die, NULL, -1);
                draw_textf(9, 8, "%sthrow %d%s", C_WHITE, total, C_RESET);
                for (i = 0; i < 9; i++) {
                    draw_textf(11, 8 + i * 5, "%s%s %d %s", i == cur ? BG_BLUE : "",
                               shut[i] == 1 ? C_GREY : shut[i] == 2 ? C_YELLOW : C_WHITE,
                               i + 1, C_RESET);
                    draw_textf(12, 8 + i * 5, "%s%s%s", C_GREY,
                               shut[i] == 1 ? "shut" : shut[i] == 2 ? " ^  " : "    ", C_RESET);
                }
                for (i = 0; i < 9; i++) if (shut[i] == 2) chosen += i + 1;
                open = 0;
                for (i = 0; i < 9; i++) if (shut[i] != 1) open += i + 1;
                draw_textf(14, 8, "%sselected %-3d of %-3d   open total %d%s", C_GREY, chosen, total, open, C_RESET);
                scr_flush();

                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "dice", 45 - best); return; }
                if (k == KEY_LEFT)  { cur = (cur + 8) % 9; continue; }
                if (k == KEY_RIGHT) { cur = (cur + 1) % 9; continue; }
                if (k == ' ')       { if (shut[cur] != 1) shut[cur] = shut[cur] == 2 ? 0 : 2; continue; }
                if (k != KEY_ENTER) continue;
                if (chosen != total) continue;
                for (i = 0; i < 9; i++) if (shut[i] == 2) shut[i] = 1;
                any = 1;
                (void)any;
                break;
            }

            open = 0;
            for (i = 0; i < 9; i++) if (!shut[i]) open += i + 1;
            if (open == 0) {
                best = 0;
                scr_clear();
                draw_centered(12, 80, "The box is shut - a perfect game.");
                score_report(p->title ? p->title : "dice", 100);
                break;
            }
            {   /* Stuck when no subset of the open flaps makes the next throw. */
                int reach = 0, mask, s;
                int nexthigh = 0;
                for (i = 6; i < 9; i++) if (!shut[i]) nexthigh = 1;
                for (s = nexthigh ? 2 : 1; s <= (nexthigh ? 12 : 6); s++) {
                    for (mask = 1; mask < 512; mask++) {
                        int t = 0, ok = 1;
                        for (i = 0; i < 9; i++) if (mask & (1 << i)) { if (shut[i]) { ok = 0; break; } t += i + 1; }
                        if (ok && t == s) { reach = 1; break; }
                    }
                    if (reach) break;
                }
                if (!reach) {
                    if (open < best) best = open;
                    scr_clear();
                    draw_centered(12, 80, "No flaps left that match a throw.");
                    score_report(p->title ? p->title : "dice", 45 - open);
                    break;
                }
            }
        }
        if (!confirm("\n  Another box?")) return;
    }
}

/* ---------------------------------------------------------- the dispatch */

void fam_dice(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    if (v < 0 || v > 19) v = 0;
    switch (v) {
    case 10: play_sheet(p, YACHT,  12, "YACHT");  break;
    case 11: play_sheet(p, BALUT,   7, "BALUT");  break;
    case 12: play_sheet(p, KISMET, 13, "KISMET"); break;
    case 0: case 9: case 13: case 14: case 16: play_press(p, v);  break;
    case 1: case 5: case 6:  case 18:          play_wager(p, v);  break;
    case 2:  play_bluff(p, 0); break;
    case 17: play_bluff(p, 1); break;
    case 4:  play_shutbox(p);  break;
    default: play_race(p, v);  break;
    }
}
