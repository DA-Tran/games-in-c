/* GIC:PARAMETERISED mancala
 * mancala.c - sowing games across the real regional rule sets.
 *
 * params: width   = houses per side (4..8)
 *         count   = seeds per house
 *         variant = 0 Kalah      1 Oware    2 Congkak  3 Sungka
 *                   4 Ayo        5 Dakon    6 Pallanguzhi
 *                   7 Toguz Kumalak         8 Bao (two-row adaptation)
 *
 * The rule sets genuinely differ, and each is expressed as a set of flags
 * rather than a separate engine:
 *
 *   store_sowing  seeds are dropped into your own store as you pass it
 *   free_turn     ending in your own store earns another turn
 *   relay         ending in an occupied house picks that house up and
 *                 keeps sowing (the Congkak/Sungka/Dakon family)
 *   capture       0 none
 *                 1 Kalah  - last seed into an empty own house takes the
 *                            facing house
 *                 2 Oware  - last seed making an enemy house hold 2 or 3
 *                            captures it, and walks backwards
 *                 3 Pallanguzhi - any house reaching 4 seeds is captured
 *   tuzdik        Toguz Kumalak: claim one enemy house as a second store
 */
#include "engine.h"
#include "games.h"

#define MAXH 8
#define PITS (MAXH * 2 + 2)

static int HOUSES, SEEDS, VARIANT;
static int pit[PITS];              /* 0..H-1 you, H your store, H+1.. cpu, last cpu store */
static int tuz[2];                 /* claimed tuzdik house index, or -1     */

static int store_sowing, free_turn, relay, capture_rule, use_tuzdik;

static int my_store(void)  { return HOUSES; }
static int cpu_store(void) { return HOUSES * 2 + 1; }
static int total_pits(void){ return HOUSES * 2 + 2; }

static const char *rule_name(void)
{
    static const char *N[9] = {"Kalah", "Oware", "Congkak", "Sungka", "Ayo",
                               "Dakon", "Pallanguzhi", "Toguz Kumalak", "Bao"};
    return N[VARIANT];
}

static void configure(void)
{
    store_sowing = 1; free_turn = 0; relay = 0; capture_rule = 1; use_tuzdik = 0;
    switch (VARIANT) {
        case 0: free_turn = 1; capture_rule = 1; break;                 /* Kalah */
        case 1: store_sowing = 0; capture_rule = 2; break;              /* Oware */
        case 2: case 3: case 5: relay = 1; free_turn = 1; break;        /* Congkak/Sungka/Dakon */
        case 4: store_sowing = 0; capture_rule = 2; break;              /* Ayo */
        case 6: capture_rule = 3; relay = 1; break;                     /* Pallanguzhi */
        case 7: use_tuzdik = 1; capture_rule = 0; break;                /* Toguz Kumalak */
        case 8: relay = 1; capture_rule = 1; break;                     /* Bao adaptation */
        default: break;
    }
}

static int owns(int human, int idx)
{
    return human ? (idx >= 0 && idx < HOUSES)
                 : (idx > HOUSES && idx < cpu_store());
}

static int side_empty(int human)
{
    int i, lo = human ? 0 : HOUSES + 1, s = 0;
    for (i = lo; i < lo + HOUSES; i++) s += pit[i];
    return s == 0;
}

/* Sow from p. Returns 1 when the mover earns another turn. */
static int sow(int p, int human)
{
    int seeds, cur = p, guard = 0;
    int mine = human ? my_store() : cpu_store();
    int theirs = human ? cpu_store() : my_store();

    for (;;) {
        seeds = pit[cur];
        pit[cur] = 0;
        if (seeds == 0) return 0;

        while (seeds > 0) {
            cur = (cur + 1) % total_pits();
            if (cur == theirs) continue;                 /* never fill their store */
            if (cur == mine && !store_sowing) continue;  /* Oware has no stores in play */
            /* A claimed tuzdik always feeds its owner. */
            if (use_tuzdik && cur == tuz[human ? 0 : 1]) {
                pit[mine]++;
                seeds--;
                continue;
            }
            pit[cur]++;
            seeds--;
        }

        if (cur == mine) return free_turn;

        if (relay && pit[cur] > 1 && ++guard < 200) continue;   /* pick up and keep going */
        break;
    }

    /* ---------------------------------------------------------- capture */
    if (capture_rule == 1) {
        if (pit[cur] == 1 && owns(human, cur)) {
            int opp = (HOUSES * 2) - cur;
            if (opp >= 0 && opp < total_pits() && opp != cur && pit[opp] > 0) {
                pit[mine] += pit[opp] + 1;
                pit[opp] = 0;
                pit[cur] = 0;
            }
        }
    } else if (capture_rule == 2) {
        /* Oware: walk backwards taking enemy houses holding two or three. */
        while (!owns(human, cur) && cur != mine && cur != theirs &&
               (pit[cur] == 2 || pit[cur] == 3)) {
            pit[mine] += pit[cur];
            pit[cur] = 0;
            cur = (cur - 1 + total_pits()) % total_pits();
        }
    } else if (capture_rule == 3) {
        int i;
        for (i = 0; i < total_pits(); i++) {
            if (i == mine || i == theirs) continue;
            if (pit[i] == 4) { pit[mine] += 4; pit[i] = 0; }
        }
    }

    /* Toguz Kumalak: an enemy house left holding exactly three becomes yours. */
    if (use_tuzdik && !owns(human, cur) && pit[cur] == 3 &&
        tuz[human ? 0 : 1] < 0 && cur != mine && cur != theirs) {
        tuz[human ? 0 : 1] = cur;
        pit[mine] += 3;
        pit[cur] = 0;
    }
    return 0;
}

