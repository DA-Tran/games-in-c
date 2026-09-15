/* GIC:PARAMETERISED sim
 * GIC:PARAMETERISED textadv
 *
 * sim.c - management simulations and text adventures.
 *
 * params: variant selects the game
 *
 * Both families are turn-and-report games: you are shown a state, you choose,
 * the world advances, it tells you what happened. So both are one loop over a
 * description of the world rather than twenty-two and twenty separate loops.
 *
 *   sim      A simulation is a set of named resources plus the rules that move
 *            them. Hammurabi's grain, Lemonade Stand's cups and Wa-Tor's fish
 *            are the same machine with different tables, and the interesting
 *            part - the feedback loop that makes the game a game - lives in
 *            the per-variant step function.
 *   textadv  An adventure is a graph of rooms with things in them. The map is
 *            generated from a seed, so the layout differs every time while the
 *            shape of the game (find the exit, survive, defeat the thing at
 *            the end) stays put.
 */
#include "engine.h"
#include "games.h"

/* ============================================================ SIMULATIONS */

#define NRES 4

typedef struct {
    const char *name;
    const char *res[NRES];       /* what the player is managing      */
    int start[NRES];
    const char *act[4];          /* the four choices each turn       */
    int turns;
    const char *goal;
} Sim;

static const Sim SIM[22] = {
 {"Hammurabi", {"bushels","acres","people","rats"}, {2800, 1000, 100, 0},
  {"buy land","sell land","feed the people","plant crops"}, 20, "keep the city alive for twenty years"},
 {"Lemonade Stand", {"cash","cups","lemons","reputation"}, {50, 0, 0, 50},
  {"buy lemons","make cups","set a high price","set a low price"}, 20, "end the season with the most cash"},
 {"Oregon Trail", {"miles","food","health","oxen"}, {0, 400, 100, 4},
  {"travel on","rest a day","hunt","trade"}, 30, "reach two thousand miles alive"},
 {"Star Trek 1971", {"energy","torpedoes","shields","klingons"}, {3000, 10, 100, 12},
  {"fire phasers","fire a torpedo","raise shields","warp away"}, 25, "clear the quadrant"},
 {"Drug Wars", {"cash","stock","heat","debt"}, {2000, 0, 0, 5500},
  {"buy stock","sell stock","lie low","pay the debt"}, 30, "clear the debt and bank the rest"},
 {"Sim Farm", {"cash","crops","livestock","soil"}, {500, 0, 0, 80},
  {"plant","buy livestock","fertilise","sell at market"}, 25, "keep the soil good and the farm solvent"},
 {"Sim City Lite", {"funds","population","industry","pollution"}, {1000, 100, 0, 0},
  {"build housing","build industry","raise taxes","clean up"}, 30, "grow the city without choking it"},
 {"Railroad Tycoon Lite", {"cash","track","trains","freight"}, {800, 0, 1, 0},
  {"lay track","buy a train","run freight","raise capital"}, 25, "build a network that pays"},
 {"Civilisation Lite", {"food","production","science","population"}, {100, 10, 0, 20},
  {"farm","build","research","expand"}, 30, "reach a hundred science"},
 {"Risk", {"armies","territories","cards","enemies"}, {20, 3, 0, 9},
  {"attack","fortify","recruit","trade cards"}, 25, "take every territory"},
 {"Stock Market Sim", {"cash","shares","price","trend"}, {5000, 0, 50, 0},
  {"buy","sell","hold","short"}, 30, "beat the market"},
 {"Elevator Simulator", {"floor","waiting","served","patience"}, {1, 0, 0, 100},
  {"go up","go down","open the doors","express"}, 30, "serve everyone before patience runs out"},
 {"Traffic Light Sim", {"queued","passed","crashes","cycle"}, {0, 0, 0, 0},
  {"green north","green east","all red","shorten the cycle"}, 30, "pass the most cars without a crash"},
 {"Ant Colony Sim", {"ants","food","tunnels","threat"}, {20, 50, 1, 0},
  {"forage","dig","breed","defend"}, 30, "grow the colony past two hundred"},
 {"Epidemic Sim", {"healthy","infected","recovered","funds"}, {1000, 5, 0, 300},
  {"vaccinate","quarantine","treat","do nothing"}, 25, "stop the outbreak"},
 {"Ecosystem Balance", {"grass","rabbits","foxes","seasons"}, {500, 40, 8, 0},
  {"seed grass","cull rabbits","cull foxes","leave it alone"}, 30, "keep all three alive"},
 {"Power Grid Sim", {"capacity","demand","fuel","outages"}, {100, 80, 200, 0},
  {"build capacity","buy fuel","load shed","upgrade"}, 25, "keep the lights on"},
 {"Airport Control", {"stacked","landed","fuel warnings","runways"}, {0, 0, 0, 2},
  {"land one","hold one","open a runway","divert"}, 30, "land them all"},
 {"Restaurant Tycoon", {"cash","tables","staff","rating"}, {600, 4, 2, 50},
  {"hire","buy tables","advertise","improve the menu"}, 25, "reach a rating of ninety"},
 {"Space Colony", {"air","water","colonists","power"}, {100, 100, 10, 100},
  {"recycle air","drill for water","grow the colony","build power"}, 30, "survive thirty months"},
 {"Wa-Tor", {"fish","sharks","water","chronons"}, {200, 20, 100, 0},
  {"seed fish","seed sharks","widen the sea","just watch"}, 30, "keep both species alive"},
 {"Bridge Builder", {"budget","spans","strength","load"}, {1000, 0, 0, 0},
  {"add a span","reinforce","test the load","cut costs"}, 20, "carry the load within budget"}
};

