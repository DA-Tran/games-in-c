/* war.c - the pure-chance card duel, with full war resolution on ties. */
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

void fam_war(const GParams *p)
{
    (void)p;
    for (;;) {
        int deck[52], i, rounds = 0;
        deck_init(deck);
        pn = cn = 0; ph = ch = 0;
        for (i = 0; i < 26; i++) push(pq, &pn, ph, deck[i]);
        for (i = 26; i < 52; i++) push(cq, &cn, ch, deck[i]);

        while (pn > 0 && cn > 0 && rounds < 600) {
            int pot[64], potn = 0, pc_, cc_, k;
            rounds++;
            pc_ = pop(pq, &pn, &ph);
            cc_ = pop(cq, &cn, &ch);
            pot[potn++] = pc_;
            pot[potn++] = cc_;

            scr_clear();
            draw_title("WAR", "Enter plays the next round, Q quits");
            draw_textf(7, 30, "%sYou%s", C_CYAN, C_RESET);
            draw_card(8, 30, pc_, 1);
            draw_textf(7, 44, "%sCPU%s", C_RED, C_RESET);
            draw_card(8, 44, cc_, 1);

            while (rank_of(pc_) == rank_of(cc_) && pn > 2 && cn > 2 && potn < 56) {
                int j;
                draw_textf(12, 30, "%sWAR! Three cards down...%s", C_BOLD C_YELLOW, C_RESET);
                scr_flush();
                sleep_ms(700);
                for (j = 0; j < 3; j++) {
                    pot[potn++] = pop(pq, &pn, &ph);
                    pot[potn++] = pop(cq, &cn, &ch);
                }
                pc_ = pop(pq, &pn, &ph);
                cc_ = pop(cq, &cn, &ch);
                pot[potn++] = pc_;
                pot[potn++] = cc_;
                draw_card(8, 30, pc_, 1);
                draw_card(8, 44, cc_, 1);
            }

            if (rank_of(pc_) > rank_of(cc_)) {
                for (i = 0; i < potn; i++) push(pq, &pn, ph, pot[i]);
                draw_textf(13, 30, "%sYou take %d cards.      %s", C_GREEN, potn, C_RESET);
            } else if (rank_of(cc_) > rank_of(pc_)) {
                for (i = 0; i < potn; i++) push(cq, &cn, ch, pot[i]);
                draw_textf(13, 30, "%sComputer takes %d cards.%s", C_RED, potn, C_RESET);
            }
            draw_textf(15, 30, "Your cards: %-3d   CPU cards: %-3d   Round %d", pn, cn, rounds);
            draw_text(17, 26, "Press Enter for the next round, Q to quit");
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
        }
        scr_clear();
        draw_centered(12, 80, pn > cn ? C_BOLD C_GREEN "You win the war!" C_RESET
                                      : pn < cn ? C_BOLD C_RED "Computer wins." C_RESET
                                                : C_BOLD "A draw." C_RESET);
        score_report("war", pn);
        if (!confirm("\n  Play again?")) return;
    }
}