static void ai_turn(void)
{
    int guard = 0;
    for (;;) {
        int i, best = -100000, bi = -1;
        for (i = HOUSES + 1; i < cpu_store(); i++) {
            int save[PITS], savetuz[2], v, again;
            if (pit[i] == 0) continue;
            memcpy(save, pit, sizeof pit);
            savetuz[0] = tuz[0]; savetuz[1] = tuz[1];
            again = sow(i, 0);
            v = (pit[cpu_store()] - save[cpu_store()]) * 10
              - (pit[my_store()] - save[my_store()]) * 8
              + (again ? 15 : 0);
            memcpy(pit, save, sizeof pit);
            tuz[0] = savetuz[0]; tuz[1] = savetuz[1];
            if (v > best) { best = v; bi = i; }
        }
        if (bi < 0) return;
        if (!sow(bi, 0)) return;
        if (side_empty(0) || ++guard > 12) return;
    }
}

static void render(int cur, const char *msg)
{
    int i, left = 40 - (HOUSES * 5) / 2;
    char sub[110];
    snprintf(sub, sizeof sub, "%s - %d houses, %d seeds - Left/Right pick, Enter sows",
             rule_name(), HOUSES, SEEDS);
    draw_title("MANCALA", sub);

    scr_move(7, left - 4);
    printf("%sCPU%s ", C_RED, C_RESET);
    for (i = cpu_store() - 1; i > HOUSES; i--)
        printf("%s[%2d]%s ", (use_tuzdik && i == tuz[0]) ? C_GREEN : C_RED, pit[i], C_RESET);

    draw_textf(8, left - 10, "%sCPU store %3d%s", C_RED, pit[cpu_store()], C_RESET);
    draw_textf(8, left + HOUSES * 5 + 2, "%sYour store %3d%s", C_CYAN, pit[my_store()], C_RESET);

    scr_move(9, left - 4);
    printf("%sYOU%s ", C_CYAN, C_RESET);
    for (i = 0; i < HOUSES; i++)
        printf("%s%s[%2d]%s ", i == cur ? C_REV : "",
               (use_tuzdik && i == tuz[1]) ? C_GREEN : C_CYAN, pit[i], C_RESET);

    if (use_tuzdik)
        draw_textf(11, left - 4, "%sGreen marks a claimed tuzdik.%s   ", C_GREY, C_RESET);
    draw_text(13, 12, "                                                              ");
    if (msg) draw_textf(13, left - 4, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_mancala(const GParams *p)
{
    HOUSES = gp_int(p->width, 6);
    if (HOUSES < 4) HOUSES = 4;
    if (HOUSES > MAXH) HOUSES = MAXH;
    SEEDS = gp_int(p->count, 4);
    if (SEEDS < 2) SEEDS = 2;
    if (SEEDS > 9) SEEDS = 9;
    VARIANT = (p->variant >= 0 && p->variant <= 8) ? p->variant : 0;
    configure();

    for (;;) {
        int cur = 0, i, over = 0;
        for (i = 0; i < total_pits(); i++) pit[i] = SEEDS;
        pit[my_store()] = pit[cpu_store()] = 0;
        tuz[0] = tuz[1] = -1;

        while (!over) {
            int k;
            render(cur, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_LEFT  && cur > 0) cur--;
            if (k == KEY_RIGHT && cur < HOUSES - 1) cur++;
            if (k != KEY_ENTER && k != ' ') continue;
            if (pit[cur] == 0) continue;

            if (sow(cur, 1)) {
                render(cur, "Another turn.");
                sleep_ms(700);
            } else if (!side_empty(1)) {
                ai_turn();
            }
            if (side_empty(1) || side_empty(0)) {
                for (i = 0; i < HOUSES; i++)  { pit[my_store()]  += pit[i]; pit[i] = 0; }
                for (i = HOUSES + 1; i < cpu_store(); i++) { pit[cpu_store()] += pit[i]; pit[i] = 0; }
                render(cur, pit[my_store()] > pit[cpu_store()] ? "You win!" :
                            pit[my_store()] < pit[cpu_store()] ? "Computer wins." : "Draw.");
                score_report(p->title ? p->title : "mancala", pit[my_store()]);
                over = 1;
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
