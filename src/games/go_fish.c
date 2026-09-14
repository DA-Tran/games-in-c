/* go_fish.c - ask for ranks and build books; the AI remembers your asks. */
#include "engine.h"
#include "cards.h"
#include "games.h"

static int deck[52], top;
static int phand[52], pn, chand[52], cn;
static int pbooks, cbooks;
static int cpu_knows[13];        /* ranks the player has asked for */

static int count_rank(const int *h, int n, int r)
{
    int i, c = 0;
    for (i = 0; i < n; i++) if (h[i] % 13 == r) c++;
    return c;
}

static void remove_rank(int *h, int *n, int r, int *out, int *outn)
{
    int i, k = 0;
    for (i = 0; i < *n; i++) {
        if (h[i] % 13 == r) { if (out) out[(*outn)++] = h[i]; }
        else h[k++] = h[i];
    }
    *n = k;
}

static int check_books(int *h, int *n, int *books)
{
    int r, made = 0;
    for (r = 0; r < 13; r++)
        if (count_rank(h, *n, r) == 4) {
            remove_rank(h, n, r, NULL, NULL);
            (*books)++;
            made = 1;
        }
    return made;
}

static void render(int cur, const char *msg)
{
    int i, row = 0, ranks[13], nr = 0;
    draw_title("GO FISH", "Left/Right pick a rank, Enter asks, Q quits");

    for (i = 0; i < 13; i++) if (count_rank(phand, pn, i)) ranks[nr++] = i;

    draw_textf(6, 16, "%sYour hand (%d cards)%s", C_CYAN, pn, C_RESET);
    for (i = 0; i < nr; i++) {
        draw_textf(8 + row, 16 + (i % 9) * 7, "%s%s%s x%d%s",
                   i == cur ? C_REV : "", C_WHITE, RANK_NAME[ranks[i]],
                   count_rank(phand, pn, ranks[i]), C_RESET);
        if (i % 9 == 8) row += 2;
    }
    draw_textf(14, 16, "Your books: %-3d   CPU books: %-3d   Deck: %-3d",
               pbooks, cbooks, 52 - top);
    draw_textf(15, 16, "CPU holds %d cards", cn);
    draw_text(17, 12, "                                                            ");
    if (msg) draw_textf(17, 16, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_go_fish(const GParams *p)
{
    (void)p;
    for (;;) {
        int i, over = 0, cur = 0;
        deck_init(deck);
        top = 0;
        pn = cn = pbooks = cbooks = 0;
        memset(cpu_knows, 0, sizeof cpu_knows);
        for (i = 0; i < 7; i++) { phand[pn++] = deck[top++]; chand[cn++] = deck[top++]; }
        check_books(phand, &pn, &pbooks);
        check_books(chand, &cn, &cbooks);

        while (!over) {
            int ranks[13], nr = 0, k, ask, got;
            char msg[96];

            for (i = 0; i < 13; i++) if (count_rank(phand, pn, i)) ranks[nr++] = i;
            if (nr == 0) {
                if (top < 52) { phand[pn++] = deck[top++]; continue; }
                break;
            }
            if (cur >= nr) cur = nr - 1;

            render(cur, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  cur = (cur + nr - 1) % nr;
            if (k == KEY_RIGHT) cur = (cur + 1) % nr;
            if (k != KEY_ENTER && k != ' ') continue;

            ask = ranks[cur];
            cpu_knows[ask] = 1;
            got = count_rank(chand, cn, ask);
            if (got) {
                int taken[8], tn = 0;
                remove_rank(chand, &cn, ask, taken, &tn);
                for (i = 0; i < tn; i++) phand[pn++] = taken[i];
                snprintf(msg, sizeof msg, "You got %d %s%s from the computer.",
                         got, RANK_NAME[ask], got > 1 ? "s" : "");
            } else {
                if (top < 52) phand[pn++] = deck[top++];
                snprintf(msg, sizeof msg, "Go fish! You drew a card.");
            }
            check_books(phand, &pn, &pbooks);
            render(cur, msg);
            sleep_ms(1100);

            /* Computer turn: prefer a rank it has seen the player ask about. */
            if (cn > 0) {
                int want = -1;
                for (i = 0; i < 13; i++)
                    if (cpu_knows[i] && count_rank(chand, cn, i)) { want = i; break; }
                if (want < 0)
                    for (i = 0; i < 13; i++)
                        if (count_rank(chand, cn, i)) { want = i; break; }
                if (want >= 0) {
                    int have = count_rank(phand, pn, want);
                    if (have) {
                        int taken[8], tn = 0;
                        remove_rank(phand, &pn, want, taken, &tn);
                        for (i = 0; i < tn; i++) chand[cn++] = taken[i];
                        snprintf(msg, sizeof msg, "Computer asked for %s and took %d.",
                                 RANK_NAME[want], tn);
                    } else {
                        if (top < 52) chand[cn++] = deck[top++];
                        snprintf(msg, sizeof msg, "Computer asked for %s - go fish.",
                                 RANK_NAME[want]);
                    }
                    check_books(chand, &cn, &cbooks);
                    render(cur, msg);
                    sleep_ms(1100);
                }
            }
            if (pbooks + cbooks >= 13 || (pn == 0 && cn == 0 && top >= 52)) over = 1;
        }
        scr_clear();
        draw_centered(12, 80, pbooks > cbooks ? C_BOLD C_GREEN "You win!" C_RESET
                                              : pbooks < cbooks ? C_BOLD C_RED "Computer wins." C_RESET
                                                                : C_BOLD "Draw." C_RESET);
        score_report("go-fish", pbooks * 100);
        if (!confirm("\n  Play again?")) return;
    }
}
