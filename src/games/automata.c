/* GIC:PARAMETERISED automata
 * automata.c - the cellular automata family.
 *
 * params: variant = which rule
 *
 * Three genuinely different machines, not sixteen speed settings:
 *
 *   elementary   a one-dimensional rule, drawn as space across and time down.
 *                Rule 30 is chaotic, rule 90 draws a Sierpinski triangle,
 *                rule 110 is Turing complete, rule 150 is additive.
 *   life-like    a two-dimensional birth/survival rule. HighLife has a
 *                replicator, Day and Night is self-complementary, Seeds never
 *                survives, Brian's Brain has a third dying state.
 *   Langton's ant a single walker that rewrites the cells it crosses and, after
 *                about ten thousand chaotic steps, builds a highway.
 *
 * You place cells and step the rule; it is a sandbox rather than a contest,
 * so the score is the population reached.
 */
#include "engine.h"
#include "games.h"

#define W 64
#define H 20

typedef struct {
    const char *name;
    int kind;            /* 0 elementary, 1 life-like, 2 Langton's ant */
    int rule;            /* elementary rule number                    */
    int birth, survive;  /* life-like bitmasks over neighbour counts   */
    int states;          /* 2, or 3 for Brian's Brain                 */
} Auto;

/* Bit n set means "n neighbours". B3/S23 is Conway's Life. */
#define M(n) (1 << (n))

static const Auto RULES[16] = {
    {"Rule 30",        0,  30, 0, 0, 2},
    {"Rule 54",        0,  54, 0, 0, 2},
    {"Rule 90",        0,  90, 0, 0, 2},
    {"Rule 110",       0, 110, 0, 0, 2},
    {"Rule 150",       0, 150, 0, 0, 2},
    {"Rule 182",       0, 182, 0, 0, 2},
    {"Rule 22",        0,  22, 0, 0, 2},
    {"Rule 60",        0,  60, 0, 0, 2},
    {"Conway's Life",  1, 0, M(3),        M(2)|M(3),        2},
    {"HighLife",       1, 0, M(3)|M(6),   M(2)|M(3),        2},
    {"Day and Night",  1, 0, M(3)|M(6)|M(7)|M(8), M(3)|M(4)|M(6)|M(7)|M(8), 2},
    {"Seeds",          1, 0, M(2),        0,                2},
    {"Brian's Brain",  1, 0, M(2),        0,                3},
    {"Maze",           1, 0, M(3),        M(1)|M(2)|M(3)|M(4)|M(5), 2},
    {"Coral",          1, 0, M(3),        M(4)|M(5)|M(6)|M(7)|M(8), 2},
    {"Langton's Ant",  2, 0, 0, 0, 2}
};

static int cell[H][W], next[H][W];
static int VAR;

static void seed_random(void)
{
    int r, c;
    memset(cell, 0, sizeof cell);
    if (RULES[VAR].kind == 0) {
        /* A single live cell is the classic elementary starting row. */
        cell[0][W / 2] = 1;
    } else if (RULES[VAR].kind == 2) {
        /* Langton's ant starts on a blank sheet. */
    } else {
        for (r = 0; r < H; r++) for (c = 0; c < W; c++)
            cell[r][c] = rnd(100) < 28 ? 1 : 0;
    }
}

static int neighbours(int r, int c)
{
    int dr, dc, n = 0;
    for (dr = -1; dr <= 1; dr++) for (dc = -1; dc <= 1; dc++) {
        int rr, cc;
        if (!dr && !dc) continue;
        rr = (r + dr + H) % H;
        cc = (c + dc + W) % W;
        if (cell[rr][cc] == 1) n++;      /* only live cells count */
    }
    return n;
}

/* One generation. Elementary rules scroll: each new row is computed from the
 * one above, so the display reads as time flowing downwards. */
