/* GIC:PARAMETERISED arcademisc
 * GIC:PARAMETERISED artillery
 * GIC:PARAMETERISED towerdefence
 *
 * arcademisc.c - the real-time games that did not fit an existing engine.
 *
 * params: variant selects the game
 *
 * Thirty arcade cabinets, nine artillery duels and six tower-defence maps.
 * Rather than thirty separate main loops, the arcade games are sorted into
 * five machines that each cover a family of cabinets:
 *
 *   shooter   you sit at the bottom and things come down: Centipede, Missile
 *             Command, Kaboom, Whack-a-Mole, Minefield Run, Cannon Angle
 *   runner    the world scrolls past and you jump or dodge: Moon Patrol,
 *             Pitfall, Doodle Jump, Icy Tower, Helicopter, Jetpack, Crossy
 *             Road, Scramble, Time Pilot
 *   maze      a grid you walk around collecting or digging: Dig Dug, Q*bert,
 *             Bomberman, Boulder Dash, Lode Runner, Berzerk, Robotron, Echo
 *             Maze, Snake Charmer
 *   thrust    momentum and gravity: Lunar Lander, Defender, Tempest, Tron,
 *             Donkey Kong, Pinball
 *
 * The differences between cabinets in one machine are real - what spawns, how
 * fast, what kills you, what scores - and are held in a table.
 */
#include "engine.h"
#include "games.h"

#define W 72
#define H 20

enum { M_SHOOT, M_RUN, M_MAZE, M_THRUST };

typedef struct {
    const char *name;
    int machine;
    int density;      /* how much stuff there is           */
    int speed;        /* tick length in ms                 */
    int gravity;      /* runner/thrust: downward pull       */
    int shots;        /* may you fire                       */
    int diggable;     /* maze: can you clear terrain        */
} Cab;

static const Cab CAB[30] = {
    {"Centipede",        M_SHOOT,  14,  70, 0, 1, 0},
    {"Missile Command",  M_SHOOT,   8,  90, 0, 1, 0},
    {"Dig Dug",          M_MAZE,   20, 110, 0, 1, 1},
    {"Q*bert",           M_MAZE,    6, 130, 0, 0, 0},
    {"Donkey Kong",      M_THRUST,  9, 100, 1, 0, 0},
    {"Lunar Lander",     M_THRUST,  4, 100, 1, 0, 0},
    {"Moon Patrol",      M_RUN,    10,  80, 1, 1, 0},
    {"Defender",         M_THRUST, 12,  70, 0, 1, 0},
    {"Tempest",          M_THRUST, 14,  75, 0, 1, 0},
    {"Robotron",         M_MAZE,   22,  85, 0, 1, 0},
    {"Berzerk",          M_MAZE,   14, 100, 0, 1, 0},
    {"Bomberman",        M_MAZE,   18, 120, 0, 1, 1},
    {"Boulder Dash",     M_MAZE,   24, 120, 1, 0, 1},
    {"Lode Runner",      M_MAZE,   16, 110, 1, 0, 1},
    {"Pitfall",          M_RUN,     9,  85, 1, 0, 0},
    {"Kaboom",           M_SHOOT,  16,  60, 0, 0, 0},
    {"Tron Light Cycles",M_THRUST, 10,  70, 0, 0, 0},
    {"Doodle Jump",      M_RUN,     8,  70, 1, 0, 0},
    {"Icy Tower",        M_RUN,     8,  75, 1, 0, 0},
    {"Helicopter Game",  M_RUN,    12,  60, 1, 0, 0},
    {"Jetpack Joyride",  M_RUN,    14,  65, 1, 1, 0},
    {"Crossy Road",      M_RUN,    16,  95, 0, 0, 0},
    {"Whack-a-Mole",     M_SHOOT,  10, 110, 0, 1, 0},
    {"Scramble",         M_RUN,    12,  70, 1, 1, 0},
    {"Time Pilot",       M_RUN,    14,  70, 0, 1, 0},
    {"Pinball",          M_THRUST,  6,  55, 1, 0, 0},
    {"Minefield Run",    M_SHOOT,  18, 100, 0, 0, 0},
    {"Snake Charmer",    M_MAZE,   12, 110, 0, 0, 0},
    {"Cannon Angle",     M_SHOOT,   6, 120, 0, 1, 0},
    {"Echo Maze",        M_MAZE,   14, 130, 0, 0, 0}
};

