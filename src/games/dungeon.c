/* GIC:PARAMETERISED dungeon
 * dungeon.c - compact roguelike: generated rooms, combat, loot and descent.
 *
 * params: theme = flavour set, or variant = its index if theme is unset
 *         level = how many floors you must clear to win
 *
 * The theme picks the monster roster and the environmental hazard that
 * bites once per turn in the open, so a Volcano run really does play
 * differently from an Ice Caves run rather than just reading differently.
 */
#include "engine.h"
#include "games.h"

#define W 64
#define H 20
#define MAXROOM 9
#define MAXMON 12

typedef struct { int x, y, w, h; } Room;
typedef struct { int x, y, hp, atk, alive; char glyph; } Mon;

/* Per-theme roster and hazard. HAZARD: 0 none, 1 poison, 2 slippery,
 * 3 burning, 4 falling, 5 vacuum, 6 blocked sight. */
typedef struct {
    const char *name;
    const char *glyphs;
    const char *hazard_text;
    int hazard;
} Theme;

static const Theme THEMES[] = {
    {"Catacombs",     "zZsSwW", "the crypt air saps you",        1},
    {"Caverns",       "bBrRtT", "loose rock underfoot",          2},
    {"Sewers",        "rRsSgG", "the fumes burn",                1},
    {"Ice Caves",     "iIwWyY", "you slide on the ice",          2},
    {"Volcano",       "fFdDmM", "the heat scorches",             3},
    {"Sky Temple",    "aAhHgG", "the wind tears at you",         4},
    {"Derelict Ship", "dDkKxX", "a hull breach hisses",          5},
    {"Forest Depths", "wWbBsS", "dense growth blocks your view", 6}
};
#define NTHEMES ((int)(sizeof(THEMES) / sizeof(THEMES[0])))

static int THEME_ID, TARGET_DEPTH;

static char map[H][W];
static int  seen[H][W];
static Room room[MAXROOM];
static int  nroom;
static Mon  mon[MAXMON];
static int  px, py, hp, maxhp, atk, gold, depth, potions;
static char log1[96], msglog2[96];

static void logmsg(const char *s)
{
    strncpy(msglog2, log1, sizeof msglog2 - 1);
    msglog2[sizeof msglog2 - 1] = '\0';
    strncpy(log1, s, sizeof log1 - 1);
    log1[sizeof log1 - 1] = '\0';
}

static void carve_h(int x1, int x2, int y)
{
    int x;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    for (x = x1; x <= x2; x++) if (y > 0 && y < H - 1) map[y][x] = '.';
}

static void carve_v(int y1, int y2, int x)
{
    int y;
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    for (y = y1; y <= y2; y++) if (x > 0 && x < W - 1) map[y][x] = '.';
}

static void generate(void)
{
    int i, r, c, tries;
    for (r = 0; r < H; r++) for (c = 0; c < W; c++) { map[r][c] = '#'; seen[r][c] = 0; }
    nroom = 0;

    for (tries = 0; tries < 120 && nroom < MAXROOM; tries++) {
        Room n;
        int ok = 1;
        n.w = rnd_range(5, 12);
        n.h = rnd_range(3, 5);
        n.x = rnd_range(1, W - n.w - 2);
        n.y = rnd_range(1, H - n.h - 2);
        for (i = 0; i < nroom; i++) {
            if (n.x < room[i].x + room[i].w + 1 && n.x + n.w + 1 > room[i].x &&
                n.y < room[i].y + room[i].h + 1 && n.y + n.h + 1 > room[i].y) { ok = 0; break; }
        }
        if (!ok) continue;
        for (r = n.y; r < n.y + n.h; r++)
            for (c = n.x; c < n.x + n.w; c++) map[r][c] = '.';
        /* Connect each new room to the previous one with an L corridor. */
        if (nroom > 0) {
            int cx = n.x + n.w / 2, cy = n.y + n.h / 2;
            int px_ = room[nroom-1].x + room[nroom-1].w / 2;
            int py_ = room[nroom-1].y + room[nroom-1].h / 2;
            if (rnd(2)) { carve_h(px_, cx, py_); carve_v(py_, cy, cx); }
            else        { carve_v(py_, cy, px_); carve_h(px_, cx, cy); }
        }
        room[nroom++] = n;
    }

    px = room[0].x + room[0].w / 2;
    py = room[0].y + room[0].h / 2;
    map[room[nroom-1].y + room[nroom-1].h / 2][room[nroom-1].x + room[nroom-1].w / 2] = '>';

    for (i = 0; i < MAXMON; i++) mon[i].alive = 0;
    for (i = 0; i < MAXMON && i < nroom * 2; i++) {
        const char *G_ = THEMES[THEME_ID].glyphs;
        int ri = 1 + rnd(nroom - 1);
        mon[i].x = room[ri].x + rnd(room[ri].w);
        mon[i].y = room[ri].y + rnd(room[ri].h);
        mon[i].glyph = G_[rnd(6)];
        mon[i].hp = 4 + depth * 2 + rnd(4);
        mon[i].atk = 1 + depth / 2 + rnd(2);
        mon[i].alive = 1;
    }
    /* Scatter a little gold and the occasional potion. */
    for (i = 0; i < 6 + depth; i++) {
        int ri = rnd(nroom);
        int x = room[ri].x + rnd(room[ri].w), y = room[ri].y + rnd(room[ri].h);
        if (map[y][x] == '.') map[y][x] = (rnd(5) == 0) ? '!' : '$';
    }
}

