/* GIC:PARAMETERISED cardmisc
 * cardmisc.c - the card games that are about what a hand is worth.
 *
 * params: variant selects the game
 *
 * Unlike the shedding and trick families these ten do not share a loop, but
 * they do share a question: given a set of cards, what is it worth? So the
 * valuations are written once and used by several games -
 *
 *   poker_rank    used by Three Card Poker and Five Card Draw
 *   cribbage_score  fifteens, pairs, runs, flush and his nob
 *   meld_value    runs and sets, used by Gin Rummy and Canasta
 *
 * and each game is then a short loop over its own betting or drawing rules.
 */
#include "engine.h"
#include "cards.h"
#include "games.h"

static int rank_of(int c) { return c % 13; }
static int suit_of(int c) { return c / 13; }

static void show(int row, int col, int c, int hi, int down)
{
    char nm[8];
    if (down) { draw_textf(row, col, "%s[##]%s", C_GREY, C_RESET); return; }
    card_name(c, nm, sizeof nm);
    draw_textf(row, col, "%s%s%-4s%s", hi ? BG_BLUE : "", suit_colour(c), nm, C_RESET);
}

/* ------------------------------------------------------------ valuations */

static const char *POKER_NAME[10] = {
    "high card", "a pair", "two pair", "three of a kind", "a straight",
    "a flush", "a full house", "four of a kind", "a straight flush", "a royal flush"
};

/* Returns a category 0-9; ties inside a category are broken by `kick`. */
static int poker_rank(const int *c, int n, int *kick)
{
    int cnt[13], suits[4], i, pairs = 0, three = 0, four = 0;
    int flush, straight = 0, hi = -1, lo = 13, run = 0;

    for (i = 0; i < 13; i++) cnt[i] = 0;
    for (i = 0; i < 4; i++) suits[i] = 0;
    for (i = 0; i < n; i++) { cnt[rank_of(c[i])]++; suits[suit_of(c[i])]++; }
    for (i = 0; i < 13; i++) {
        if (cnt[i] == 2) pairs++;
        if (cnt[i] == 3) three++;
        if (cnt[i] == 4) four++;
        if (cnt[i]) { if (i > hi) hi = i; if (i < lo) lo = i; }
    }
    flush = 0;
    for (i = 0; i < 4; i++) if (suits[i] == n) flush = 1;

    for (i = 0; i < 13; i++) {
        if (cnt[i]) { run++; if (run >= n) straight = 1; }
        else run = 0;
    }
    /* The wheel: ace plays low in A-2-3-4-5. */
    if (!straight && n == 5 && cnt[0] && cnt[1] && cnt[2] && cnt[3] && cnt[12]) { straight = 1; hi = 3; }
    if (!straight && n == 3 && cnt[0] && cnt[1] && cnt[2]) straight = 1;

    *kick = hi;
    if (straight && flush) return (hi == 12) ? 9 : 8;
    if (four) return 7;
    if (three && pairs) return 6;
    if (flush) return 5;
    if (straight) return 4;
    if (three) return 3;
    if (pairs >= 2) return 2;
    if (pairs) return 1;
    return 0;
}

/* Cribbage: every combination totalling fifteen, every pair, every run of
 * three or more, a flush, and the jack matching the turn-up. */