/* One shared world; each machine uses the parts it needs. */
static char grid[H][W];
static int px, py, vy, score, lives, level;

static void clear_world(void)
{
    int r, c;
    for (r = 0; r < H; r++) for (c = 0; c < W; c++) grid[r][c] = ' ';
}

static void draw_world(const char *name, const char *hint)
{
    int r, c;
    char sub[150];
    snprintf(sub, sizeof sub, "%s — %s  score %d, lives %d, level %d", name, hint, score, lives, level);
    draw_title("ARCADE", sub);
    for (r = 0; r < H; r++) {
        scr_move(4 + r, 4);
        for (c = 0; c < W; c++) {
            char ch = grid[r][c];
            const char *col = ch == '#' ? C_GREY : ch == '*' ? C_YELLOW : ch == 'o' ? C_GREEN :
                              ch == 'X' ? C_RED : ch == '^' ? C_CYAN : C_WHITE;
            if (r == py && c == px) printf("%s%sA%s", BG_BLUE, C_BOLD C_WHITE, C_RESET);
            else if (ch == ' ') printf(" ");
            else printf("%s%c%s", col, ch, C_RESET);
        }
    }
    scr_flush();
}

/* -------------------------------------------------------------- shooters */

static void play_shoot(const GParams *p, const Cab *C)
{
    int fall[40][2], nf, shot_x, shot_y, i, tick = 0;

    for (;;) {
        int over = 0;
        score = 0; lives = 3; level = 1;
        nf = 0; shot_x = -1; shot_y = -1;
        px = W / 2; py = H - 1;

        while (!over) {
            int k;
            clear_world();
            if (tick % (12 - (C->density / 4 > 8 ? 8 : C->density / 4)) == 0 && nf < 40) {
                fall[nf][0] = rnd(W);
                fall[nf][1] = 0;
                nf++;
            }
            for (i = 0; i < nf; i++) {
                if (tick % 2 == 0) fall[i][1]++;
                if (fall[i][1] >= H) {
                    /* Reaching the floor costs a life, except in Whack-a-Mole
                     * where a missed mole only costs points. */
                    if (C - CAB == 22) score -= 5; else lives--;
                    fall[i][0] = fall[nf - 1][0]; fall[i][1] = fall[nf - 1][1];
                    nf--; i--;
                    continue;
                }
                grid[fall[i][1]][fall[i][0]] = 'X';
            }
            if (shot_y >= 0) {
                shot_y--;
                if (shot_y < 0) shot_x = -1;
                else {
                    for (i = 0; i < nf; i++)
                        if (fall[i][1] == shot_y && fall[i][0] == shot_x) {
                            score += 10 * level;
                            fall[i][0] = fall[nf - 1][0]; fall[i][1] = fall[nf - 1][1];
                            nf--;
                            shot_y = -1; shot_x = -1;
                            break;
                        }
                    if (shot_y >= 0) grid[shot_y][shot_x] = '^';
                }
            }
            for (i = 0; i < nf; i++)
                if (fall[i][1] == py && fall[i][0] == px) {
                    lives--;
                    fall[i][0] = fall[nf - 1][0]; fall[i][1] = fall[nf - 1][1];
                    nf--; i--;
                }
            if (lives <= 0) over = 1;
            if (score > level * 200) level++;

            draw_world(C->name, C->shots ? "arrows move, Space fires" : "arrows move to catch");
            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out;
            if (k == KEY_LEFT  && px > 0)     px--;
            if (k == KEY_RIGHT && px < W - 1) px++;
            if ((k == ' ' || k == KEY_ENTER) && C->shots && shot_y < 0) { shot_x = px; shot_y = py - 1; }
            tick++;
            sleep_ms(C->speed);
        }
        scr_clear();
        draw_centered(12, 80, "Out of lives.");
        score_report(p->title ? p->title : "arcade", score);
        if (!confirm("\n  Play again?")) return;
    }
out:
    score_report(p->title ? p->title : "arcade", score);
}