/* One turn of the world. Each simulation gets its own feedback loop, because
 * the loop is the game: Wa-Tor's predator-prey oscillation and Sim City's
 * pollution spiral are not the same rule with different numbers. */
static void sim_step(int v, int *r, int act, char *log, int ln)
{
    int i;
    switch (v) {
    case 0: {                                   /* Hammurabi */
        int price = 17 + rnd(10), harvest = 1 + rnd(5);
        if (act == 0) { int n = 10; if (r[0] >= price * n) { r[0] -= price * n; r[1] += n; } }
        if (act == 1) { int n = 10; if (r[1] >= n) { r[1] -= n; r[0] += price * n; } }
        if (act == 2) { int need = r[2] * 20; if (r[0] >= need) r[0] -= need; else { r[2] -= (need - r[0]) / 20; r[0] = 0; } }
        if (act == 3) r[0] += r[1] * harvest;
        r[3] = r[0] / 40;
        r[0] -= r[3];
        r[2] += rnd(5) - 1;
        snprintf(log, ln, "land at %d bushels an acre, harvest %d per acre, rats ate %d", price, harvest, r[3]);
        break;
    }
    case 1: {                                   /* Lemonade Stand */
        int weather = rnd(100), sold;
        if (act == 0) { if (r[0] >= 20) { r[0] -= 20; r[2] += 10; } }
        if (act == 1) { int n = r[2] < 10 ? r[2] : 10; r[2] -= n; r[1] += n; }
        sold = r[1] < weather / 8 ? r[1] : weather / 8;
        if (act == 2) { sold = sold / 2; r[0] += sold * 4; r[3] -= 2; }
        else          { r[0] += sold * 2; r[3] += 1; }
        r[1] -= sold;
        snprintf(log, ln, "weather %d, sold %d cups", weather, sold);
        break;
    }
    case 2: {                                   /* Oregon Trail */
        if (act == 0) { r[0] += 40 + r[3] * 10; r[1] -= 30; r[2] -= 4; }
        if (act == 1) { r[2] += 12; r[1] -= 12; }
        if (act == 2) { r[1] += 30 + rnd(60); r[2] -= 2; }
        if (act == 3) { if (r[1] > 50) { r[1] -= 50; r[3]++; } }
        if (r[1] < 0) { r[2] += r[1] / 4; r[1] = 0; }
        if (rnd(100) < 18) { r[2] -= 10; snprintf(log, ln, "dysentery in the party"); }
        else snprintf(log, ln, "%d miles covered so far", r[0]);
        break;
    }
    case 3: {                                   /* Star Trek */
        if (act == 0) { r[0] -= 300; r[3] -= 1 + rnd(2); }
        if (act == 1) { if (r[1] > 0) { r[1]--; r[3] -= 2; } }
        if (act == 2) { r[0] -= 200; r[2] += 30; }
        if (act == 3) { r[0] -= 500; r[3] = r[3] > 2 ? r[3] - 2 : 0; }
        if (r[3] > 0) { int hit = 40 + rnd(60); if (r[2] > hit) r[2] -= hit; else { r[0] -= hit - r[2]; r[2] = 0; } }
        if (r[3] < 0) r[3] = 0;
        snprintf(log, ln, "%d klingons left, shields at %d", r[3], r[2]);
        break;
    }
    case 4: {                                   /* Drug Wars */
        int price = 20 + rnd(80);
        if (act == 0) { int n = r[0] / price; r[0] -= n * price; r[1] += n; r[2] += 5; }
        if (act == 1) { r[0] += r[1] * price; r[1] = 0; r[2] += 10; }
        if (act == 2) { r[2] -= 20; if (r[2] < 0) r[2] = 0; }
        if (act == 3) { int pay = r[0] / 2; r[0] -= pay; r[3] -= pay; }
        r[3] += r[3] / 10;
        if (r[2] > 60 && rnd(100) < r[2] / 2) { r[0] /= 2; snprintf(log, ln, "busted - half the cash gone"); }
        else snprintf(log, ln, "the going price was %d, heat %d", price, r[2]);
        break;
    }
    case 20: {                                  /* Wa-Tor */
        /* The classic predator-prey oscillation, in integers. */
        int births = r[0] / 4, eaten = r[1] * 3;
        if (act == 0) r[0] += 50;
        if (act == 1) r[1] += 5;
        if (act == 2) r[2] += 20;
        if (eaten > r[0]) eaten = r[0];
        r[0] += births - eaten;
        r[1] += eaten / 8 - r[1] / 5;
        if (r[0] > r[2] * 6) r[0] = r[2] * 6;     /* the sea only holds so much */
        if (r[0] < 0) r[0] = 0;
        if (r[1] < 0) r[1] = 0;
        snprintf(log, ln, "%d fish born, %d eaten", births, eaten);
        break;
    }
    case 15: {                                  /* Ecosystem Balance */
        int grazed = r[1] * 4, hunted = r[2] * 3;
        if (act == 0) r[0] += 200;
        if (act == 1) r[1] -= r[1] / 4;
        if (act == 2) r[2] -= r[2] / 4;
        if (grazed > r[0]) grazed = r[0];
        if (hunted > r[1]) hunted = r[1];
        r[0] += 80 - grazed;
        r[1] += grazed / 5 - hunted;
        r[2] += hunted / 4 - r[2] / 6;
        for (i = 0; i < 3; i++) if (r[i] < 0) r[i] = 0;
        r[3]++;
        snprintf(log, ln, "%d grass grazed, %d rabbits taken", grazed, hunted);
        break;
    }
    case 14: {                                  /* Epidemic */
        int newcases = r[1] * (r[0] > 0 ? 2 : 0) / 3;
        if (act == 0) { if (r[3] >= 100) { r[3] -= 100; r[0] -= 100; r[2] += 100; } }
        if (act == 1) { r[3] -= 50; newcases /= 3; }
        if (act == 2) { r[3] -= 60; r[2] += r[1] / 2; r[1] -= r[1] / 2; }
        if (newcases > r[0]) newcases = r[0];
        r[0] -= newcases;
        r[1] += newcases - r[1] / 4;
        r[2] += r[1] / 4;
        r[3] += 40;
        for (i = 0; i < 4; i++) if (r[i] < 0) r[i] = 0;
        snprintf(log, ln, "%d new cases this week", newcases);
        break;
    }
    case 6: {                                   /* Sim City Lite */
        if (act == 0) { if (r[0] >= 200) { r[0] -= 200; r[1] += 120; } }
        if (act == 1) { if (r[0] >= 300) { r[0] -= 300; r[2] += 40; r[3] += 15; } }
        if (act == 2) { r[0] += r[1] / 2; r[1] -= r[1] / 20; }
        if (act == 3) { if (r[0] >= 150) { r[0] -= 150; r[3] -= 30; } }
        r[0] += r[2] * 3;
        r[3] += r[2] / 8;
        if (r[3] > 60) r[1] -= r[3] / 2;          /* people leave a filthy city */
        if (r[3] < 0) r[3] = 0;
        if (r[1] < 0) r[1] = 0;
        snprintf(log, ln, "industry paid %d, pollution %d", r[2] * 3, r[3]);
        break;
    }
    default: {
        /* The remaining simulations share a general shape: the first action
         * spends, the second converts, the third trades short-term cost for
         * long-term gain, and the fourth waits. */
        int swing = rnd(20) - 8;
        if (act == 0) { if (r[0] >= 50) { r[0] -= 50; r[1] += 10; } }
        if (act == 1) { if (r[1] >= 5)  { r[1] -= 5;  r[2] += 8; } }
        if (act == 2) { r[3] += 10; r[0] -= 20; }
        if (act == 3) { r[0] += r[2] * 2; }
        r[0] += r[1] + swing;
        r[2] += r[3] / 20;
        for (i = 0; i < 4; i++) if (r[i] < 0) r[i] = 0;
        snprintf(log, ln, "conditions moved by %+d", swing);
        break;
    }
    }
    for (i = 0; i < NRES; i++) if (r[i] < 0) r[i] = 0;
}