static void step_rule(int *ant_r, int *ant_c, int *ant_dir)
{
    const Auto *A = &RULES[VAR];
    int r, c;

    if (A->kind == 0) {
        for (r = H - 1; r > 0; r--)
            memcpy(cell[r], cell[r - 1], sizeof cell[0]);
        for (c = 0; c < W; c++) {
            int l = cell[1][(c + W - 1) % W], m = cell[1][c], rr = cell[1][(c + 1) % W];
            int idx = (l << 2) | (m << 1) | rr;
            cell[0][c] = (A->rule >> idx) & 1;
        }
        return;
    }
    if (A->kind == 2) {
        static const int DR[4] = {-1, 0, 1, 0};
        static const int DC[4] = {0, 1, 0, -1};
        int r2 = *ant_r, c2 = *ant_c;
        if (cell[r2][c2]) { *ant_dir = (*ant_dir + 3) % 4; cell[r2][c2] = 0; }
        else              { *ant_dir = (*ant_dir + 1) % 4; cell[r2][c2] = 1; }
        *ant_r = (r2 + DR[*ant_dir] + H) % H;
        *ant_c = (c2 + DC[*ant_dir] + W) % W;
        return;
    }

    for (r = 0; r < H; r++) for (c = 0; c < W; c++) {
        int n = neighbours(r, c), v = cell[r][c];
        if (A->states == 3) {
            /* Brian's Brain: live -> dying -> dead -> maybe live. */
            if (v == 1)      next[r][c] = 2;
            else if (v == 2) next[r][c] = 0;
            else             next[r][c] = (A->birth & M(n)) ? 1 : 0;
        } else {
            if (v) next[r][c] = (A->survive & M(n)) ? 1 : 0;
            else   next[r][c] = (A->birth   & M(n)) ? 1 : 0;
        }
    }
    memcpy(cell, next, sizeof cell);
}

static int population(void)
{
    int r, c, n = 0;
    for (r = 0; r < H; r++) for (c = 0; c < W; c++) if (cell[r][c] == 1) n++;
    return n;
}

void fam_automata(const GParams *p)
{
    int running = 1, gen = 0, peak = 0;
    int cr = H / 2, cc = W / 2;
    int ant_r = H / 2, ant_c = W / 2, ant_dir = 0;
    long last = now_ms();

    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR > 15) VAR = 0;
    seed_random();

    for (;;) {
        int k, r, c, pop;
        char sub[110];
        const Auto *A = &RULES[VAR];

        snprintf(sub, sizeof sub,
                 "%s — Space runs/pauses, S steps once, R reseeds, arrows+Enter draw, Q quits",
                 A->name);
        draw_title("CELLULAR AUTOMATA", sub);
        for (r = 0; r < H; r++) {
            scr_move(4 + r, 8);
            for (c = 0; c < W; c++) {
                int v = cell[r][c];
                int here = (A->kind == 2) ? (r == ant_r && c == ant_c)
                                          : (r == cr && c == cc);
                if (here)        printf("%s%s@%s", BG_BLUE, C_BOLD C_WHITE, C_RESET);
                else if (v == 1) printf("%s#%s", C_GREEN, C_RESET);
                else if (v == 2) printf("%s+%s", C_BLUE, C_RESET);   /* dying */
                else             printf("%s.%s", C_GREY, C_RESET);
            }
        }
        pop = population();
        if (pop > peak) peak = pop;
        draw_textf(5 + H, 8, "Generation %-6d  Population %-5d  Peak %-5d  %s   ",
                   gen, pop, peak, running ? "running" : "paused ");
        scr_flush();

        if (running && now_ms() - last >= 90) {
            last = now_ms();
            step_rule(&ant_r, &ant_c, &ant_dir);
            gen++;
        }

        k = key_poll();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        if (k == ' ')             running = !running;
        if (k == 's' || k == 'S') { step_rule(&ant_r, &ant_c, &ant_dir); gen++; }
        if (k == 'r' || k == 'R') { seed_random(); gen = 0; peak = 0; ant_r = H/2; ant_c = W/2; ant_dir = 0; }
        if (k == KEY_UP    && cr > 0)     cr--;
        if (k == KEY_DOWN  && cr < H - 1) cr++;
        if (k == KEY_LEFT  && cc > 0)     cc--;
        if (k == KEY_RIGHT && cc < W - 1) cc++;
        if (k == KEY_ENTER) cell[cr][cc] = cell[cr][cc] ? 0 : 1;
        sleep_ms(10);
    }
    score_report(p->title ? p->title : "automata", peak);
    scr_clear();
    draw_centered(12, 80, "Simulation stopped.");
    pause_msg("Press any key...");
}