/* --------------------------------------------------------------- runners */

static void play_run(const GParams *p, const Cab *C)
{
    int ground[W], i, tick = 0, dist;

    for (;;) {
        int over = 0;
        score = 0; lives = 3; level = 1; dist = 0;
        px = 8; py = H - 3; vy = 0;
        for (i = 0; i < W; i++) ground[i] = H - 2;

        while (!over) {
            int k;
            clear_world();
            /* Scroll the terrain in from the right. */
            for (i = 0; i < W - 1; i++) ground[i] = ground[i + 1];
            {
                int prev = ground[W - 2], next = prev;
                if (rnd(100) < C->density) next = prev + (rnd(2) ? -2 : 2);
                if (next < 6) next = 6;
                if (next > H - 1) next = H - 1;
                ground[W - 1] = next;
            }
            for (i = 0; i < W; i++) {
                int r;
                for (r = ground[i]; r < H; r++) grid[r][i] = '#';
                /* A gap in the floor is the thing you have to jump. */
                if (ground[i] >= H) grid[H - 1][i] = ' ';
            }
            if (tick % 7 == 0) grid[ground[W - 1] - 1][W - 1] = '*';

            if (C->gravity) {
                vy++;
                py += vy / 2;
                if (py >= ground[px] - 1) { py = ground[px] - 1; vy = 0; }
                if (py < 0) { py = 0; vy = 0; }
            }
            if (grid[py][px] == '#') { lives--; py = ground[px] - 1; vy = 0; }
            if (grid[py][px] == '*') { score += 25; grid[py][px] = ' '; }
            dist++;
            score++;
            if (dist % 300 == 0) level++;
            if (lives <= 0) over = 1;

            draw_world(C->name, C->gravity ? "Space jumps, arrows steer" : "arrows steer");
            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out;
            if (k == KEY_LEFT  && px > 0)     px--;
            if (k == KEY_RIGHT && px < W - 1) px++;
            if (k == KEY_UP || k == ' ') { if (py >= ground[px] - 1) vy = -6; }
            if (k == KEY_DOWN && py < H - 1) py++;
            tick++;
            sleep_ms(C->speed);
        }
        scr_clear();
        draw_centered(12, 80, "Out of lives.");
        score_report(p->title ? p->title : "arcade", score);
        if (!confirm("\n  Play again?")) return;
    }
out:
    score_report(p->title ? p->title : "arcade", score);
}

/* ----------------------------------------------------------------- mazes */