void fam_sim(const GParams *p)
{
    int v = gp_int(p->variant, 0), res[NRES], turn, cur = 0, i;
    char log[120] = "";
    const Sim *S;

    if (v < 0 || v > 21) v = 0;
    S = &SIM[v];

    for (;;) {
        for (i = 0; i < NRES; i++) res[i] = S->start[i];
        turn = 0;
        log[0] = '\0';
        cur = 0;

        while (turn < S->turns) {
            int k;
            char sub[160];
            snprintf(sub, sizeof sub, "%s — turn %d of %d. Arrows choose, Enter commits, Q quits",
                     S->name, turn + 1, S->turns);
            draw_title("SIMULATION", sub);
            draw_textf(4, 6, "%sGoal: %s%s", C_GREY, S->goal, C_RESET);
            for (i = 0; i < NRES; i++)
                draw_textf(6 + i, 8, "%s%-14s %s%6d%s", C_WHITE, S->res[i], C_YELLOW, res[i], C_RESET);
            draw_textf(11, 6, "%sthis turn:%s", C_GREY, C_RESET);
            for (i = 0; i < 4; i++)
                draw_textf(12 + i, 8, "%s%s%-26s%s", i == cur ? BG_BLUE : "", C_WHITE, S->act[i], C_RESET);
            draw_textf(17, 6, "%s%-70s%s", C_YELLOW, log, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "sim", res[0]); return; }
            if (k == KEY_UP)   { cur = (cur + 3) % 4; continue; }
            if (k == KEY_DOWN) { cur = (cur + 1) % 4; continue; }
            if (k != KEY_ENTER && k != ' ') continue;

            sim_step(v, res, cur, log, sizeof log);
            turn++;

            /* Most of these end early if the thing you are managing dies. */
            if (res[0] == 0 && res[1] == 0 && res[2] == 0) break;
        }
        scr_clear();
        draw_textf(9, 8, "%sAfter %d turns:%s", C_BOLD, turn, C_RESET);
        for (i = 0; i < NRES; i++)
            draw_textf(11 + i, 8, "%s%-14s %6d%s", C_WHITE, S->res[i], res[i], C_RESET);
        scr_flush();
        score_report(p->title ? p->title : "sim", res[0] + res[1] + res[2]);
        if (!confirm("\n\n  Run it again?")) return;
    }
}