static int cribbage_score(const int *hand4, int turn, char *note, int nn)
{
    int c[5], i, j, k, m, total = 0, fifteens = 0, pairs = 0, runs = 0, flush = 0;
    int cnt[13], best = 0;

    for (i = 0; i < 4; i++) c[i] = hand4[i];
    c[4] = turn;
    for (i = 0; i < 13; i++) cnt[i] = 0;
    for (i = 0; i < 5; i++) cnt[rank_of(c[i])]++;

    /* Fifteens: every subset. Court cards count ten, ace counts one. */
    for (m = 1; m < 32; m++) {
        int sum = 0;
        for (i = 0; i < 5; i++)
            if (m & (1 << i)) { int r = rank_of(c[i]); sum += r >= 9 ? 10 : r + 1; }
        if (sum == 15) fifteens++;
    }
    for (i = 0; i < 5; i++) for (j = i + 1; j < 5; j++)
        if (rank_of(c[i]) == rank_of(c[j])) pairs++;

    /* Runs score once per distinct sequence, multiplied by duplicate ranks. */
    for (i = 0; i < 13; i++) {
        int len = 0, mult = 1;
        for (j = i; j < 13 && cnt[j]; j++) { len++; mult *= cnt[j]; }
        if (len >= 3 && len > best) { best = len; runs = len * mult; i = j; }
    }
    for (i = 0; i < 4; i++) {
        int same = 1;
        for (j = 0; j < 4; j++) if (suit_of(hand4[j]) != suit_of(hand4[0])) same = 0;
        if (same) { flush = (suit_of(turn) == suit_of(hand4[0])) ? 5 : 4; break; }
    }
    for (i = 0; i < 4; i++)
        if (rank_of(hand4[i]) == 10 && suit_of(hand4[i]) == suit_of(turn)) total += 1;   /* his nob */

    k = fifteens * 2 + pairs * 2 + runs + flush;
    total += k;
    snprintf(note, nn, "%d fifteens, %d pairs, run %d, flush %d", fifteens, pairs, runs, flush);
    return total;
}

/* Gin Rummy and Canasta both want "how much of this hand is melded". */
static int meld_value(const int *h, int n, int *deadwood)
{
    int cnt[13], i, melded = 0, dead = 0;
    for (i = 0; i < 13; i++) cnt[i] = 0;
    for (i = 0; i < n; i++) cnt[rank_of(h[i])]++;
    for (i = 0; i < 13; i++) {
        if (cnt[i] >= 3) melded += cnt[i];
        else dead += cnt[i] * (i >= 9 ? 10 : i + 1);
    }
    /* Runs in a suit also count, which sets can miss. */
    for (i = 0; i < 4; i++) {
        int r, run = 0;
        for (r = 0; r < 13; r++) {
            int have = 0, j;
            for (j = 0; j < n; j++) if (suit_of(h[j]) == i && rank_of(h[j]) == r) have = 1;
            if (have) { run++; if (run == 3) melded += 3; else if (run > 3) melded += 1; }
            else run = 0;
        }
    }
    *deadwood = dead;
    return melded;
}

/* ------------------------------------------------------------ the games */

static void play_baccarat(const GParams *p)
{
    int deck[52], pos = 52, purse = 100, bet = 10, choice = 0, best = 100;
    const char *RES[3] = {"Player", "Banker", "Tie"};

    for (;;) {
        int k, i, ph[3], bh[3], pn, bn, pv, bv, win;
        char sub[140];
        snprintf(sub, sizeof sub, "Purse %d — arrows choose, +/- stakes %d, Enter deals, Q quits", purse, bet);
        draw_title("BACCARAT", sub);
        for (i = 0; i < 3; i++)
            draw_textf(5 + i, 12, "%s%sbet on %-8s  pays %s%s", i == choice ? BG_BLUE : "", C_WHITE,
                       RES[i], i == 2 ? "8 to 1" : i == 1 ? "19 to 20" : "even", C_RESET);
        draw_textf(12, 8, "%sbest purse %d%s", C_GREY, best, C_RESET);
        scr_flush();

        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        if (k == KEY_UP)   { choice = (choice + 2) % 3; continue; }
        if (k == KEY_DOWN) { choice = (choice + 1) % 3; continue; }
        if (k == '+' || k == '=') { if (bet + 5 <= purse) bet += 5; continue; }
        if (k == '-')             { if (bet > 5) bet -= 5; continue; }
        if (k != KEY_ENTER) continue;

        if (pos > 46) { deck_init(deck); shuffle_int(deck, 52); pos = 0; }
        pn = bn = 0;
        for (i = 0; i < 2; i++) { ph[pn++] = deck[pos++]; bh[bn++] = deck[pos++]; }
        /* Baccarat counts modulo ten, with courts and tens worth nothing. */
        pv = 0; for (i = 0; i < pn; i++) { int r = rank_of(ph[i]); pv += r >= 9 ? 0 : r + 1; } pv %= 10;
        bv = 0; for (i = 0; i < bn; i++) { int r = rank_of(bh[i]); bv += r >= 9 ? 0 : r + 1; } bv %= 10;
        if (pv <= 5 && pv < 8 && bv < 8) {
            ph[pn++] = deck[pos++];
            pv = 0; for (i = 0; i < pn; i++) { int r = rank_of(ph[i]); pv += r >= 9 ? 0 : r + 1; } pv %= 10;
        }
        if (bv <= 5 && pv < 8 && bv < 8) {
            bh[bn++] = deck[pos++];
            bv = 0; for (i = 0; i < bn; i++) { int r = rank_of(bh[i]); bv += r >= 9 ? 0 : r + 1; } bv %= 10;
        }
        win = pv > bv ? 0 : bv > pv ? 1 : 2;

        draw_title("BACCARAT", "The coup");
        for (i = 0; i < pn; i++) show(5, 10 + i * 5, ph[i], 0, 0);
        for (i = 0; i < bn; i++) show(8, 10 + i * 5, bh[i], 0, 0);
        draw_textf(5, 30, "%splayer %d%s", C_WHITE, pv, C_RESET);
        draw_textf(8, 30, "%sbanker %d%s", C_WHITE, bv, C_RESET);
        if (win == choice) purse += (choice == 2) ? bet * 8 : (choice == 1) ? bet * 19 / 20 : bet;
        else purse -= bet;
        if (purse > best) best = purse;
        draw_textf(11, 8, "%s%s wins — purse now %d%s", win == choice ? C_GREEN : C_RED,
                   RES[win], purse, C_RESET);
        scr_flush();
        sleep_ms(700);
        if (purse <= 0) { purse = 100; bet = 10; }
        if (bet > purse) bet = purse;
    }
    score_report(p->title ? p->title : "baccarat", best);
}