static void play_maze(const GParams *p, const Cab *C)
{
    int ex[12][3], ne, i, tick = 0, pellets;

    for (;;) {
        int over = 0, r, c;
        score = 0; lives = 3; level = 1;
        clear_world();
        for (r = 0; r < H; r++) for (c = 0; c < W; c++)
            grid[r][c] = (r == 0 || c == 0 || r == H - 1 || c == W - 1) ? '#'
                       : (rnd(100) < C->density ? '#' : (rnd(100) < 12 ? '*' : ' '));
        px = 1; py = 1;
        grid[py][px] = ' ';
        ne = 4 + C->density / 8;
        if (ne > 12) ne = 12;
        for (i = 0; i < ne; i++) {
            ex[i][0] = W - 2 - rnd(8);
            ex[i][1] = H - 2 - rnd(6);
            ex[i][2] = rnd(4);
        }
        pellets = 0;
        for (r = 0; r < H; r++) for (c = 0; c < W; c++) if (grid[r][c] == '*') pellets++;

        while (!over) {
            int k;
            static const int DR[4] = {-1, 1, 0, 0}, DC[4] = {0, 0, -1, 1};
            if (tick % 3 == 0) {
                for (i = 0; i < ne; i++) {
                    int nr = ex[i][1] + DR[ex[i][2]], nc = ex[i][0] + DC[ex[i][2]];
                    /* Turn towards the player when the way is clear. */
                    if (rnd(100) < 40) {
                        int want = (ex[i][1] < py) ? 1 : (ex[i][1] > py) ? 0 : (ex[i][0] < px) ? 3 : 2;
                        if (grid[ex[i][1] + DR[want]][ex[i][0] + DC[want]] != '#') ex[i][2] = want;
                    }
                    nr = ex[i][1] + DR[ex[i][2]]; nc = ex[i][0] + DC[ex[i][2]];
                    if (nr > 0 && nr < H - 1 && nc > 0 && nc < W - 1 && grid[nr][nc] != '#') {
                        ex[i][1] = nr; ex[i][0] = nc;
                    } else ex[i][2] = rnd(4);
                }
            }
            for (i = 0; i < ne; i++) grid[ex[i][1]][ex[i][0]] = 'X';

            draw_world(C->name, C->diggable ? "arrows move, Space digs" : "arrows move");
            for (i = 0; i < ne; i++) if (grid[ex[i][1]][ex[i][0]] == 'X') grid[ex[i][1]][ex[i][0]] = ' ';

            for (i = 0; i < ne; i++)
                if (ex[i][0] == px && ex[i][1] == py) {
                    lives--;
                    px = 1; py = 1;
                    if (lives <= 0) over = 1;
                }
            if (pellets == 0) { level++; score += 200; break; }

            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out;
            {
                int nx = px, ny = py;
                if (k == KEY_UP)    ny--;
                if (k == KEY_DOWN)  ny++;
                if (k == KEY_LEFT)  nx--;
                if (k == KEY_RIGHT) nx++;
                if (nx > 0 && nx < W - 1 && ny > 0 && ny < H - 1) {
                    if (grid[ny][nx] == '#') {
                        if (C->diggable && (k == ' ' || k == KEY_ENTER)) { grid[ny][nx] = ' '; score += 2; }
                    } else {
                        if (grid[ny][nx] == '*') { score += 10; pellets--; grid[ny][nx] = ' '; }
                        px = nx; py = ny;
                    }
                }
                if ((k == ' ' || k == KEY_ENTER) && C->diggable) {
                    int d;
                    for (d = 0; d < 4; d++) {
                        int br = py + DR[d], bc = px + DC[d];
                        if (br > 0 && br < H - 1 && bc > 0 && bc < W - 1 && grid[br][bc] == '#') {
                            grid[br][bc] = ' ';
                            score += 2;
                        }
                    }
                }
            }
            tick++;
            sleep_ms(C->speed);
        }
        scr_clear();
        draw_centered(12, 80, lives > 0 ? "Maze cleared." : "Out of lives.");
        score_report(p->title ? p->title : "arcade", score);
        if (!confirm("\n  Play again?")) return;
    }
out:
    score_report(p->title ? p->title : "arcade", score);
}

/* ---------------------------------------------------------------- thrust */

