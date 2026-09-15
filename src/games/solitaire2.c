/* solitaire2.c - the eleven patiences that are not tableau games.
 *
 * The family marker lives in solitaire.c; this file holds the variants whose
 * mechanism is something other than "build down, ship to foundations":
 *
 *   pairing        Pyramid removes pairs summing to thirteen
 *   ladder         TriPeaks and Golf take cards one rank either side of a waste
 *   elimination    Aces Up discards to the highest card of a suit
 *   compaction     Accordion folds a row of fifty-two down towards one pile
 *   dealing        Clock Patience and Garbage have no choices, only a verdict
 *   arithmetic     Calculation builds four foundations at four different steps
 *   opposed        Kings Corner, Nertz and Spit are played against someone
 *
 * Each of these needed its own loop, so each has one.
 */
#include "engine.h"
#include "cards.h"
#include "games.h"

static int rank_of(int c) { return c % 13; }
static int suit_of(int c) { return (c / 13) % 4; }
static int red_of(int c)  { int s = suit_of(c); return s == 1 || s == 2; }

static void put_card(int row, int col, int card, int hi, int gone)
{
    char nm[8];
    if (gone)      { draw_textf(row, col, "    "); return; }
    if (card < 0)  { draw_textf(row, col, "%s%s[  ]%s", hi ? BG_BLUE : "", C_GREY, C_RESET); return; }
    card_name(card, nm, sizeof nm);
    draw_textf(row, col, "%s%s%-4s%s", hi ? BG_BLUE : "", suit_colour(card), nm, C_RESET);
}

/* ================================================================ Pyramid */
/* Twenty-eight cards in a triangle. A card is exposed once both cards resting
 * on it have gone. Kings leave alone; everything else leaves in pairs that
 * total thirteen, counting ace as one. */

