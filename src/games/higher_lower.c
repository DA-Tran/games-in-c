/* higher_lower.c - call the next card and push your streak. */
#include "engine.h"
#include "cards.h"
#include "games.h"

static int rank_of(int card) { int r = card % 13; return r == 0 ? 14 : r + 1; }

void fam_higher_lower(const GParams *p)
{
    (void)p;
    for (;;) {
        int deck[52], top = 0, streak = 0, best = 0, chips = 50;
        deck_init(deck);

        while (chips > 0 && top < 51) {
            int cur = deck[top], nxt, k, guess_high;
            char m[96];

            scr_clear();
            draw_title("HIGHER OR LOWER", "H higher, L lower, Q quits");
            draw_card(9, 36, cur, 1);
            draw_textf(13, 28, "Streak: %-3d  Best: %-3d  Chips: %-4d", streak, best, chips);
            draw_textf(15, 28, "%sWill the next card be higher or lower?%s", C_GREY, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
            if (k == 'h' || k == 'H' || k == KEY_UP)        guess_high = 1;
            else if (k == 'l' || k == 'L' || k == KEY_DOWN) guess_high = 0;
            else continue;

            top++;
            nxt = deck[top];
            draw_card(9, 46, nxt, 1);

            if (rank_of(nxt) == rank_of(cur)) {
                snprintf(m, sizeof m, "Equal rank - push.");
            } else if ((rank_of(nxt) > rank_of(cur)) == guess_high) {
                streak++;
                chips += 5 + streak;
                if (streak > best) best = streak;
                snprintf(m, sizeof m, "Correct! Streak %d, +%d chips.", streak, 5 + streak);
            } else {
                snprintf(m, sizeof m, "Wrong - streak lost. -10 chips.");
                chips -= 10;
                streak = 0;
            }
            draw_textf(17, 28, "%s%-50s%s", C_BOLD, m, C_RESET);
            scr_flush();
            sleep_ms(1100);
        }
        scr_clear();
        draw_centered(12, 80, C_BOLD "Run over." C_RESET);
        score_report("higher-lower", best);
        if (!confirm("\n  Play again?")) return;
    }
}