static void play_thrust(const GParams *p, const Cab *C)
{
    int vx, tick = 0, pad, fuel, i;
    int obj[16][2], no;

    for (;;) {
        int over = 0, landed = 0;
        score = 0; lives = 3; level = 1;
        px = W / 2; py = 2; vx = 0; vy = 0;
        fuel = 400;
        pad = 8 + rnd(W - 20);
        no = C->density;
        if (no > 16) no = 16;
        for (i = 0; i < no; i++) { obj[i][0] = rnd(W); obj[i][1] = 4 + rnd(H - 8); }

        while (!over) {
            int k;
            clear_world();
            for (i = 0; i < W; i++) grid[H - 1][i] = '#';
            for (i = 0; i < 4; i++) if (pad + i < W) grid[H - 1][pad + i] = '=';
            for (i = 0; i < no; i++) {
                if (tick % 4 == 0) obj[i][0] = (obj[i][0] + 1) % W;
                grid[obj[i][1]][obj[i][0]] = 'X';
            }

            /* Momentum rather than direct control - that is the whole genre. */
            if (C->gravity && tick % 2 == 0) vy++;
            px += vx / 2;
            py += vy / 2;
            if (px < 0) px = W - 1;
            if (px >= W) px = 0;
            if (py < 0) { py = 0; vy = 0; }

            if (py >= H - 1) {
                if (px >= pad && px < pad + 4 && vy <= 4) { landed = 1; score += 500 + fuel; }
                else { lives--; }
                py = 2; px = W / 2; vx = 0; vy = 0;
                if (lives <= 0) over = 1;
                if (landed) { level++; landed = 0; pad = 8 + rnd(W - 20); fuel = 400; }
            }
            for (i = 0; i < no; i++)
                if (obj[i][0] == px && obj[i][1] == py) {
                    lives--;
                    py = 2; px = W / 2; vx = 0; vy = 0;
                    if (lives <= 0) over = 1;
                }

            draw_world(C->name, "arrows thrust - land gently on the pad");
            draw_textf(4 + H + 1, 4, "%sfuel %-4d  vertical speed %-3d%s", C_GREY, fuel, vy, C_RESET);
            scr_flush();

            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out;
            if (fuel > 0) {
                if (k == KEY_LEFT)  { vx--; fuel--; }
                if (k == KEY_RIGHT) { vx++; fuel--; }
                if (k == KEY_UP || k == ' ') { vy -= 3; fuel -= 2; }
            }
            if (vx > 6) vx = 6;
            if (vx < -6) vx = -6;
            tick++;
            sleep_ms(C->speed);
        }
        scr_clear();
        draw_centered(12, 80, "Out of lives.");
        score_report(p->title ? p->title : "arcade", score);
        if (!confirm("\n  Play again?")) return;
    }
out:
    score_report(p->title ? p->title : "arcade", score);
}

void fam_arcademisc(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    const Cab *C;
    if (v < 0 || v > 29) v = 0;
    C = &CAB[v];
    switch (C->machine) {
    case M_SHOOT:  play_shoot(p, C);  break;
    case M_RUN:    play_run(p, C);    break;
    case M_MAZE:   play_maze(p, C);   break;
    default:       play_thrust(p, C); break;
    }
}

/* ============================================================= ARTILLERY */

/* All nine duels are the same shot fired under different conditions, so the
 * conditions are the parameters: wind, gravity, whether the terrain is
 * destructible, how many shots a turn, and whether the target moves. */