static void play_poker(const GParams *p, int five)
{
    int deck[52], purse = 100, bet = 10, best = 100;

    for (;;) {
        int k, i, n = five ? 5 : 3, mine[5], theirs[5], hold[5];
        int mr, tr, mk, tk, pos = 0;
        char note[80];

        deck_init(deck); shuffle_int(deck, 52);
        for (i = 0; i < n; i++) { mine[i] = deck[pos++]; theirs[i] = deck[pos++]; hold[i] = 0; }

        if (five) {
            int cur = 0;
            for (;;) {
                char sub[150];
                snprintf(sub, sizeof sub,
                         "Purse %d, stake %d — arrows pick, Space holds, Enter draws, Q quits", purse, bet);
                draw_title("FIVE CARD DRAW", sub);
                for (i = 0; i < 5; i++) {
                    show(6, 12 + i * 6, mine[i], i == cur, 0);
                    draw_textf(7, 12 + i * 6, "%s%s%s", C_GREEN, hold[i] ? "held" : "    ", C_RESET);
                }
                mr = poker_rank(mine, 5, &mk);
                draw_textf(9, 10, "%syou hold %s%s", C_GREY, POKER_NAME[mr], C_RESET);
                scr_flush();
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out;
                if (k == KEY_LEFT)  { cur = (cur + 4) % 5; continue; }
                if (k == KEY_RIGHT) { cur = (cur + 1) % 5; continue; }
                if (k == ' ')       { hold[cur] = !hold[cur]; continue; }
                if (k == '+' || k == '=') { if (bet + 5 <= purse) bet += 5; continue; }
                if (k == '-')             { if (bet > 5) bet -= 5; continue; }
                if (k == KEY_ENTER) break;
            }
            for (i = 0; i < 5; i++) if (!hold[i]) mine[i] = deck[pos++];
            /* The opponent redraws whatever is not part of its best holding. */
            tr = poker_rank(theirs, 5, &tk);
            for (i = 0; i < 5; i++) if (tr == 0 && rank_of(theirs[i]) < 9) theirs[i] = deck[pos++];
        } else {
            for (;;) {
                char sub[150];
                snprintf(sub, sizeof sub,
                         "Purse %d — +/- stakes %d, Enter plays the hand, F folds, Q quits", purse, bet);
                draw_title("THREE CARD POKER", sub);
                for (i = 0; i < 3; i++) show(6, 14 + i * 6, mine[i], 0, 0);
                mr = poker_rank(mine, 3, &mk);
                draw_textf(8, 12, "%syou hold %s%s", C_GREY, POKER_NAME[mr], C_RESET);
                scr_flush();
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out;
                if (k == '+' || k == '=') { if (bet + 5 <= purse) bet += 5; continue; }
                if (k == '-')             { if (bet > 5) bet -= 5; continue; }
                if (k == 'f' || k == 'F') { purse -= bet / 2; goto settled; }
                if (k == KEY_ENTER) break;
            }
        }

        mr = poker_rank(mine, n, &mk);
        tr = poker_rank(theirs, n, &tk);
        draw_title(five ? "FIVE CARD DRAW" : "THREE CARD POKER", "The showdown");
        for (i = 0; i < n; i++) show(6,  12 + i * 6, mine[i], 0, 0);
        for (i = 0; i < n; i++) show(10, 12 + i * 6, theirs[i], 0, 0);
        snprintf(note, sizeof note, "you: %s   them: %s", POKER_NAME[mr], POKER_NAME[tr]);
        draw_textf(12, 10, "%s%s%s", C_WHITE, note, C_RESET);
        {
            int win = (mr > tr) || (mr == tr && mk > tk);
            /* Better hands pay more than even money. */
            int mult = mr >= 8 ? 40 : mr >= 6 ? 8 : mr >= 4 ? 4 : mr >= 1 ? 2 : 1;
            if (win) purse += bet * mult; else purse -= bet;
            if (purse > best) best = purse;
            draw_textf(14, 10, "%s%s — purse now %d%s", win ? C_GREEN : C_RED,
                       win ? "You take it" : "They take it", purse, C_RESET);
        }
        scr_flush();
        sleep_ms(800);
settled:
        if (purse <= 0) { purse = 100; bet = 10; }
        if (bet > purse) bet = purse;
    }
out:
    score_report(p->title ? p->title : "poker", best);
}

