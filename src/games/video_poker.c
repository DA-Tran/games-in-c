/* video_poker.c - Jacks or Better with a hold-and-draw round. */
#include "engine.h"
#include "cards.h"
#include "games.h"

static int deck[52], top, hand[5], hold[5];
static int credits;

static const char *PAY_NAME[9] = {
    "Royal Flush", "Straight Flush", "Four of a Kind", "Full House",
    "Flush", "Straight", "Three of a Kind", "Two Pair", "Jacks or Better"
};
static const int PAYOUT[9] = {800, 50, 25, 9, 6, 4, 3, 2, 1};

/* Returns an index into PAYOUT, or -1 for no win. */
static int evaluate(void)
{
    int counts[13] = {0}, suits[4] = {0}, i;
    int pairs = 0, three = 0, four = 0, jacks = 0;
    int flush = 0, straight = 0, lo = 13, hi = -1;

    for (i = 0; i < 5; i++) { counts[hand[i] % 13]++; suits[hand[i] / 13]++; }
    for (i = 0; i < 4; i++) if (suits[i] == 5) flush = 1;
    for (i = 0; i < 13; i++) {
        if (counts[i] == 2) { pairs++; if (i == 0 || i >= 10) jacks = 1; }
        if (counts[i] == 3) three = 1;
        if (counts[i] == 4) four = 1;
        if (counts[i]) { if (i < lo) lo = i; if (i > hi) hi = i; }
    }
    {
        int distinct = 0;
        for (i = 0; i < 13; i++) if (counts[i]) distinct++;
        if (distinct == 5) {
            if (hi - lo == 4) straight = 1;
            /* Ace-high straight: A,10,J,Q,K */
            if (counts[0] && counts[9] && counts[10] && counts[11] && counts[12]) straight = 2;
        }
    }
    if (straight == 2 && flush) return 0;
    if (straight && flush)      return 1;
    if (four)                   return 2;
    if (three && pairs == 1)    return 3;
    if (flush)                  return 4;
    if (straight)               return 5;
    if (three)                  return 6;
    if (pairs == 2)             return 7;
    if (pairs == 1 && jacks)    return 8;
    return -1;
}

static void render(int cur, int phase, const char *msg)
{
    int i;
    draw_title("VIDEO POKER", "Left/Right select, Space holds, Enter draws, Q quits");
    for (i = 0; i < 9; i++)
        draw_textf(6 + i, 54, "%s%-16s %3d%s", C_GREY, PAY_NAME[i], PAYOUT[i], C_RESET);

    for (i = 0; i < 5; i++) {
        draw_card(9, 14 + i * 7, hand[i], 1);
        draw_textf(12, 14 + i * 7, "%s%s%s",
                   hold[i] ? C_BOLD C_YELLOW : C_GREY,
                   hold[i] ? " HOLD " : "      ", C_RESET);
        draw_textf(13, 14 + i * 7, "%s%s%s", i == cur ? C_REV : "",
                   i == cur ? "  ^^  " : "      ", C_RESET);
    }
    draw_textf(16, 14, "Credits: %-6d   %s", credits, phase ? "Draw phase" : "Hold phase");
    draw_text(18, 10, "                                                        ");
    if (msg) draw_textf(18, 14, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_video_poker(const GParams *p)
{
    (void)p;
    credits = 100;
    deck_init(deck);
    top = 0;

    while (credits > 0) {
        int cur = 0, i, result;
        const int bet = 5;

        if (top > 40) { deck_init(deck); top = 0; }
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
                if (top >= 52) { deck_init(deck); top = 0; }
                hand[i] = deck[top++];
            }

        result = evaluate();
        if (result >= 0) {
            char m[80];
            credits += PAYOUT[result] * bet;
            snprintf(m, sizeof m, "%s - pays %d!", PAY_NAME[result], PAYOUT[result] * bet);
            render(cur, 1, m);
        } else {
            render(cur, 1, "No win.");
        }
        score_report("video-poker", credits);
        pause_msg("Press any key for the next hand...");
    }
    scr_clear();
    draw_centered(12, 80, C_BOLD C_RED "Out of credits." C_RESET);
    pause_msg("Press any key...");
}