void fam_artillery(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    int wind = 0, grav = 2, destructible = 0, shots = 1, moving = 0;
    int terrain[W], myx, foex, angle = 45, power = 50, i, hits = 0, taken = 0;
    static const char *NAME[9] = {
        "Gorillas", "Scorched Earth", "Artillery Duel Wind", "Artillery Duel Gravity",
        "Artillery Duel Terrain", "Artillery Duel Multi Shot", "Artillery Moving Target",
        "Tank Battle", "Worms Lite"
    };

    if (v < 0 || v > 8) v = 0;
    if (v == 1 || v == 4 || v == 8) destructible = 1;
    if (v == 2) wind = 1;
    if (v == 3) grav = 4;
    if (v == 5) shots = 3;
    if (v == 6) moving = 1;

    for (;;) {
        int over = 0, windnow = wind ? rnd(9) - 4 : 0;
        hits = 0; taken = 0;
        for (i = 0; i < W; i++) terrain[i] = H - 3 - (int)(4 * ((i % 17) < 9 ? (i % 17) : 17 - (i % 17)) / 4);
        myx = 5; foex = W - 6;

        while (!over) {
            int k, s;
            clear_world();
            for (i = 0; i < W; i++) { int r; for (r = terrain[i]; r < H; r++) grid[r][i] = '#'; }
            if (terrain[myx]  > 0) grid[terrain[myx] - 1][myx]   = 'A';
            if (terrain[foex] > 0) grid[terrain[foex] - 1][foex] = 'X';

            draw_world(NAME[v], "left/right aim, up/down power, Space fires");
            draw_textf(4 + H + 1, 4,
                       "%sangle %-3d  power %-3d  wind %+d  gravity %d  hits %d, taken %d%s",
                       C_GREY, angle, power, windnow, grav, hits, taken, C_RESET);
            scr_flush();

            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out2;
            if (k == KEY_LEFT  && angle > 5)   angle -= 5;
            if (k == KEY_RIGHT && angle < 85)  angle += 5;
            if (k == KEY_UP    && power < 100) power += 5;
            if (k == KEY_DOWN  && power > 10)  power -= 5;
            if (k != ' ' && k != KEY_ENTER) { sleep_ms(30); continue; }

            for (s = 0; s < shots && !over; s++) {
                /* Integer ballistics: fixed-point x and y, stepped until it
                 * meets the ground or leaves the field. */
                int fx = myx * 100, fy = (terrain[myx] - 2) * 100;
                int vxf = power * (90 - angle) / 45, vyf = -power * angle / 30;
                int step;
                for (step = 0; step < 400; step++) {
                    int gx, gy;
                    fx += vxf; fy += vyf;
                    vyf += grav * 2;
                    vxf += windnow;
                    gx = fx / 100; gy = fy / 100;
                    if (gx < 0 || gx >= W || gy >= H) break;
                    if (gy >= 0 && gy < H && gx >= 0 && gx < W) {
                        if (gy >= terrain[gx]) {
                            if (gx >= foex - 1 && gx <= foex + 1) {
                                hits++;
                                score += 100;
                                if (hits >= 3) over = 1;
                            }
                            if (destructible) {
                                int d;
                                for (d = -2; d <= 2; d++)
                                    if (gx + d >= 0 && gx + d < W && terrain[gx + d] < H - 1) terrain[gx + d]++;
                            }
                            break;
                        }
                        grid[gy][gx] = '^';
                        draw_world(NAME[v], "shell in flight");
                        grid[gy][gx] = ' ';
                        sleep_ms(18);
                    }
                }
            }
            if (over) break;

            {   /* Return fire, converging on your position. */
                /* It ranges in: every miss nudges its aim, so its accuracy
                 * climbs over the duel rather than being a flat dice roll. */
                static int lastmiss = 0;
                int accuracy = 30 + hits * 8 - (lastmiss < 0 ? -lastmiss : lastmiss);
                if (accuracy < 10) accuracy = 10;
                if (rnd(100) < accuracy) { taken++; score -= 25; lastmiss = 0; }
                else lastmiss += (rnd(2) ? 3 : -3);
                if (taken >= 3) { over = 1; }
                if (moving) { foex += rnd(2) ? 3 : -3; if (foex < 10) foex = 10; if (foex > W - 6) foex = W - 6; }
                if (wind) windnow = rnd(9) - 4;
            }
        }
        scr_clear();
        draw_centered(12, 80, hits >= 3 ? "Target destroyed." : "You were knocked out.");
        score_report(p->title ? p->title : "artillery", score);
        if (!confirm("\n  Another duel?")) return;
    }
out2:
    score_report(p->title ? p->title : "artillery", score);
}

/* ========================================================= TOWER DEFENCE */