static int blocked(int x, int y)
{
    if (x < 0 || x >= W || y < 0 || y >= H) return 1;
    return map[y][x] == '#';
}

static int mon_at(int x, int y)
{
    int i;
    for (i = 0; i < MAXMON; i++)
        if (mon[i].alive && mon[i].x == x && mon[i].y == y) return i;
    return -1;
}

static void update_vision(void)
{
    int r, c;
    for (r = py - 4; r <= py + 4; r++)
        for (c = px - 8; c <= px + 8; c++)
            if (r >= 0 && r < H && c >= 0 && c < W) seen[r][c] = 1;
}

static void monsters_act(void)
{
    int i;
    for (i = 0; i < MAXMON; i++) {
        int dx, dy, nx, ny;
        if (!mon[i].alive) continue;
        dx = px - mon[i].x;
        dy = py - mon[i].y;
        if (dx * dx + dy * dy > 64) continue;          /* out of aggro range */
        if (dx == 0 && dy == 0) continue;
        if (abs(dx) <= 1 && abs(dy) <= 1) {
            char m[96];
            hp -= mon[i].atk;
            snprintf(m, sizeof m, "The %c hits you for %d.", mon[i].glyph, mon[i].atk);
            logmsg(m);
            continue;
        }
        nx = mon[i].x + (dx > 0 ? 1 : dx < 0 ? -1 : 0);
        ny = mon[i].y + (dy > 0 ? 1 : dy < 0 ? -1 : 0);
        if (!blocked(nx, mon[i].y) && mon_at(nx, mon[i].y) < 0) mon[i].x = nx;
        else if (!blocked(mon[i].x, ny) && mon_at(mon[i].x, ny) < 0) mon[i].y = ny;
    }
}

static void render(void)
{
    int r, c, i;
    {
        char sub[110];
        snprintf(sub, sizeof sub, "%s - reach depth %d - arrows move, P potion, > descends",
                 THEMES[THEME_ID].name, TARGET_DEPTH);
        draw_title("DUNGEON CRAWL", sub);
    }
    for (r = 0; r < H; r++) {
        scr_move(6 + r, 8);
        for (c = 0; c < W; c++) {
            if (!seen[r][c]) { putchar(' '); continue; }
            {
                int lit = (r >= py - 4 && r <= py + 4 && c >= px - 8 && c <= px + 8);
                const char *dim = lit ? "" : C_GREY;
                char ch = map[r][c];
                if (ch == '#')      printf("%s%s#%s", dim, lit ? C_BLUE : "", C_RESET);
                else if (ch == '>') printf("%s%s>%s", dim, C_MAGENTA, C_RESET);
                else if (ch == '$') printf("%s%s$%s", dim, C_YELLOW, C_RESET);
                else if (ch == '!') printf("%s%s!%s", dim, C_GREEN, C_RESET);
                else                printf("%s%s.%s", dim, C_GREY, C_RESET);
            }
        }
    }
    for (i = 0; i < MAXMON; i++) {
        if (!mon[i].alive) continue;
        if (!(mon[i].y >= py - 4 && mon[i].y <= py + 4 && mon[i].x >= px - 8 && mon[i].x <= px + 8))
            continue;
        draw_textf(6 + mon[i].y, 8 + mon[i].x, "%s%c%s", C_RED, mon[i].glyph, C_RESET);
    }
    draw_textf(6 + py, 8 + px, "%s@%s", C_BOLD C_CYAN, C_RESET);

    draw_textf(H + 7, 8, "HP %s%3d%s/%-3d  Atk %-2d  Gold %-5d  Potions %-2d  Depth %d/%d   ",
               hp < maxhp / 3 ? C_RED : C_GREEN, hp, C_RESET, maxhp, atk, gold, potions,
               depth, TARGET_DEPTH);
    draw_textf(H + 8, 8, "%s%-70s%s", C_GREY, log1, C_RESET);
    draw_textf(H + 9, 8, "%s%-70s%s", C_GREY, msglog2, C_RESET);
    scr_flush();
}