/* ========================================================= TEXT ADVENTURE */

#define NROOM 12

typedef struct {
    const char *name;
    const char *place[6];      /* room-name fragments for this setting */
    const char *thing[5];      /* what you can pick up                 */
    const char *foe;
    int foehp, playerhp;
    const char *win;
} Adv;

static const Adv ADV[20] = {
 {"Text Adventure",      {"hall","cellar","study","garden","attic","gate"},
  {"lamp","key","rope","coin","map"}, "shadow", 20, 30, "You step out through the gate."},
 {"Colossal Cave Lite",  {"grate","crawl","hall of mists","dome","canyon","debris"},
  {"lamp","keys","cage","rod","bird"}, "dwarf", 18, 30, "You surface with the treasure."},
 {"Zork Lite",           {"west of house","kitchen","living room","cellar","maze","altar"},
  {"lantern","sword","sack","egg","torch"}, "grue", 24, 30, "The trophy case is full."},
 {"Escape the Room",     {"office","closet","corridor","vault","lobby","stairwell"},
  {"keycard","note","battery","screwdriver","code"}, "alarm", 14, 25, "The door clicks open."},
 {"Quest for the Grail", {"chapel","bridge","forest","castle","ford","cave"},
  {"shield","relic","horn","banner","chalice"}, "black knight", 26, 32, "The grail is yours."},
 {"Choose Your Path",    {"crossroads","village","river","ridge","market","shrine"},
  {"charm","letter","flask","token","cloak"}, "stranger", 16, 28, "You chose well."},
 {"Vampire Castle",      {"crypt","chapel","tower","library","cellar","courtyard"},
  {"stake","garlic","mirror","cross","flask"}, "count", 30, 30, "Dawn breaks and the count is dust."},
 {"Zombie Survival",     {"mall","pharmacy","rooftop","car park","clinic","tunnel"},
  {"bat","medkit","fuel","radio","ammo"}, "horde", 34, 30, "The helicopter lifts off."},
 {"Space Trader",        {"dock","hold","bridge","market","engine bay","airlock"},
  {"fuel cell","manifest","scanner","credits","sidearm"}, "pirate", 22, 30, "You dock rich and whole."},
 {"Monster Arena",       {"pit","stands","tunnel","gate","armoury","ring"},
  {"blade","tonic","charm","shield","net"}, "champion", 36, 34, "The crowd is on its feet."},
 {"Wizard Duel",         {"circle","tower","vault","observatory","cloister","stair"},
  {"wand","grimoire","rune","phial","focus"}, "rival", 28, 28, "Their last shield falls."},
 {"Gladiator Manager",   {"barracks","sands","gate","infirmary","stable","box"},
  {"trident","net","balm","helm","contract"}, "champion", 30, 32, "Your stable takes the laurel."},
 {"Merchant Sim",        {"wharf","warehouse","road","bazaar","counting house","inn"},
  {"ledger","silk","spice","letter of credit","guard"}, "brigand", 20, 30, "The caravan gets through."},
 {"Tower Climb RPG",     {"landing","stair","gallery","cell","spire","vault"},
  {"sword","potion","ring","scroll","lantern"}, "warden", 32, 32, "You reach the spire."},
 {"Pet Monster Battler", {"meadow","lab","gym","cave","route","centre"},
  {"ball","potion","stone","badge","berry"}, "champion", 26, 30, "The badge is yours."},
 {"Dungeon of Doom",     {"entrance","crypt","armoury","well","shrine","deep"},
  {"sword","shield","potion","amulet","torch"}, "lich", 38, 34, "The amulet is recovered."},
 {"Rogue",               {"room","corridor","vault","stair","larder","shrine"},
  {"mace","ration","scroll","ring","wand"}, "hobgoblin", 24, 30, "You climb out alive."},
 {"NetHack Lite",        {"level","sokoban","mines","temple","shop","oracle"},
  {"pick-axe","ration","wand","amulet","scroll"}, "mind flayer", 30, 30, "You ascend."},
 {"Angband Lite",        {"town","level","pit","vault","stair","cave"},
  {"lantern","potion","scroll","ring","blade"}, "balrog", 40, 36, "Morgoth's servant falls."},
 {"Hunt the Wumpus",     {"cave","tunnel","pit room","bat roost","hollow","den"},
  {"arrow","arrow","arrow","arrow","rope"}, "wumpus", 12, 20, "The wumpus is slain."}
};

