/* tictactoe.c - 3x3 noughts and crosses with an unbeatable minimax AI. */
#include "engine.h"
#include "games.h"

static char bd[9];

static int winner(void)
{
    static const int L[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},
                                {1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    int i;
    for (i = 0; i < 8; i++)
        if (bd[L[i][0]] != ' ' && bd[L[i][0]] == bd[L[i][1]] && bd[L[i][1]] == bd[L[i][2]])
            return bd[L[i][0]];
    for (i = 0; i < 9; i++) if (bd[i] == ' ') return 0;
    return 'D';
}

/* Negamax: +10 for an AI win, discounted by depth so it wins quickly. */
static int minimax(int is_ai, int depth)
{
    int w = winner(), i, best;
    if (w == 'O') return 10 - depth;
    if (w == 'X') return depth - 10;
    if (w == 'D') return 0;

    best = is_ai ? -100 : 100;
    for (i = 0; i < 9; i++) {
        int v;
        if (bd[i] != ' ') continue;
        bd[i] = is_ai ? 'O' : 'X';
        v = minimax(!is_ai, depth + 1);
        bd[i] = ' ';
        if (is_ai) { if (v > best) best = v; }
        else       { if (v < best) best = v; }
    }
    return best;
}

static int ai_move(void)
{
    int i, best = -100, mv = -1;
    for (i = 0; i < 9; i++) {
        int v;
        if (bd[i] != ' ') continue;
        bd[i] = 'O';
        v = minimax(0, 0);
        bd[i] = ' ';
        if (v > best) { best = v; mv = i; }
    }
    return mv;
}

static void render(int cur, const char *msg)
{
    int r, c;
    draw_title("TIC TAC TOE", "Arrow keys to move, Enter to place, Q to quit");
    for (r = 0; r < 3; r++) {
        for (c = 0; c < 3; c++) {
            int i = r * 3 + c;
            const char *col = bd[i] == 'X' ? C_CYAN : (bd[i] == 'O' ? C_YELLOW : C_GREY);
            char cell[32];
            snprintf(cell, sizeof cell, "%s%s %c %s%s",
                     i == cur ? C_REV : "", col, bd[i] == ' ' ? '.' : bd[i],
                     C_RESET, "");
            draw_text(7 + r * 2, 34 + c * 4, cell);
        }
    }
    draw_text(14, 28, "                                  ");
    if (msg) draw_textf(14, 30, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_tictactoe(const GParams *p)
{
    (void)p;
    for (;;) {
        int cur = 4, over = 0, i;
        for (i = 0; i < 9; i++) bd[i] = ' ';

        while (!over) {
            int k, w;
            render(cur, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT)  cur = (cur % 3 == 0) ? cur + 2 : cur - 1;
            if (k == KEY_RIGHT) cur = (cur % 3 == 2) ? cur - 2 : cur + 1;
            if (k == KEY_UP)    cur = (cur < 3) ? cur + 6 : cur - 3;
            if (k == KEY_DOWN)  cur = (cur > 5) ? cur - 6 : cur + 3;
            if (k != KEY_ENTER && k != ' ') continue;
            if (bd[cur] != ' ') continue;

            bd[cur] = 'X';
            w = winner();
            if (!w) {
                int m = ai_move();
                if (m >= 0) bd[m] = 'O';
                w = winner();
            }
            if (w) {
                render(cur, w == 'X' ? "You win!" : w == 'O' ? "Computer wins." : "Draw.");
                over = 1;
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