void fam_dungeon(const GParams *p)
{
    int i;
    THEME_ID = -1;
    for (i = 0; i < NTHEMES; i++)
        if (p->theme && strcmp(p->theme, THEMES[i].name) == 0) THEME_ID = i;
    if (THEME_ID < 0)
        THEME_ID = (p->variant >= 0 && p->variant < NTHEMES) ? p->variant : 0;
    TARGET_DEPTH = gp_int(p->level, 10);

    for (;;) {
        int dead = 0, won = 0;
        char opening[96];
        depth = 1; hp = maxhp = 24; atk = 4; gold = 0; potions = 1;
        log1[0] = msglog2[0] = '\0';
        snprintf(opening, sizeof opening, "You enter the %s. Reach depth %d to escape.",
                 THEMES[THEME_ID].name, TARGET_DEPTH);
        logmsg(opening);
        generate();
        update_vision();

        while (!dead && !won) {
            int k, nx = px, ny = py, acted = 0;
            render();
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP)    ny--;
            if (k == KEY_DOWN)  ny++;
            if (k == KEY_LEFT)  nx--;
            if (k == KEY_RIGHT) nx++;
            if (k == 'p' || k == 'P') {
                if (potions > 0) {
                    int heal = 10 + rnd(8);
                    potions--;
                    hp += heal;
                    if (hp > maxhp) hp = maxhp;
                    {
                        char m[64];
                        snprintf(m, sizeof m, "You drink a potion and recover %d HP.", heal);
                        logmsg(m);
                    }
                    acted = 1;
                } else logmsg("You have no potions.");
            }
            if (k == '>' || k == '.') {
                if (map[py][px] == '>') {
                    depth++;
                    maxhp += 4;
                    hp = maxhp;
                    atk += 1;
                    if (depth > TARGET_DEPTH) { won = 1; break; }
                    generate();
                    update_vision();
                    {
                        char m[64];
                        snprintf(m, sizeof m, "You descend to depth %d of %d.",
                                 depth, TARGET_DEPTH);
                        logmsg(m);
                    }
                    continue;
                }
                logmsg("There are no stairs here.");
            }

            if (nx != px || ny != py) {
                int mi = mon_at(nx, ny);
                if (mi >= 0) {
                    int dmg = atk + rnd(3);
                    char m[96];
                    mon[mi].hp -= dmg;
                    if (mon[mi].hp <= 0) {
                        mon[mi].alive = 0;
                        gold += 5 + rnd(10) * depth;
                        snprintf(m, sizeof m, "You kill the %c!", mon[mi].glyph);
                    } else {
                        snprintf(m, sizeof m, "You hit the %c for %d.", mon[mi].glyph, dmg);
                    }
                    logmsg(m);
                    acted = 1;
                } else if (!blocked(nx, ny)) {
                    px = nx; py = ny;
                    acted = 1;
                    if (map[py][px] == '$') {
                        int g = 5 + rnd(20);
                        char m[64];
                        gold += g;
                        map[py][px] = '.';
                        snprintf(m, sizeof m, "You pick up %d gold.", g);
                        logmsg(m);
                    } else if (map[py][px] == '!') {
                        potions++;
                        map[py][px] = '.';
                        logmsg("You find a healing potion.");
                    }
                }
            }
            if (acted) {
                monsters_act();
                /* The environment itself bites when you stand in the open. */
                if (THEMES[THEME_ID].hazard && rnd(100) < 6 + depth) {
                    hp -= 1 + depth / 5;
                    logmsg(THEMES[THEME_ID].hazard_text);
                }
                update_vision();
                if (hp <= 0) dead = 1;
            }
        }
        render();
        {
            char m[96];
            if (won) {
                snprintf(m, sizeof m, "You escaped the %s with %d gold.",
                         THEMES[THEME_ID].name, gold);
                draw_centered(H + 11, 80, C_BOLD C_GREEN "You escape!" C_RESET);
            } else {
                snprintf(m, sizeof m, "You died on depth %d of %d with %d gold.",
                         depth, TARGET_DEPTH, gold);
                draw_centered(H + 11, 80, C_BOLD C_RED "You have died." C_RESET);
            }
            draw_centered(H + 12, 80, m);
            scr_flush();
            score_report(p->title ? p->title : "dungeon",
                         gold + depth * 100 + (won ? TARGET_DEPTH * 200 : 0));
        }
        if (!confirm("\n  Play again?")) return;
    }
}
