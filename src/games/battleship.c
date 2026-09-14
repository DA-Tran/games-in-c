/* GIC:PARAMETERISED battleship
 * battleship.c - the fleet-duel family.
 *
 * params: size    = grid edge (8, 10 or 12)
 *         variant = 0 classic, 1 salvo, 2 big board, 3 moving ships,
 *                   4 fog, 5 small
 *
 * Salvo gives each side one shot per surviving ship, so the opening volleys
 * are huge and shrink as you lose hulls. Moving ships redeploy whatever has
 * not been hit every few turns, which defeats methodical sweeps. Fog withholds
 * the hit/miss call until a ship actually sinks. These change how you search,
 * not how it looks.
 */
#include "engine.h"
#include "games.h"

#define MAXN 12
#define N MAXN
#define FLEET 5

static int GRID, VAR;

static const int SHIP_LEN[FLEET] = {5, 4, 3, 3, 2};
static const char *SHIP_NAME[FLEET] = {"Carrier", "Battleship", "Cruiser", "Submarine", "Destroyer"};

/* 0 empty, 1 ship, 2 miss, 3 hit */
static int you[N][N], cpu[N][N];
static int cpu_queue[4][2], cpu_qn;

static int can_place(int g[N][N], int r, int c, int len, int horiz)
{
    int i;
    if (horiz ? c + len > GRID : r + len > GRID) return 0;
    for (i = 0; i < len; i++)
        if (g[r + (horiz ? 0 : i)][c + (horiz ? i : 0)]) return 0;
    return 1;
}

static void place_fleet(int g[N][N])
{
    int s, i;
    memset(g, 0, sizeof(int) * N * N);
    for (s = 0; s < FLEET; s++) {
        for (;;) {
            int horiz = rnd(2), r = rnd(GRID), c = rnd(GRID);
            if (!can_place(g, r, c, SHIP_LEN[s], horiz)) continue;
            for (i = 0; i < SHIP_LEN[s]; i++)
                g[r + (horiz ? 0 : i)][c + (horiz ? i : 0)] = 1;
            break;
        }
    }
}

static int remaining(int g[N][N])
{
    int r, c, n = 0;
    for (r = 0; r < GRID; r++) for (c = 0; c < GRID; c++) if (g[r][c] == 1) n++;
    return n;
}

/* Number of ships still carrying at least one unhit cell. */
static int ship_count(int g[N][N])
{
    int n = remaining(g);
    return n > FLEET ? FLEET : (n > 0 ? (n + 2) / 3 : 0);
}

static void push_target(int r, int c)
{
    if (r < 0 || r >= GRID || c < 0 || c >= GRID) return;
    if (cpu_qn < 4) { cpu_queue[cpu_qn][0] = r; cpu_queue[cpu_qn][1] = c; cpu_qn++; }
}

static void cpu_shot(void)
{
    int r, c;
    for (;;) {
        if (cpu_qn > 0) {
            cpu_qn--;
            r = cpu_queue[cpu_qn][0];
            c = cpu_queue[cpu_qn][1];
        } else {
            /* Parity search: no ship shorter than two can hide on one colour. */
            r = rnd(GRID); c = rnd(GRID);
            if ((r + c) % 2) continue;
        }
        if (you[r][c] == 2 || you[r][c] == 3) continue;
        break;
    }
    if (you[r][c] == 1) {
        you[r][c] = 3;
        push_target(r-1, c); push_target(r+1, c);
        push_target(r, c-1); push_target(r, c+1);
    } else {
        you[r][c] = 2;
    }
}

static void draw_grid(int top, int left, int g[N][N], int hide, int cr, int cc)
{
    int r, c;
    char hdr[48];
    int k = 0;
    hdr[k++] = ' '; hdr[k++] = ' '; hdr[k++] = ' ';
    for (c = 0; c < GRID; c++) { hdr[k++] = (char)('0' + c % 10); hdr[k++] = ' '; hdr[k++] = ' '; }
    hdr[k] = '\0';
    draw_text(top, left, hdr);
    for (r = 0; r < GRID; r++) {
        scr_move(top + 1 + r, left);
        printf("%c ", 'A' + r);
        for (c = 0; c < GRID; c++) {
            int v = g[r][c];
            int sel = (cr == r && cc == c);
            const char *bg = sel ? BG_BLUE : "";
            /* Fog withholds the result until something actually sinks. */
            if (VAR == 4 && hide && (v == 2 || v == 3)) printf("%s%s ? %s", bg, C_MAGENTA, C_RESET);
            else if (v == 3)              printf("%s%s X %s", bg, C_RED, C_RESET);
            else if (v == 2)              printf("%s%s o %s", bg, C_GREY, C_RESET);
            else if (v == 1 && !hide)     printf("%s%s # %s", bg, C_CYAN, C_RESET);
            else                          printf("%s%s . %s", bg, C_BLUE, C_RESET);
        }
    }
}