static void play_cribbage(const GParams *p)
{
    int deck[52], mine[6], theirs[6], keep[4], hold[6], cur = 0, i;
    int mypeg = 0, theirpeg = 0;

    for (;;) {
        int k, pos = 0, nh = 0, turn;
        char note[80];
        deck_init(deck); shuffle_int(deck, 52);
        for (i = 0; i < 6; i++) { mine[i] = deck[pos++]; theirs[i] = deck[pos++]; hold[i] = 0; }
        turn = deck[pos++];
        cur = 0;

        for (;;) {
            char sub[150];
            nh = 0;
            for (i = 0; i < 6; i++) if (hold[i]) nh++;
            snprintf(sub, sizeof sub,
                     "Keep four — arrows pick, Space keeps (%d of 4 chosen), Enter confirms, Q quits", nh);
            draw_title("CRIBBAGE", sub);
            for (i = 0; i < 6; i++) {
                show(6, 12 + i * 6, mine[i], i == cur, 0);
                draw_textf(7, 12 + i * 6, "%s%s%s", C_GREEN, hold[i] ? "keep" : "    ", C_RESET);
            }
            draw_textf(9, 10, "%sturn-up:%s", C_GREY, C_RESET);
            show(9, 20, turn, 0, 0);
            draw_textf(11, 10, "%syou %d — them %d (first to 121)%s", C_WHITE, mypeg, theirpeg, C_RESET);
            scr_flush();
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "cribbage", mypeg); return; }
            if (k == KEY_LEFT)  { cur = (cur + 5) % 6; continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % 6; continue; }
            if (k == ' ')       { if (hold[cur] || nh < 4) hold[cur] = !hold[cur]; continue; }
            if (k == KEY_ENTER && nh == 4) break;
        }

        { int j = 0; for (i = 0; i < 6; i++) if (hold[i]) keep[j++] = mine[i]; }
        {
            int mine_s = cribbage_score(keep, turn, note, sizeof note);
            int their_s = cribbage_score(theirs, turn, note, sizeof note);
            mypeg += mine_s;
            theirpeg += their_s;
            scr_clear();
            draw_textf(8,  8, "%sYour hand scores %d — %s%s", C_GREEN, mine_s, note, C_RESET);
            draw_textf(10, 8, "%sTheirs scores %d%s", C_YELLOW, their_s, C_RESET);
            draw_textf(12, 8, "%sYou %d — them %d%s", C_BOLD, mypeg, theirpeg, C_RESET);
            scr_flush();
        }
        if (mypeg >= 121 || theirpeg >= 121) {
            draw_centered(15, 80, mypeg >= 121 ? "You peg out." : "They peg out.");
            score_report(p->title ? p->title : "cribbage", mypeg);
            if (!confirm("\n  Another game?")) return;
            mypeg = theirpeg = 0;
        } else if (!confirm("\n  Another hand?")) {
            score_report(p->title ? p->title : "cribbage", mypeg);
            return;
        }
        for (i = 0; i < 6; i++) hold[i] = 0;
    }
}

