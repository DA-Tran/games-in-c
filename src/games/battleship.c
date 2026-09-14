/* battleship.c - 10x10 fleet duel; the AI hunts with parity then targets. */
#include "engine.h"
#include "games.h"

#define N 10
#define FLEET 5

static const int SHIP_LEN[FLEET] = {5, 4, 3, 3, 2};
static const char *SHIP_NAME[FLEET] = {"Carrier", "Battleship", "Cruiser", "Submarine", "Destroyer"};

/* 0 empty, 1 ship, 2 miss, 3 hit */
static int you[N][N], cpu[N][N];
static int cpu_queue[4][2], cpu_qn;

static int can_place(int g[N][N], int r, int c, int len, int horiz)
{
    int i;
    if (horiz ? c + len > N : r + len > N) return 0;
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
            int horiz = rnd(2), r = rnd(N), c = rnd(N);
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
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (g[r][c] == 1) n++;
    return n;
}

static void push_target(int r, int c)
{
    if (r < 0 || r >= N || c < 0 || c >= N) return;
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
            r = rnd(N); c = rnd(N);
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
    draw_text(top, left, "   0 1 2 3 4 5 6 7 8 9");
    for (r = 0; r < N; r++) {
        scr_move(top + 1 + r, left);
        printf("%c ", 'A' + r);
        for (c = 0; c < N; c++) {
            int v = g[r][c];
            int sel = (cr == r && cc == c);
            const char *bg = sel ? BG_BLUE : "";
            if (v == 3)                   printf("%s%s X %s", bg, C_RED, C_RESET);
            else if (v == 2)              printf("%s%s o %s", bg, C_GREY, C_RESET);
            else if (v == 1 && !hide)     printf("%s%s ▩ %s", bg, C_CYAN, C_RESET);
            else                          printf("%s%s · %s", bg, C_BLUE, C_RESET);
        }
    }
}

static void render(int cr, int cc, const char *msg)
{
    draw_title("BATTLESHIP", "Arrows aim, Enter fires, Q quits");
    draw_textf(6, 8, "%sYOUR FLEET%s", C_CYAN, C_RESET);
    draw_grid(7, 8, you, 0, -1, -1);
    draw_textf(6, 46, "%sENEMY WATERS%s", C_RED, C_RESET);
    draw_grid(7, 46, cpu, 1, cr, cc);
    draw_textf(19, 8,  "Ships afloat: %d      ", remaining(you));
    draw_textf(19, 46, "Enemy afloat: %d      ", remaining(cpu));
    draw_text(21, 8, "                                                            ");
    if (msg) draw_textf(21, 8, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_battleship(const GParams *p)
{
    (void)p;
    for (;;) {
        int cr = 0, cc = 0, over = 0, i;
        place_fleet(you);
        place_fleet(cpu);
        cpu_qn = 0;

        scr_clear();
        draw_title("BATTLESHIP", "Fleets deployed");
        for (i = 0; i < FLEET; i++)
            draw_textf(7 + i, 30, "%-12s %d cells", SHIP_NAME[i], SHIP_LEN[i]);
        pause_msg("Press any key to begin...");

        while (!over) {
            int k;
            render(cr, cc, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < N - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < N - 1) cc++;
            if (k != KEY_ENTER && k != ' ') continue;
            if (cpu[cr][cc] == 2 || cpu[cr][cc] == 3) continue;

            if (cpu[cr][cc] == 1) { cpu[cr][cc] = 3; render(cr, cc, "HIT!"); }
            else                  { cpu[cr][cc] = 2; render(cr, cc, "Miss."); }
            sleep_ms(450);
            if (remaining(cpu) == 0) { render(cr, cc, "Enemy fleet destroyed - you win!"); over = 1; break; }

            cpu_shot();
            if (remaining(you) == 0) { render(cr, cc, "Your fleet is lost - computer wins."); over = 1; }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