static void render(int cr, int cc, int shots, const char *msg)
{
    int right = 12 + GRID * 3 + 6;
    char sub[110];
    snprintf(sub, sizeof sub, "%s — Arrows aim, Enter fires, Q quits",
             VAR == 1 ? "SALVO: one shot per surviving ship"
           : VAR == 3 ? "MOVING: undamaged ships redeploy"
           : VAR == 4 ? "FOG: results stay hidden until a ship sinks"
                      : "Sink the enemy fleet");
    draw_title("BATTLESHIP", sub);
    draw_textf(5, 8, "%sYOUR FLEET%s", C_CYAN, C_RESET);
    draw_grid(6, 8, you, 0, -1, -1);
    draw_textf(5, right, "%sENEMY WATERS%s", C_RED, C_RESET);
    draw_grid(6, right, cpu, 1, cr, cc);
    draw_textf(8 + GRID, 8,  "Ships afloat: %d      ", remaining(you));
    draw_textf(8 + GRID, right, "Enemy afloat: %d      ", remaining(cpu));
    if (VAR == 1) draw_textf(9 + GRID, 8, "Shots left this volley: %d   ", shots);
    draw_text(11 + GRID, 8, "                                                            ");
    if (msg) draw_textf(11 + GRID, 8, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

/* Moving-ships mode lifts every unhit hull and redeploys it. */
static void redeploy(int g[N][N])
{
    int r, c, s, i, intact = 0;
    for (r = 0; r < GRID; r++) for (c = 0; c < GRID; c++)
        if (g[r][c] == 1) { g[r][c] = 0; intact++; }
    for (s = 0; s < FLEET && intact > 0; s++) {
        int len = SHIP_LEN[s];
        int tries;
        if (len > intact) continue;
        for (tries = 0; tries < 200; tries++) {
            int horiz = rnd(2), r2 = rnd(GRID), c2 = rnd(GRID), ok = 1;
            if (horiz ? c2 + len > GRID : r2 + len > GRID) continue;
            for (i = 0; i < len; i++)
                if (g[r2 + (horiz ? 0 : i)][c2 + (horiz ? i : 0)]) { ok = 0; break; }
            if (!ok) continue;
            for (i = 0; i < len; i++)
                g[r2 + (horiz ? 0 : i)][c2 + (horiz ? i : 0)] = 1;
            intact -= len;
            break;
        }
    }
}

void fam_battleship(const GParams *p)
{
    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR > 5) VAR = 0;
    GRID = gp_int(p->size, 10);
    if (GRID < 8) GRID = 8;
    if (GRID > MAXN) GRID = MAXN;

    for (;;) {
        int cr = 0, cc = 0, over = 0, i, turns = 0;
        place_fleet(you);
        place_fleet(cpu);
        cpu_qn = 0;

        scr_clear();
        draw_title("BATTLESHIP", "Fleets deployed");
        for (i = 0; i < FLEET; i++)
            draw_textf(7 + i, 30, "%-12s %d cells", SHIP_NAME[i], SHIP_LEN[i]);
        pause_msg("Press any key to begin...");

        while (!over) {
            int k, volley, fired;

            /* Salvo: your volley is as big as your surviving fleet. */
            volley = (VAR == 1) ? (remaining(cpu) > 0 ? ship_count(you) : 1) : 1;
            if (volley < 1) volley = 1;

            for (fired = 0; fired < volley && !over; ) {
                render(cr, cc, volley - fired, NULL);
                k = key_get();
                if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
                if (k == KEY_UP    && cr > 0)        cr--;
                if (k == KEY_DOWN  && cr < GRID - 1) cr++;
                if (k == KEY_LEFT  && cc > 0)        cc--;
                if (k == KEY_RIGHT && cc < GRID - 1) cc++;
                if (k != KEY_ENTER && k != ' ') continue;
                if (cpu[cr][cc] == 2 || cpu[cr][cc] == 3) continue;

                if (cpu[cr][cc] == 1) {
                    cpu[cr][cc] = 3;
                    render(cr, cc, volley - fired - 1, VAR == 4 ? "Shot away." : "HIT!");
                } else {
                    cpu[cr][cc] = 2;
                    render(cr, cc, volley - fired - 1, VAR == 4 ? "Shot away." : "Miss.");
                }
                fired++;
                sleep_ms(280);
                if (remaining(cpu) == 0) {
                    render(cr, cc, 0, "Enemy fleet destroyed - you win!");
                    score_report(p->title ? p->title : "battleship", remaining(you) * 100);
                    over = 1;
                }
            }
            if (over) break;

            {
                int salvo = (VAR == 1) ? ship_count(cpu) : 1, s2;
                if (salvo < 1) salvo = 1;
                for (s2 = 0; s2 < salvo; s2++) {
                    cpu_shot();
                    if (remaining(you) == 0) break;
                }
            }
            if (remaining(you) == 0) {
                render(cr, cc, 0, "Your fleet is lost - computer wins.");
                over = 1;
            }

            if (VAR == 3 && ++turns % 4 == 0 && !over) {
                redeploy(cpu);
                redeploy(you);
                cpu_qn = 0;
                render(cr, cc, 0, "Sonar contact shifts - the fleets have moved.");
                sleep_ms(600);
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