static void play_meld(const GParams *p, int canasta)
{
    int deck[52], mine[20], mn, theirs[20], tn2, pos, cur = 0, i;
    int myscore = 0, theirscore = 0;

    for (;;) {
        int k, dead, melded, over = 0;
        deck_init(deck); shuffle_int(deck, 52);
        pos = 0; mn = 0; tn2 = 0;
        for (i = 0; i < (canasta ? 13 : 10); i++) { mine[mn++] = deck[pos++]; theirs[tn2++] = deck[pos++]; }
        cur = 0;

        while (!over) {
            char sub[150];
            melded = meld_value(mine, mn, &dead);
            snprintf(sub, sizeof sub,
                     "%s — arrows pick, D draws, Space discards, K knocks (deadwood %d), Q quits",
                     canasta ? "Canasta" : "Gin Rummy", dead);
            draw_title(canasta ? "CANASTA" : "GIN RUMMY", sub);
            for (i = 0; i < mn && i < 14; i++) show(6, 5 + i * 5, mine[i], i == cur, 0);
            draw_textf(8, 5, "%smelded %d cards, deadwood %d%s", C_GREY, melded, dead, C_RESET);
            draw_textf(10, 5, "%sopponent holds %d   stock %d%s", C_GREY, tn2, 52 - pos, C_RESET);
            draw_textf(12, 5, "%syou %d — them %d%s", C_WHITE, myscore, theirscore, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "meld", myscore); return; }
            if (k == KEY_LEFT)  { if (mn) cur = (cur + mn - 1) % mn; continue; }
            if (k == KEY_RIGHT) { if (mn) cur = (cur + 1) % mn; continue; }
            if (k == 'd' || k == 'D') {
                if (pos < 52 && mn < 20) mine[mn++] = deck[pos++];
                if (pos < 52 && tn2 < 20) theirs[tn2++] = deck[pos++];
                continue;
            }
            if (k == ' ') {
                if (mn <= 1) continue;
                for (i = cur; i < mn - 1; i++) mine[i] = mine[i + 1];
                mn--;
                if (cur >= mn && cur > 0) cur--;
                continue;
            }
            if (k == 'k' || k == 'K') {
                int theirdead;
                meld_value(theirs, tn2, &theirdead);
                if (dead > (canasta ? 20 : 10)) continue;   /* you may not knock yet */
                myscore    += theirdead - dead > 0 ? theirdead - dead : 0;
                theirscore += dead > theirdead ? dead - theirdead : 0;
                over = 1;
            }
            if (pos >= 52) over = 1;
        }
        scr_clear();
        draw_centered(12, 80, myscore > theirscore ? "You are ahead." : "They are ahead.");
        score_report(p->title ? p->title : "meld", myscore);
        if (!confirm("\n  Another hand?")) return;
    }
}