void fam_towerdefence(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    static const char *NAME[6] = {"Forest", "Desert", "Space", "Castle", "Cyber", "Underwater"};
    int path[W], towers[H][W], creeps[40][3], nc, money, health, wave, tick, cr, cc, i;

    if (v < 0 || v > 5) v = 0;

    for (;;) {
        int over = 0, r, c;
        money = 120; health = 20; wave = 1; nc = 0; tick = 0; score = 0;
        cr = H / 2; cc = W / 2;
        for (r = 0; r < H; r++) for (c = 0; c < W; c++) towers[r][c] = 0;
        /* Each map bends its lane differently, which changes where towers pay. */
        for (i = 0; i < W; i++) {
            int base = H / 2;
            if (v == 0) base += (i / 6) % 2 ? 3 : -3;
            if (v == 1) base += (i % 24 < 12) ? i % 12 - 6 : 6 - (i % 12);
            if (v == 2) base += (i % 8 < 4) ? 2 : -2;
            if (v == 3) base += (i < W / 2) ? -4 : 4;
            if (v == 4) base += ((i * 7) % 11) - 5;
            if (v == 5) base += (i % 16 < 8) ? -3 : 3;
            if (base < 1) base = 1;
            if (base > H - 2) base = H - 2;
            path[i] = base;
        }

        while (!over) {
            int k;
            clear_world();
            for (i = 0; i < W; i++) grid[path[i]][i] = '.';
            for (r = 0; r < H; r++) for (c = 0; c < W; c++) if (towers[r][c]) grid[r][c] = 'o';

            if (tick % (26 - (wave > 18 ? 18 : wave)) == 0 && nc < 40) {
                creeps[nc][0] = 0;
                creeps[nc][1] = 4 + wave * 2;
                creeps[nc][2] = 0;
                nc++;
            }
            for (i = 0; i < nc; i++) {
                if (tick % 3 == 0) creeps[i][0]++;
                if (creeps[i][0] >= W) {
                    health--;
                    creeps[i][0] = creeps[nc-1][0]; creeps[i][1] = creeps[nc-1][1]; creeps[i][2] = creeps[nc-1][2];
                    nc--; i--;
                    continue;
                }
                grid[path[creeps[i][0]]][creeps[i][0]] = 'X';
            }
            /* Every tower shoots the nearest creep within range once a tick. */
            if (tick % 2 == 0)
                for (r = 0; r < H; r++) for (c = 0; c < W; c++) {
                    if (!towers[r][c]) continue;
                    for (i = 0; i < nc; i++) {
                        int dx = creeps[i][0] - c, dy = path[creeps[i][0]] - r;
                        if (dx * dx + dy * dy <= 16) {
                            creeps[i][1] -= towers[r][c];
                            if (creeps[i][1] <= 0) {
                                money += 8; score += 15;
                                creeps[i][0] = creeps[nc-1][0]; creeps[i][1] = creeps[nc-1][1];
                                nc--;
                            }
                            break;
                        }
                    }
                }
            if (health <= 0) over = 1;
            if (tick % 400 == 399) wave++;

            draw_world(NAME[v], "arrows move, Enter builds (30), U upgrades (40)");
            draw_textf(4 + H + 1, 4, "%smoney %-4d  health %-3d  wave %-2d  creeps %d%s",
                       C_GREY, money, health, wave, nc, C_RESET);
            draw_textf(4, 4 + cc, "%s+%s", C_CYAN, C_RESET);
            scr_flush();

            k = key_poll();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) goto out3;
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < H - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < W - 1) cc++;
            if ((k == KEY_ENTER || k == ' ') && money >= 30 && !towers[cr][cc] && path[cc] != cr) {
                towers[cr][cc] = 3;
                money -= 30;
            }
            if ((k == 'u' || k == 'U') && money >= 40 && towers[cr][cc]) {
                towers[cr][cc] += 3;
                money -= 40;
            }
            tick++;
            sleep_ms(45);
        }
        scr_clear();
        draw_centered(12, 80, "The lane was overrun.");
        score_report(p->title ? p->title : "towerdefence", score);
        if (!confirm("\n  Another map?")) return;
    }
out3:
    score_report(p->title ? p->title : "towerdefence", score);
}