static void play_pyramid(const GParams *p)
{
    int deck[52], pyr[28], gone[28], stock[52], sn, waste[52], wn;
    int cur, sel, i, r, c, cleared, redeals;
    const char *msg;

    for (;;) {
        deck_init(deck); shuffle_int(deck, 52);
        for (i = 0; i < 28; i++) { pyr[i] = deck[i]; gone[i] = 0; }
        sn = 0; wn = 0;
        for (i = 28; i < 52; i++) stock[sn++] = deck[i];
        cur = 27; sel = -1; cleared = 0; redeals = 0; msg = NULL;

        for (;;) {
            int k, exposed;
            draw_title("PYRAMID SOLITAIRE",
                       "Pairs totalling thirteen — arrows move, Enter selects, "
                       "D draws, Q quits");
            for (r = 0; r < 7; r++) {
                int base = r * (r + 1) / 2;
                for (c = 0; c <= r; c++) {
                    int idx = base + c;
                    put_card(4 + r * 2, 26 - r * 3 + c * 6, pyr[idx],
                             idx == cur || idx == sel, gone[idx]);
                }
            }
            draw_textf(19, 8, "%sstock %-3d%s", C_GREY, sn, C_RESET);
            put_card(19, 20, wn ? waste[wn - 1] : -1, cur == 28 || sel == 28, 0);
            draw_textf(19, 30, "%scleared %d/28%s", C_GREY, cleared, C_RESET);
            draw_textf(21, 8, "%s%-70s%s", C_YELLOW, msg ? msg : "", C_RESET);
            scr_flush();
            msg = NULL;

            if (cleared == 28) {
                score_report(p->title ? p->title : "pyramid", 1000 - redeals * 100);
                if (!confirm("\n  Pyramid cleared! Again?")) return;
                break;
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  { cur = (cur + 28) % 29; continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % 29;  continue; }
            if (k == KEY_UP)    { cur = cur >= 28 ? 27 : (cur > 0 ? (cur - 1) / 2 : 28); continue; }
            if (k == KEY_DOWN)  { cur = cur < 28 ? 28 : 27; continue; }
            if (k == 'd' || k == 'D' || k == ' ') {
                if (sn > 0) waste[wn++] = stock[--sn];
                else if (wn > 0 && redeals < 2) { while (wn) stock[sn++] = waste[--wn]; redeals++; }
                else msg = "No draws left.";
                continue;
            }
            if (k != KEY_ENTER) continue;

            /* A pyramid card is playable only when nothing rests on it. */
            exposed = 1;
            if (cur < 28) {
                r = 0; while ((r + 1) * (r + 2) / 2 <= cur) r++;
                c = cur - r * (r + 1) / 2;
                if (r < 6) {
                    int b = (r + 1) * (r + 2) / 2;
                    if (!gone[b + c] || !gone[b + c + 1]) exposed = 0;
                }
                if (gone[cur]) exposed = 0;
            } else if (wn == 0) exposed = 0;

            if (!exposed) { msg = "That card is still covered."; sel = -1; continue; }

            {
                int card = cur < 28 ? pyr[cur] : waste[wn - 1];
                if (rank_of(card) == 12) {          /* a king goes alone */
                    if (cur < 28) { gone[cur] = 1; cleared++; } else wn--;
                    sel = -1;
                    continue;
                }
                if (sel < 0) { sel = cur; msg = "Now pick its partner."; continue; }
                if (sel == cur) { sel = -1; continue; }
                {
                    int other = sel < 28 ? pyr[sel] : waste[wn - 1];
                    if (rank_of(card) + rank_of(other) + 2 != 13) {
                        msg = "Those two do not total thirteen.";
                        sel = -1;
                        continue;
                    }
                    if (sel < 28) { gone[sel] = 1; cleared++; } else wn--;
                    if (cur < 28) { gone[cur] = 1; cleared++; } else wn--;
                    sel = -1;
                }
            }
        }
    }
}

/* ======================================================== TriPeaks / Golf */
/* Both take a card one rank away from the top of the waste. TriPeaks uncovers
 * a three-peak layout and wraps ace to king; Golf works off seven flat piles
 * and does not wrap, which is what makes it the harder of the two. */

static void play_ladder(const GParams *p, int tripeaks)
{
    int deck[52], lay[28], gone[28], pile[7][52], pn[7];
    int stock[52], sn, waste[52], wn, cur, cleared, streak, best, score;
    const char *msg;

    for (;;) {
        int i, k;
        deck_init(deck); shuffle_int(deck, 52);
        cleared = 0; streak = 0; best = 0; score = 0; sn = 0; wn = 0; cur = 0; msg = NULL;
        if (tripeaks) {
            for (i = 0; i < 28; i++) { lay[i] = deck[i]; gone[i] = 0; }
            for (i = 28; i < 52; i++) stock[sn++] = deck[i];
        } else {
            int pos = 0;
            for (i = 0; i < 7; i++) { int j; pn[i] = 0; for (j = 0; j < 5; j++) pile[i][pn[i]++] = deck[pos++]; }
            while (pos < 52) stock[sn++] = deck[pos++];
        }
        waste[wn++] = stock[--sn];

        for (;;) {
            int total = tripeaks ? 28 : 35;
            draw_title(tripeaks ? "TRIPEAKS" : "GOLF SOLITAIRE",
                       tripeaks ? "One rank either way, aces wrap — arrows move, Enter takes, D deals, Q quits"
                                : "One rank either way, no wrap — arrows move, Enter takes, D deals, Q quits");
            if (tripeaks) {
                static const int ROWLEN[4] = {3, 6, 9, 10};
                int r, c, base = 0;
                for (r = 0; r < 4; r++) {
                    for (c = 0; c < ROWLEN[r]; c++) {
                        int idx = base + c, col;
                        if (r == 0)      col = 12 + c * 20;
                        else if (r == 1) col = 8 + (c / 2) * 20 + (c % 2) * 8;
                        else if (r == 2) col = 6 + (c / 3) * 20 + (c % 3) * 6;
                        else             col = 4 + c * 6;
                        put_card(4 + r * 2, col, lay[idx], idx == cur, gone[idx]);
                    }
                    base += ROWLEN[r];
                }
            } else {
                for (i = 0; i < 7; i++) {
                    int j;
                    draw_textf(4, 6 + i * 7, "%s%s %d %s", i == cur ? BG_BLUE : "", C_BOLD, i + 1, C_RESET);
                    for (j = 0; j < pn[i]; j++)
                        put_card(5 + j, 6 + i * 7, pile[i][j], i == cur && j == pn[i] - 1, 0);
                    if (pn[i] == 0) put_card(5, 6 + i * 7, -1, i == cur, 0);
                }
            }
            draw_textf(14, 6, "%sstock %-3d%s", C_GREY, sn, C_RESET);
            put_card(14, 18, wn ? waste[wn - 1] : -1, 0, 0);
            draw_textf(14, 26, "%scleared %d/%d  streak %d (best %d)  score %d%s",
                       C_GREY, cleared, total, streak, best, score, C_RESET);
            draw_textf(16, 6, "%s%-70s%s", C_YELLOW, msg ? msg : "", C_RESET);
            scr_flush();
            msg = NULL;

            if (cleared == total) {
                score += 500;
                score_report(p->title ? p->title : "ladder", score);
                if (!confirm("\n  Board cleared! Again?")) return;
                break;
            }
            if (sn == 0) {
                /* Out of deals: check whether any card is still takeable. */
                int any = 0, w = waste[wn - 1];
                if (tripeaks) {
                    for (i = 0; i < 28; i++) {
                        int r = 0, c, ok = 1, b;
                        if (gone[i]) continue;
                        if (i >= 3 + 6 + 9) r = 3; else if (i >= 3 + 6) r = 2; else if (i >= 3) r = 1;
                        c = i - (r == 0 ? 0 : r == 1 ? 3 : r == 2 ? 9 : 18);
                        if (r == 0)      { b = 3 + 2 * c;       ok = gone[b] && gone[b + 1]; }
                        else if (r == 1) { b = 3 + 6 + 3 * (c / 2) + (c % 2); ok = gone[b] && gone[b + 1]; }
                        else if (r == 2) { b = 18 + c;          ok = gone[b] && gone[b + 1]; }
                        if (!ok) continue;
                        { int d = (rank_of(lay[i]) - rank_of(w) + 13) % 13; if (d == 1 || d == 12) any = 1; }
                    }
                } else {
                    for (i = 0; i < 7; i++) {
                        int d;
                        if (pn[i] == 0) continue;
                        d = rank_of(pile[i][pn[i] - 1]) - rank_of(w);
                        if (d == 1 || d == -1) any = 1;
                    }
                }
                if (!any) {
                    draw_textf(18, 6, "%sNo moves left — %d cards stranded.%s", C_RED, total - cleared, C_RESET);
                    scr_flush();
                    score_report(p->title ? p->title : "ladder", score);
                    if (!confirm("\n  Stuck. Deal again?")) return;
                    break;
                }
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  { cur = (cur + (tripeaks ? 27 : 6)) % (tripeaks ? 28 : 7); continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % (tripeaks ? 28 : 7); continue; }
            if (k == KEY_UP && tripeaks)   { cur = cur >= 10 ? cur - 10 : cur; continue; }
            if (k == KEY_DOWN && tripeaks) { cur = cur + 10 < 28 ? cur + 10 : cur; continue; }
            if (k == 'd' || k == 'D' || k == ' ') {
                if (sn == 0) { msg = "The stock is empty."; continue; }
                waste[wn++] = stock[--sn];
                streak = 0;
                continue;
            }
            if (k != KEY_ENTER) continue;

            {
                int card, d;
                if (tripeaks) {
                    int r = 0, c, b, ok = 1;
                    if (gone[cur]) { msg = "Already taken."; continue; }
                    if (cur >= 18) r = 3; else if (cur >= 9) r = 2; else if (cur >= 3) r = 1;
                    c = cur - (r == 0 ? 0 : r == 1 ? 3 : r == 2 ? 9 : 18);
                    if (r == 0)      { b = 3 + 2 * c;                     ok = gone[b] && gone[b + 1]; }
                    else if (r == 1) { b = 9 + 3 * (c / 2) + (c % 2);     ok = gone[b] && gone[b + 1]; }
                    else if (r == 2) { b = 18 + c;                        ok = gone[b] && gone[b + 1]; }
                    if (!ok) { msg = "That card is still covered."; continue; }
                    card = lay[cur];
                } else {
                    if (pn[cur] == 0) { msg = "That pile is empty."; continue; }
                    card = pile[cur][pn[cur] - 1];
                }
                d = rank_of(card) - rank_of(waste[wn - 1]);
                if (tripeaks) { int w = (rank_of(card) - rank_of(waste[wn - 1]) + 13) % 13;
                                if (w != 1 && w != 12) { msg = "Not one rank away."; continue; } }
                else if (d != 1 && d != -1) { msg = "Not one rank away (Golf does not wrap)."; continue; }

                waste[wn++] = card;
                if (tripeaks) gone[cur] = 1; else pn[cur]--;
                cleared++;
                streak++;
                if (streak > best) best = streak;
                score += streak * 10;          /* runs are where the points are */
            }
        }
    }
}

/* ================================================================ Aces Up */
/* Four piles dealt four at a time. A card goes to the bin when another pile
 * shows a higher card of its own suit; the aces, being unbeatable, survive. */

static void play_acesup(const GParams *p)
{
    int deck[52], pile[4][52], pn[4], sn, pos, cur, i;
    const char *msg;

    for (;;) {
        int k;
        deck_init(deck); shuffle_int(deck, 52);
        for (i = 0; i < 4; i++) pn[i] = 0;
        pos = 0; cur = 0; msg = NULL;
        for (i = 0; i < 4; i++) pile[i][pn[i]++] = deck[pos++];
        sn = 52 - pos;

        for (;;) {
            int left = 0;
            draw_title("ACES UP",
                       "Discard any card beaten by its own suit — arrows move, "
                       "Enter discards, M moves to a space, D deals, Q quits");
            for (i = 0; i < 4; i++) {
                int j;
                draw_textf(4, 12 + i * 10, "%s%s pile %d %s", i == cur ? BG_BLUE : "",
                           C_BOLD, i + 1, C_RESET);
                for (j = 0; j < pn[i]; j++)
                    put_card(5 + j, 12 + i * 10, pile[i][j], i == cur && j == pn[i] - 1, 0);
                if (pn[i] == 0) put_card(5, 12 + i * 10, -1, i == cur, 0);
                left += pn[i];
            }
            draw_textf(18, 8, "%sstock %-3d   cards left %-3d%s", C_GREY, sn, left, C_RESET);
            draw_textf(20, 8, "%s%-70s%s", C_YELLOW, msg ? msg : "", C_RESET);
            scr_flush();
            msg = NULL;

            if (left == 4 && sn == 0) {
                int aces = 0;
                for (i = 0; i < 4; i++) if (pn[i] == 1 && rank_of(pile[i][0]) == 0) aces++;
                if (aces == 4) {
                    score_report(p->title ? p->title : "acesup", 1000);
                    if (!confirm("\n  Four aces! Again?")) return;
                    break;
                }
            }
            if (sn == 0) {
                int any = 0;
                for (i = 0; i < 4; i++) {
                    int j;
                    if (pn[i] == 0) { any = 1; break; }
                    for (j = 0; j < 4; j++)
                        if (j != i && pn[j] && suit_of(pile[i][pn[i]-1]) == suit_of(pile[j][pn[j]-1]) &&
                            rank_of(pile[j][pn[j]-1]) > rank_of(pile[i][pn[i]-1])) any = 1;
                }
                if (!any) {
                    score_report(p->title ? p->title : "acesup", (52 - left) * 10);
                    if (!confirm("\n  Stuck. Deal again?")) return;
                    break;
                }
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  { cur = (cur + 3) % 4; continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % 4; continue; }
            if (k == 'd' || k == 'D' || k == ' ') {
                if (sn == 0) { msg = "The stock is empty."; continue; }
                for (i = 0; i < 4 && sn > 0; i++) { pile[i][pn[i]++] = deck[pos++]; sn--; }
                continue;
            }
            if (k == 'm' || k == 'M') {
                int t = -1;
                if (pn[cur] == 0) { msg = "Nothing to move."; continue; }
                for (i = 0; i < 4; i++) if (pn[i] == 0) t = i;
                if (t < 0) { msg = "No empty pile to move into."; continue; }
                pile[t][pn[t]++] = pile[cur][--pn[cur]];
                continue;
            }
            if (k != KEY_ENTER) continue;
            if (pn[cur] == 0) { msg = "That pile is empty."; continue; }
            {
                int beaten = 0, c = pile[cur][pn[cur] - 1];
                for (i = 0; i < 4; i++)
                    if (i != cur && pn[i] && suit_of(pile[i][pn[i] - 1]) == suit_of(c)) {
                        int hr = rank_of(pile[i][pn[i] - 1]), mr = rank_of(c);
                        /* Aces are high in this game, so rank 0 beats everything. */
                        if (hr == 0 || (mr != 0 && hr > mr)) beaten = 1;
                    }
                if (!beaten) { msg = "Nothing of that suit beats it."; continue; }
                pn[cur]--;
            }
        }
    }
}

/* ============================================================== Accordion */
/* Fifty-two piles in a row, folded leftwards onto a neighbour or the pile
 * three back whenever the top cards share a suit or a rank. */

static void play_accordion(const GParams *p)
{
    int deck[52], top[52], size[52], n, cur, i;
    const char *msg;

    for (;;) {
        int k;
        deck_init(deck); shuffle_int(deck, 52);
        for (i = 0; i < 52; i++) { top[i] = deck[i]; size[i] = 1; }
        n = 52; cur = 0; msg = NULL;

        for (;;) {
            draw_title("ACCORDION",
                       "Fold onto the pile beside it or three back — arrows move, "
                       "Enter folds left, 3 folds three back, Q quits");
            for (i = 0; i < n; i++) {
                int row = 4 + (i / 13) * 3, col = 6 + (i % 13) * 5;
                put_card(row, col, top[i], i == cur, 0);
                if (size[i] > 1) draw_textf(row + 1, col, "%s x%-2d%s", C_GREY, size[i], C_RESET);
            }
            draw_textf(17, 6, "%spiles %-3d (one pile wins)%s", C_GREY, n, C_RESET);
            draw_textf(19, 6, "%s%-70s%s", C_YELLOW, msg ? msg : "", C_RESET);
            scr_flush();
            msg = NULL;

            if (n == 1) {
                score_report(p->title ? p->title : "accordion", 2000);
                if (!confirm("\n  One pile! Again?")) return;
                break;
            }
            {   /* Stuck when no fold is legal anywhere. */
                int any = 0;
                for (i = 1; i < n; i++) {
                    if (suit_of(top[i]) == suit_of(top[i - 1]) || rank_of(top[i]) == rank_of(top[i - 1])) any = 1;
                    if (i >= 3 && (suit_of(top[i]) == suit_of(top[i - 3]) || rank_of(top[i]) == rank_of(top[i - 3]))) any = 1;
                }
                if (!any) {
                    score_report(p->title ? p->title : "accordion", (52 - n) * 20);
                    if (!confirm("\n  No folds left. Again?")) return;
                    break;
                }
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  { cur = (cur + n - 1) % n; continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % n; continue; }
            if (k == KEY_UP)    { cur = cur >= 13 ? cur - 13 : cur; continue; }
            if (k == KEY_DOWN)  { cur = cur + 13 < n ? cur + 13 : cur; continue; }
            if (k == KEY_ENTER || k == '1' || k == '3') {
                int back = (k == '3') ? 3 : 1, dst = cur - back;
                if (dst < 0) { msg = "Nothing that far back."; continue; }
                if (suit_of(top[cur]) != suit_of(top[dst]) && rank_of(top[cur]) != rank_of(top[dst])) {
                    msg = "They share neither suit nor rank.";
                    continue;
                }
                size[dst] += size[cur];
                top[dst] = top[cur];
                for (i = cur; i < n - 1; i++) { top[i] = top[i + 1]; size[i] = size[i + 1]; }
                n--;
                if (cur >= n) cur = n - 1;
            }
        }
    }
}

/* ========================================================= Clock Patience */
/* Thirteen piles arranged as a clock face. The game plays itself: you turn a
 * card, it goes under its own hour, and you turn the next from there. It ends
 * when the fourth king appears — which is a win only if nothing is left. */

static void play_clock(const GParams *p)
{
    int deck[52], pile[13][4], pn[13], up[13][4], un[13], placed, kings, cur, i, j;

    for (;;) {
        int k;
        deck_init(deck); shuffle_int(deck, 52);
        for (i = 0; i < 13; i++) { pn[i] = 0; un[i] = 0; }
        for (i = 0; i < 52; i++) pile[i % 13][pn[i % 13]++] = deck[i];
        placed = 0; kings = 0; cur = 12;

        for (;;) {
            static const int CR[13] = {4,5,7,9,11,12,11,9,7,5,4,3,8};
            static const int CC[13] = {40,50,57,60,57,50,40,30,23,20,23,30,40};
            draw_title("CLOCK PATIENCE",
                       "The cards decide — press Space to turn the next, Q quits");
            for (i = 0; i < 13; i++) {
                draw_textf(CR[i], CC[i], "%s%s%2d%s", i == cur ? BG_BLUE : "", C_GREY,
                           i == 12 ? 13 : i + 1, C_RESET);
                for (j = 0; j < un[i]; j++)
                    put_card(CR[i], CC[i] + 3 + j * 5, up[i][j], 0, 0);
                if (un[i] == 0) draw_textf(CR[i], CC[i] + 3, "%s[%d]%s", C_GREY, pn[i], C_RESET);
            }
            draw_textf(15, 8, "%splaced %-2d/52   kings shown %d/4%s", C_GREY, placed, kings, C_RESET);
            scr_flush();

            if (kings == 4 || placed == 52) {
                int won = (placed == 52);
                draw_textf(17, 8, "%s%s%s", won ? C_GREEN : C_RED,
                           won ? "Every card home — a one-in-eighty win." :
                                 "The fourth king came up too soon.", C_RESET);
                scr_flush();
                score_report(p->title ? p->title : "clock", placed * 10);
                if (!confirm("\n  Deal again?")) return;
                break;
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k != ' ' && k != KEY_ENTER) continue;
            if (pn[cur] == 0) { cur = (cur + 1) % 13; continue; }
            {
                int card = pile[cur][--pn[cur]], home = rank_of(card);
                if (un[home] < 4) up[home][un[home]++] = card;
                placed++;
                if (home == 12) kings++;
                cur = home;
            }
        }
    }
}

/* ============================================================ Calculation */
/* Four foundations, each climbing by a different step: ace by ones, two by
 * twos, three by threes, four by fours, all modulo thirteen. Four waste piles
 * hold what you cannot place, and only their top cards come back. */

static void play_calculation(const GParams *p)
{
    int deck[52], found[4][13], fnum[4], waste[4][52], wnum[4];
    int stock[52], sn, cur, i, j, placed;
    const char *msg;

    for (;;) {
        int k, pos = 0, want[4];
        deck_init(deck); shuffle_int(deck, 52);
        /* The four base cards are set aside first, in order A 2 3 4. */
        for (i = 0; i < 4; i++) {
            for (j = pos; j < 52; j++) if (rank_of(deck[j]) == i) break;
            { int t = deck[pos]; deck[pos] = deck[j]; deck[j] = t; }
            found[i][0] = deck[pos++];
            fnum[i] = 1;
            wnum[i] = 0;
        }
        sn = 0;
        while (pos < 52) stock[sn++] = deck[pos++];
        cur = 0; placed = 4; msg = NULL;

        for (;;) {
            char nm[8];
            for (i = 0; i < 4; i++)
                want[i] = (rank_of(found[i][fnum[i] - 1]) + i + 1) % 13;
            draw_title("CALCULATION",
                       "Foundations climb by one, two, three and four — "
                       "arrows move, Enter places, Q quits");
            for (i = 0; i < 4; i++) {
                draw_textf(4, 8 + i * 16, "%sstep +%d%s", C_GREY, i + 1, C_RESET);
                put_card(5, 8 + i * 16, found[i][fnum[i] - 1], cur == i, 0);
                draw_textf(5, 14 + i * 16, "%s%d/13 next %s%s", C_GREY, fnum[i],
                           RANK_NAME[want[i]], C_RESET);
            }
            for (i = 0; i < 4; i++) {
                draw_textf(8, 8 + i * 16, "%swaste %d (%d)%s", C_GREY, i + 1, wnum[i], C_RESET);
                put_card(9, 8 + i * 16, wnum[i] ? waste[i][wnum[i] - 1] : -1, cur == 4 + i, 0);
            }
            draw_textf(12, 8, "%sstock %-3d%s", C_GREY, sn, C_RESET);
            if (sn) { card_name(stock[sn - 1], nm, sizeof nm);
                      draw_textf(12, 20, "%sin hand: %s%s%s", C_GREY, suit_colour(stock[sn-1]), nm, C_RESET); }
            draw_textf(14, 8, "%splaced %d/52%s", C_GREY, placed, C_RESET);
            draw_textf(16, 8, "%s%-70s%s", C_YELLOW, msg ? msg : "", C_RESET);
            scr_flush();
            msg = NULL;

            if (placed == 52) {
                score_report(p->title ? p->title : "calculation", 2000);
                if (!confirm("\n  All fifty-two placed! Again?")) return;
                break;
            }
            if (sn == 0) {
                int any = 0;
                for (i = 0; i < 4; i++)
                    if (wnum[i]) for (j = 0; j < 4; j++)
                        if (fnum[j] < 13 && rank_of(waste[i][wnum[i] - 1]) == want[j]) any = 1;
                if (!any) {
                    score_report(p->title ? p->title : "calculation", placed * 15);
                    if (!confirm("\n  Stuck. Deal again?")) return;
                    break;
                }
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  { cur = (cur + 7) % 8; continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % 8; continue; }
            if (k == KEY_UP)    { cur = cur >= 4 ? cur - 4 : cur; continue; }
            if (k == KEY_DOWN)  { cur = cur < 4 ? cur + 4 : cur; continue; }
            if (k != KEY_ENTER) continue;

            {
                /* The card in play is the stock top, or a waste top once the
                 * stock runs out. */
                int card, src = -1;
                if (sn > 0) card = stock[sn - 1];
                else { msg = "Pick a waste pile to replay from."; continue; }
                if (cur < 4) {
                    if (fnum[cur] >= 13) { msg = "That foundation is finished."; continue; }
                    if (rank_of(card) != want[cur]) { msg = "Not the rank that foundation wants."; continue; }
                    found[cur][fnum[cur]++] = card;
                    placed++;
                    sn--;
                } else {
                    src = cur - 4;
                    waste[src][wnum[src]++] = card;
                    sn--;
                }
                /* Anything on a waste top that now fits goes up automatically. */
                {
                    int again = 1;
                    while (again) {
                        again = 0;
                        for (i = 0; i < 4; i++) {
                            if (!wnum[i]) continue;
                            for (j = 0; j < 4; j++) {
                                int w = (rank_of(found[j][fnum[j] - 1]) + j + 1) % 13;
                                if (fnum[j] < 13 && rank_of(waste[i][wnum[i] - 1]) == w) {
                                    found[j][fnum[j]++] = waste[i][--wnum[i]];
                                    placed++; again = 1;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/* ================================================================ Garbage */
/* Ten face-down slots numbered ace to ten. A drawn card goes to its own slot
 * and turns up whatever was there, which you then place in turn. A face card
 * that is not a king ends the run. */

static void play_garbage(const GParams *p)
{
    int deck[52], slot[10], sn, pos, hand, round, best;

    for (;;) {
        int k, filled;
        deck_init(deck); shuffle_int(deck, 52);
        { int i; for (i = 0; i < 10; i++) slot[i] = -1; }
        pos = 10; sn = 42; hand = -1; round = 1; best = 0;

        for (;;) {
            int i;
            filled = 0;
            for (i = 0; i < 10; i++) if (slot[i] >= 0) filled++;
            if (filled > best) best = filled;
            draw_title("GARBAGE",
                       "Cards find their own slot; a jack or queen ends the run — "
                       "Space draws, Q quits");
            for (i = 0; i < 10; i++) {
                draw_textf(5, 6 + i * 7, "%s%2d%s", C_GREY, i + 1, C_RESET);
                if (slot[i] >= 0) put_card(6, 6 + i * 7, slot[i], 0, 0);
                else              draw_textf(6, 6 + i * 7, "%s[##]%s", C_GREY, C_RESET);
            }
            draw_textf(9, 6, "%sstock %-3d   round %d   filled %d/10%s", C_GREY, sn, round, filled, C_RESET);
            if (hand >= 0) { char nm[8]; card_name(hand, nm, sizeof nm);
                             draw_textf(11, 6, "%sin hand %s%s%s", C_GREY, suit_colour(hand), nm, C_RESET); }
            scr_flush();

            if (filled == 10) {
                score_report(p->title ? p->title : "garbage", 1000 - round * 50);
                if (!confirm("\n  All ten filled! Again?")) return;
                break;
            }
            if (sn == 0 && hand < 0) {
                score_report(p->title ? p->title : "garbage", best * 60);
                if (!confirm("\n  Out of cards. Again?")) return;
                break;
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k != ' ' && k != KEY_ENTER) continue;

            if (hand < 0) {
                if (sn == 0) continue;
                hand = deck[pos++]; sn--;
            }
            {
                int r = rank_of(hand);
                if (r == 12) {                       /* kings are wild */
                    for (i = 0; i < 10; i++) if (slot[i] < 0) break;
                    if (i == 10) { hand = -1; continue; }
                    slot[i] = hand; hand = -1;
                } else if (r >= 10) {                /* jack or queen: bust */
                    draw_textf(13, 6, "%sGarbage — the run ends.%s", C_RED, C_RESET);
                    scr_flush();
                    sleep_ms(500);
                    hand = -1;
                    round++;
                    { int j; for (j = 0; j < 10; j++) slot[j] = -1; }
                    deck_init(deck); shuffle_int(deck, 52); pos = 0; sn = 52;
                } else if (slot[r] >= 0) {
                    draw_textf(13, 6, "%sSlot %d is taken — the run ends.%s", C_RED, r + 1, C_RESET);
                    scr_flush();
                    sleep_ms(500);
                    hand = -1;
                    round++;
                } else {
                    slot[r] = hand;
                    hand = -1;
                }
            }
        }
    }
}

/* =========================================================== Kings Corner */
/* Against one opponent. Eight lay-off piles around the deck; build down in
 * alternating colours, and only a king may open a corner. First hand empty
 * wins. */

static void play_kingscorner(const GParams *p)
{
    int deck[52], pos, pile[8][52], pn[8], hand[2][52], hn[2], cur, turn, sel, i;
    const char *msg;

    for (;;) {
        int k;
        deck_init(deck); shuffle_int(deck, 52);
        pos = 0;
        for (i = 0; i < 8; i++) pn[i] = 0;
        for (i = 0; i < 2; i++) { int j; hn[i] = 0; for (j = 0; j < 7; j++) hand[i][hn[i]++] = deck[pos++]; }
        for (i = 0; i < 4; i++) pile[i][pn[i]++] = deck[pos++];
        cur = 0; turn = 0; sel = -1; msg = NULL;

        for (;;) {
            draw_title("KINGS CORNER",
                       "Build down in alternating colours; corners need a king — "
                       "arrows move, Enter plays, D draws, Q quits");
            for (i = 0; i < 8; i++) {
                draw_textf(4 + (i / 4) * 4, 8 + (i % 4) * 14, "%s%s%s%s",
                           C_GREY, i < 4 ? "side " : "corner ", i < 4 ? "" : "", C_RESET);
                put_card(5 + (i / 4) * 4, 8 + (i % 4) * 14, pn[i] ? pile[i][pn[i] - 1] : -1, 0, 0);
                if (pn[i] > 1) draw_textf(5 + (i / 4) * 4, 13 + (i % 4) * 14, "%sx%-2d%s", C_GREY, pn[i], C_RESET);
            }
            draw_textf(13, 8, "%sopponent holds %d cards   stock %d%s", C_GREY, hn[1], 52 - pos, C_RESET);
            draw_textf(15, 8, "%syour hand:%s", C_BOLD, C_RESET);
            for (i = 0; i < hn[0]; i++)
                put_card(16, 8 + i * 5, hand[0][i], i == cur, 0);
            draw_textf(18, 8, "%s%-70s%s", C_YELLOW, msg ? msg : "", C_RESET);
            scr_flush();
            msg = NULL;

            if (hn[0] == 0 || hn[1] == 0) {
                int youwin = (hn[0] == 0);
                draw_textf(20, 8, "%s%s%s", youwin ? C_GREEN : C_RED,
                           youwin ? "Your hand is empty — you win." : "The opponent went out first.", C_RESET);
                scr_flush();
                score_report(p->title ? p->title : "kingscorner", youwin ? 500 : 100);
                if (!confirm("\n  Another hand?")) return;
                break;
            }

            if (turn == 1) {
                /* The opponent plays greedily: every card it can shed, it sheds. */
                int again = 1;
                while (again) {
                    again = 0;
                    for (i = 0; i < hn[1] && !again; i++)
                        for (k = 0; k < 8; k++) {
                            int c = hand[1][i];
                            int ok = pn[k] ? (rank_of(pile[k][pn[k]-1]) == rank_of(c) + 1 &&
                                              red_of(pile[k][pn[k]-1]) != red_of(c))
                                           : (k >= 4 ? rank_of(c) == 12 : 1);
                            if (!ok) continue;
                            pile[k][pn[k]++] = c;
                            { int j; for (j = i; j < hn[1] - 1; j++) hand[1][j] = hand[1][j + 1]; }
                            hn[1]--;
                            again = 1;
                            break;
                        }
                }
                if (pos < 52 && hn[1] > 0) hand[1][hn[1]++] = deck[pos++];
                turn = 0;
                msg = "Your turn.";
                continue;
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  { if (hn[0]) cur = (cur + hn[0] - 1) % hn[0]; continue; }
            if (k == KEY_RIGHT) { if (hn[0]) cur = (cur + 1) % hn[0]; continue; }
            if (k == 'd' || k == 'D') {
                if (pos < 52) hand[0][hn[0]++] = deck[pos++];
                turn = 1;
                continue;
            }
            if (k >= '1' && k <= '8') {
                int t = k - '1', c;
                if (cur >= hn[0]) { msg = "No card selected."; continue; }
                c = hand[0][cur];
                if (pn[t]) {
                    if (rank_of(pile[t][pn[t]-1]) != rank_of(c) + 1 || red_of(pile[t][pn[t]-1]) == red_of(c)) {
                        msg = "Must be one lower and the other colour.";
                        continue;
                    }
                } else if (t >= 4 && rank_of(c) != 12) {
                    msg = "Only a king may open a corner.";
                    continue;
                }
                pile[t][pn[t]++] = c;
                for (i = cur; i < hn[0] - 1; i++) hand[0][i] = hand[0][i + 1];
                hn[0]--;
                if (cur >= hn[0] && cur > 0) cur--;
                msg = "Played — keep going or press D to end your turn.";
                continue;
            }
            if (k == KEY_ENTER) msg = "Press 1-8 to choose a pile for the selected card.";
            (void)sel;
        }
    }
}

/* ======================================================== Nertz and Spit */
/* Both are races. Nertz is patience against a clock; Spit is patience against
 * an opponent reaching for the same two centre piles. */

static void play_race(const GParams *p, int spit)
{
    int deck[52], pos, nertz[26], nn, work[4][52], wkn[4];
    int centre[2][52], cn[2], hand[52], hnum, foes, cur, i;
    long t0, limit;
    int score;
    const char *msg;

    for (;;) {
        int k;
        deck_init(deck); shuffle_int(deck, 52);
        pos = 0; nn = 0; hnum = 0; score = 0; cur = 0; msg = NULL;
        for (i = 0; i < (spit ? 10 : 13); i++) nertz[nn++] = deck[pos++];
        for (i = 0; i < 4; i++) { wkn[i] = 0; work[i][wkn[i]++] = deck[pos++]; }
        for (i = 0; i < 2; i++) { cn[i] = 0; centre[i][cn[i]++] = deck[pos++]; }
        while (pos < 52) hand[hnum++] = deck[pos++];
        foes = spit ? 12 : 0;
        t0 = now_ms();
        limit = spit ? 0 : 180000;               /* Nertz: three minutes */

        for (;;) {
            long el = now_ms() - t0;
            draw_title(spit ? "SPIT" : "NERTZ",
                       spit ? "Slap cards one rank either way onto the centre — "
                              "arrows move, Enter plays, D flips, Q quits"
                            : "Beat the clock — arrows move, Enter plays, D flips, Q quits");
            draw_textf(4, 8, "%s%s pile %-2d%s", C_BOLD, spit ? "your stock" : "nertz", nn, C_RESET);
            put_card(5, 8, nn ? nertz[nn - 1] : -1, cur == 0, 0);
            for (i = 0; i < 4; i++) {
                draw_textf(4, 24 + i * 10, "%swork %d%s", C_GREY, i + 1, C_RESET);
                put_card(5, 24 + i * 10, wkn[i] ? work[i][wkn[i] - 1] : -1, cur == 1 + i, 0);
                if (wkn[i] > 1) draw_textf(5, 29 + i * 10, "%sx%-2d%s", C_GREY, wkn[i], C_RESET);
            }
            for (i = 0; i < 2; i++) {
                draw_textf(8, 24 + i * 16, "%scentre %d%s", C_YELLOW, i + 1, C_RESET);
                put_card(9, 24 + i * 16, cn[i] ? centre[i][cn[i] - 1] : -1, 0, 0);
            }
            draw_textf(11, 8, "%shand %-2d   score %-4d%s", C_GREY, hnum, score, C_RESET);
            if (spit) draw_textf(11, 34, "%sopponent stock %-2d%s", C_GREY, foes, C_RESET);
            else      draw_textf(11, 34, "%stime left %3ld s%s", C_GREY, (limit - el) / 1000, C_RESET);
            draw_textf(13, 8, "%s%-70s%s", C_YELLOW, msg ? msg : "", C_RESET);
            scr_flush();
            msg = NULL;

            if (nn == 0) {
                draw_textf(15, 8, "%sStock cleared — you win.%s", C_GREEN, C_RESET);
                scr_flush();
                score += 500;
                score_report(p->title ? p->title : "race", score);
                if (!confirm("\n  Another round?")) return;
                break;
            }
            if (spit && foes == 0) {
                draw_textf(15, 8, "%sThe opponent went out first.%s", C_RED, C_RESET);
                scr_flush();
                score_report(p->title ? p->title : "race", score);
                if (!confirm("\n  Another round?")) return;
                break;
            }
            if (!spit && el >= limit) {
                draw_textf(15, 8, "%sTime — %d cards left on the nertz pile.%s", C_RED, nn, C_RESET);
                scr_flush();
                score_report(p->title ? p->title : "race", score);
                if (!confirm("\n  Another round?")) return;
                break;
            }

            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;

            if (spit) {
                /* The opponent grabs a centre pile roughly twice a second. */
                static long last;
                if (now_ms() - last > 1400) {
                    last = now_ms();
                    if (foes > 0 && rnd(100) < 55) {
                        int t = rnd(2), c = rnd(52);
                        int d = (rank_of(c) - rank_of(centre[t][cn[t] - 1]) + 13) % 13;
                        if (d == 1 || d == 12) { centre[t][cn[t]++] = c; foes--; }
                    }
                }
            }

            if (k == KEY_LEFT)  { cur = (cur + 4) % 5; continue; }
            if (k == KEY_RIGHT) { cur = (cur + 1) % 5; continue; }
            if (k == 'd' || k == 'D') {
                if (hnum > 0) { int t = rnd(4); work[t][wkn[t]++] = hand[--hnum]; }
                else msg = "Your hand is empty.";
                continue;
            }
            if (k == '1' || k == '2' || k == KEY_ENTER) {
                int t = (k == '2') ? 1 : 0;
                int card = (cur == 0) ? (nn ? nertz[nn - 1] : -1)
                                      : (wkn[cur - 1] ? work[cur - 1][wkn[cur - 1] - 1] : -1);
                int d;
                if (card < 0) { msg = "Nothing there to play."; continue; }
                if (cn[t] == 0) { msg = "That centre pile is empty."; continue; }
                d = (rank_of(card) - rank_of(centre[t][cn[t] - 1]) + 13) % 13;
                if (d != 1 && d != 12) { msg = "Not one rank away."; continue; }
                centre[t][cn[t]++] = card;
                if (cur == 0) nn--; else wkn[cur - 1]--;
                score += 25;
                continue;
            }
            sleep_ms(12);
        }
    }
}

/* ------------------------------------------------------------- dispatcher */

void solitaire_other(const GParams *p, int v)
{
    switch (v) {
    case  9: play_pyramid(p);      break;
    case 10: play_ladder(p, 1);    break;   /* TriPeaks */
    case 11: play_ladder(p, 0);    break;   /* Golf     */
    case 17: play_acesup(p);       break;
    case 18: play_accordion(p);    break;
    case 21: play_clock(p);        break;
    case 23: play_calculation(p);  break;
    case 25: play_garbage(p);      break;
    case 26: play_kingscorner(p);  break;
    case 27: play_race(p, 0);      break;   /* Nertz */
    case 28: play_race(p, 1);      break;   /* Spit  */
    default: play_pyramid(p);      break;
    }
}