static void play_casino(const GParams *p, int golf)
{
    /* Casino captures table cards by value; Golf keeps the lowest six. */
    int deck[52], table[16], tn, mine[6], mn, pos, cur = 0, i, captured = 0, best = 999;

    for (;;) {
        int k, over = 0, shown[6];
        deck_init(deck); shuffle_int(deck, 52);
        pos = 0; tn = 0; mn = 0; captured = 0;
        if (golf) { for (i = 0; i < 6; i++) { mine[mn++] = deck[pos++]; shown[i] = i < 2; } }
        else { for (i = 0; i < 4; i++) table[tn++] = deck[pos++];
               for (i = 0; i < 4; i++) mine[mn++] = deck[pos++]; }

        while (!over) {
            char sub[150];
            snprintf(sub, sizeof sub, golf
                     ? "Swap drawn cards for high ones — arrows pick, Enter swaps, P passes, Q quits"
                     : "Capture table cards of the same rank — arrows pick, Enter plays, Q quits");
            draw_title(golf ? "GOLF CARD GAME" : "CASINO", sub);
            if (golf) {
                for (i = 0; i < 6; i++) show(6 + (i / 3) * 3, 14 + (i % 3) * 6, mine[i], i == cur, !shown[i]);
                draw_textf(12, 10, "%sstock %d — lowest total wins%s", C_GREY, 52 - pos, C_RESET);
                if (pos < 52) { draw_textf(14, 10, "%sdrawn:%s", C_GREY, C_RESET); show(14, 18, deck[pos], 0, 0); }
            } else {
                draw_textf(4, 8, "%stable:%s", C_GREY, C_RESET);
                for (i = 0; i < tn; i++) show(5, 8 + i * 5, table[i], 0, 0);
                draw_textf(8, 8, "%scaptured %d%s", C_GREY, captured, C_RESET);
                draw_textf(10, 8, "%syour hand:%s", C_GREY, C_RESET);
                for (i = 0; i < mn; i++) show(11, 8 + i * 5, mine[i], i == cur, 0);
            }
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
            if (k == KEY_LEFT)  { cur = (cur + (golf ? 5 : mn - 1)) % (golf ? 6 : (mn ? mn : 1)); continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % (golf ? 6 : (mn ? mn : 1)); continue; }
            if (k == 'p' || k == 'P') { if (pos < 52) pos++; if (pos >= 52) over = 1; continue; }
            if (k != KEY_ENTER) continue;

            if (golf) {
                if (pos >= 52) { over = 1; continue; }
                mine[cur] = deck[pos++];
                shown[cur] = 1;
                { int all = 1; for (i = 0; i < 6; i++) if (!shown[i]) all = 0; if (all || pos >= 52) over = 1; }
            } else {
                int hit = -1;
                if (!mn) { over = 1; continue; }
                for (i = 0; i < tn; i++) if (rank_of(table[i]) == rank_of(mine[cur])) { hit = i; break; }
                if (hit >= 0) {
                    captured += 2;
                    for (i = hit; i < tn - 1; i++) table[i] = table[i + 1];
                    tn--;
                } else if (tn < 16) table[tn++] = mine[cur];
                for (i = cur; i < mn - 1; i++) mine[i] = mine[i + 1];
                mn--;
                if (cur >= mn && cur > 0) cur--;
                if (mn == 0) {
                    if (pos + 4 <= 52) { for (i = 0; i < 4; i++) mine[mn++] = deck[pos++]; }
                    else over = 1;
                }
            }
        }
        scr_clear();
        if (golf) {
            int total = 0;
            for (i = 0; i < 6; i++) { int r = rank_of(mine[i]); total += r == 12 ? 0 : r >= 9 ? 10 : r + 1; }
            if (total < best) best = total;
            draw_textf(12, 20, "%sYour six total %d (lower is better, best %d)%s", C_BOLD, total, best, C_RESET);
            score_report(p->title ? p->title : "golf", 200 - total);
        } else {
            draw_textf(12, 20, "%sYou captured %d cards.%s", C_BOLD, captured, C_RESET);
            score_report(p->title ? p->title : "casino", captured * 10);
        }
        scr_flush();
        if (!confirm("\n  Another deal?")) return;
    }
}

