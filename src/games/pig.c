/* pig.c - press-your-luck dice race to 100; a one wipes the turn total. */
#include "engine.h"
#include "games.h"

static int you, cpu;

static void render(int turn_total, int die, int cpu_turn, const char *msg)
{
    draw_title("PIG", "R rolls, H holds, Q quits - first to 100");
    draw_textf(7,  28, "%sYou%s  %3d", C_CYAN, C_RESET, you);
    draw_textf(8,  28, "%sCPU%s  %3d", C_RED, C_RESET, cpu);
    draw_textf(11, 28, "Turn total: %s%-4d%s", C_YELLOW, turn_total, C_RESET);
    if (die) {
        draw_textf(13, 28, "%s┌───┐%s", C_WHITE, C_RESET);
        draw_textf(14, 28, "%s│ %d │%s", C_WHITE, die, C_RESET);
        draw_textf(15, 28, "%s└───┘%s", C_WHITE, C_RESET);
    }
    draw_textf(17, 28, "%s%s%s", C_GREY, cpu_turn ? "Computer's turn" : "Your turn   ", C_RESET);
    draw_text(19, 24, "                                                  ");
    if (msg) draw_textf(19, 28, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_pig(const GParams *p)
{
    (void)p;
    for (;;) {
        you = cpu = 0;

        while (you < 100 && cpu < 100) {
            int turn = 0, rolling = 1;

            while (rolling) {
                int k;
                render(turn, 0, 0, NULL);
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                if (k == 'r' || k == 'R' || k == ' ') {
                    int d = rnd_range(1, 6);
                    if (d == 1) {
                        render(0, d, 0, "Rolled a 1 - you lose the turn total.");
                        sleep_ms(1200);
                        turn = 0;
                        rolling = 0;
                    } else {
                        turn += d;
                        render(turn, d, 0, NULL);
                        sleep_ms(450);
                        if (you + turn >= 100) { you += turn; rolling = 0; turn = 0; }
                    }
                }
                if (k == 'h' || k == 'H') { you += turn; turn = 0; rolling = 0; }
            }
            if (you >= 100) break;

            /* Computer holds at 20, or sooner if it can win outright. */
            turn = 0;
            for (;;) {
                int d;
                if (turn >= 20 || cpu + turn >= 100) break;
                d = rnd_range(1, 6);
                render(turn, d, 1, NULL);
                sleep_ms(500);
                if (d == 1) {
                    render(0, d, 1, "Computer rolled a 1.");
                    sleep_ms(900);
                    turn = 0;
                    break;
                }
                turn += d;
            }
            cpu += turn;
            render(0, 0, 1, "Computer holds.");
            sleep_ms(700);
        }
        render(0, 0, 0, you >= 100 ? "You reach 100 - you win!" : "Computer reaches 100.");
        score_report("pig", you);
        if (!confirm("\n  Play again?")) return;
    }
}