void fam_textadv(const GParams *p)
{
    int v = gp_int(p->variant, 0);
    const Adv *A;
    int exits[NROOM][4], item[NROOM], here, hp, foehp, carried[5], ncarry;
    int i, j, moves, foeroom, exitroom;
    char log[140];

    if (v < 0 || v > 19) v = 0;
    A = &ADV[v];

    for (;;) {
        int over = 0, won = 0;
        /* Build a fresh map: a ring so everywhere is reachable, plus some
         * shortcuts so it does not read as a corridor. */
        for (i = 0; i < NROOM; i++) {
            exits[i][0] = (i + 1) % NROOM;             /* north */
            exits[i][1] = (i + NROOM - 1) % NROOM;     /* south */
            exits[i][2] = rnd(100) < 40 ? rnd(NROOM) : -1;
            exits[i][3] = rnd(100) < 40 ? rnd(NROOM) : -1;
            item[i] = -1;
        }
        for (i = 0; i < 5; i++) item[1 + rnd(NROOM - 1)] = i;
        here = 0; hp = A->playerhp; foehp = A->foehp; ncarry = 0; moves = 0;
        for (i = 0; i < 5; i++) carried[i] = 0;
        foeroom = NROOM / 2 + rnd(NROOM / 2);
        exitroom = NROOM - 1;
        snprintf(log, sizeof log, "You are in the %s.", A->place[0]);

        while (!over) {
            int k;
            char sub[160];
            snprintf(sub, sizeof sub,
                     "%s — arrows go north/south/east/west, T takes, F fights, Q quits", A->name);
            draw_title("ADVENTURE", sub);
            draw_textf(4, 6, "%sYou are in the %s (%d).%s", C_WHITE,
                       A->place[here % 6], here, C_RESET);
            draw_textf(6, 6, "%sExits:%s", C_GREY, C_RESET);
            {
                static const char *DIRN[4] = {"north", "south", "east", "west"};
                int col = 14;
                for (i = 0; i < 4; i++)
                    if (exits[here][i] >= 0) {
                        draw_textf(6, col, "%s%s%s", C_CYAN, DIRN[i], C_RESET);
                        col += 8;
                    }
            }
            if (item[here] >= 0)
                draw_textf(8, 6, "%sYou can see a %s here.%s", C_YELLOW, A->thing[item[here]], C_RESET);
            if (here == foeroom && foehp > 0)
                draw_textf(9, 6, "%sA %s blocks the way (%d).%s", C_RED, A->foe, foehp, C_RESET);
            if (here == exitroom && foehp <= 0)
                draw_textf(9, 6, "%sThe way out is here.%s", C_GREEN, C_RESET);

            draw_textf(12, 6, "%shealth %-3d   carrying %d of 5   moves %d%s",
                       C_WHITE, hp, ncarry, moves, C_RESET);
            draw_textf(13, 6, "%s", C_GREY);
            for (i = 0, j = 0; i < 5; i++)
                if (carried[i]) { draw_textf(13, 6 + j * 14, "%s%-13s%s", C_GREY, A->thing[i], C_RESET); j++; }
            draw_textf(15, 6, "%s%-72s%s", C_YELLOW, log, C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "textadv", ncarry * 50); return; }
            {
                int dir = k == KEY_UP ? 0 : k == KEY_DOWN ? 1 : k == KEY_RIGHT ? 2 : k == KEY_LEFT ? 3 : -1;
                if (dir >= 0) {
                    if (exits[here][dir] < 0) { snprintf(log, sizeof log, "You cannot go that way."); continue; }
                    if (here == foeroom && foehp > 0) {
                        snprintf(log, sizeof log, "The %s will not let you past.", A->foe);
                        continue;
                    }
                    here = exits[here][dir];
                    moves++;
                    snprintf(log, sizeof log, "You go %s.",
                             dir == 0 ? "north" : dir == 1 ? "south" : dir == 2 ? "east" : "west");
                    continue;
                }
            }
            if (k == 't' || k == 'T') {
                if (item[here] < 0) { snprintf(log, sizeof log, "There is nothing to take."); continue; }
                carried[item[here]] = 1;
                ncarry++;
                snprintf(log, sizeof log, "Taken: the %s.", A->thing[item[here]]);
                item[here] = -1;
                continue;
            }
            if (k == 'f' || k == 'F') {
                if (here != foeroom || foehp <= 0) { snprintf(log, sizeof log, "There is nothing to fight."); continue; }
                {
                    /* What you carry is the whole difference between winning
                     * and losing this fight. */
                    int mine = 3 + ncarry * 2 + rnd(4);
                    int theirs = 2 + rnd(5);
                    foehp -= mine;
                    hp -= theirs;
                    if (foehp <= 0) snprintf(log, sizeof log, "You strike for %d - the %s falls.", mine, A->foe);
                    else snprintf(log, sizeof log, "You hit for %d, it hits back for %d.", mine, theirs);
                }
                if (hp <= 0) { over = 1; won = 0; }
                continue;
            }
            if (here == exitroom && foehp <= 0) { over = 1; won = 1; }
        }
        scr_clear();
        draw_centered(11, 80, won ? A->win : "You did not make it out.");
        draw_textf(13, 20, "%s%d rooms walked, %d things carried%s", C_GREY, moves, ncarry, C_RESET);
        scr_flush();
        score_report(p->title ? p->title : "textadv", won ? 500 + ncarry * 50 : ncarry * 20);
        if (!confirm("\n\n  Another go?")) return;
    }
}