static void play_switch(const GParams *p)
{
    /* Blackjack Switch: two hands, and you may swap the second cards. */
    int deck[52], h[2][8], n[2], dealer[8], dn, pos, purse = 100, bet = 10, best = 100;

    for (;;) {
        int k, i, j, swapped = 0, done = 0;
        deck_init(deck); shuffle_int(deck, 52); pos = 0;
        n[0] = n[1] = 0; dn = 0;
        for (i = 0; i < 2; i++) for (j = 0; j < 2; j++) h[j][n[j]++] = deck[pos++];
        for (i = 0; i < 2; i++) dealer[dn++] = deck[pos++];

        while (!done) {
            int v[2], soft[2];
            char sub[150];
            for (j = 0; j < 2; j++) {
                int aces = 0;
                v[j] = 0;
                for (i = 0; i < n[j]; i++) {
                    int r = rank_of(h[j][i]);
                    if (r == 0) { aces++; v[j] += 11; }
                    else v[j] += r >= 9 ? 10 : r + 1;
                }
                while (v[j] > 21 && aces) { v[j] -= 10; aces--; }
                soft[j] = aces;
            }
            snprintf(sub, sizeof sub,
                     "Purse %d, stake %d — S switches the second cards, 1/2 hits that hand, Enter stands, Q quits",
                     purse, bet);
            draw_title("BLACKJACK SWITCH", sub);
            draw_textf(4, 8, "%sdealer:%s", C_GREY, C_RESET);
            for (i = 0; i < dn; i++) show(5, 8 + i * 5, dealer[i], 0, i == 1 && !done);
            for (j = 0; j < 2; j++) {
                draw_textf(8 + j * 3, 8, "%shand %d (%d)%s", C_WHITE, j + 1, v[j], C_RESET);
                for (i = 0; i < n[j]; i++) show(9 + j * 3, 22 + i * 5, h[j][i], 0, 0);
            }
            draw_textf(15, 8, "%s%s%s", C_GREY, swapped ? "switched" : "not switched", C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out2;
            if (k == 's' || k == 'S') {
                if (!swapped) { int t = h[0][1]; h[0][1] = h[1][1]; h[1][1] = t; swapped = 1; }
                continue;
            }
            if (k == '+' || k == '=') { if (bet + 5 <= purse) bet += 5; continue; }
            if (k == '-')             { if (bet > 5) bet -= 5; continue; }
            if (k == '1' || k == '2') { int t = k - '1'; if (n[t] < 8 && v[t] < 21) h[t][n[t]++] = deck[pos++]; continue; }
            if (k == KEY_ENTER) done = 1;
            (void)soft;
        }
        {
            int dv = 0, aces = 0, i2, j2, net = 0;
            for (i2 = 0; i2 < dn; i2++) {
                int r = rank_of(dealer[i2]);
                if (r == 0) { aces++; dv += 11; } else dv += r >= 9 ? 10 : r + 1;
            }
            while (dv > 21 && aces) { dv -= 10; aces--; }
            while (dv < 17) {
                int r;
                dealer[dn++] = deck[pos++];
                r = rank_of(dealer[dn - 1]);
                dv += r == 0 ? 11 : r >= 9 ? 10 : r + 1;
                if (r == 0) aces++;
                while (dv > 21 && aces) { dv -= 10; aces--; }
            }
            for (j2 = 0; j2 < 2; j2++) {
                int v2 = 0, a2 = 0;
                for (i2 = 0; i2 < n[j2]; i2++) {
                    int r = rank_of(h[j2][i2]);
                    if (r == 0) { a2++; v2 += 11; } else v2 += r >= 9 ? 10 : r + 1;
                }
                while (v2 > 21 && a2) { v2 -= 10; a2--; }
                /* In Switch a dealer 22 pushes rather than busts. */
                if (v2 > 21) net -= bet / 2;
                else if (dv == 22) net += 0;
                else if (dv > 21 || v2 > dv) net += bet / 2;
                else if (v2 < dv) net -= bet / 2;
            }
            purse += net;
            if (purse > best) best = purse;
            draw_textf(17, 8, "%sdealer %d — you %s %d, purse %d%s", net >= 0 ? C_GREEN : C_RED,
                       dv, net >= 0 ? "win" : "lose", net < 0 ? -net : net, purse, C_RESET);
            scr_flush();
            sleep_ms(800);
        }
        if (purse <= 0) { purse = 100; bet = 10; }
        if (bet > purse) bet = purse;
    }
out2:
    score_report(p->title ? p->title : "switch", best);
}

void fam_cardmisc(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    if (v < 0 || v > 9) v = 0;
    switch (v) {
    case 0: play_baccarat(p);   break;
    case 1: play_cribbage(p);   break;
    case 2: play_meld(p, 0);    break;
    case 3: play_meld(p, 1);    break;
    case 4: play_meld(p, 1);    break;   /* Pinochle: melds, played with a doubled deck feel */
    case 5: play_casino(p, 0);  break;
    case 6: play_casino(p, 1);  break;
    case 7: play_switch(p);     break;
    case 8: play_poker(p, 0);   break;
    default: play_poker(p, 1);  break;
    }
}
