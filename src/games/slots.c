/* GIC:PARAMETERISED slots
 * slots.c - three weighted reels; the theme picks the symbol set.
 * params: theme = symbol set name, or variant = its index
 * Weights and payouts stay constant across themes, so the published
 * return-to-player is the same whichever set you play.
 */
#include "engine.h"
#include "games.h"

#define SYMS 6
#define NTHEMES 11

/* Plain ASCII glyphs keep the reels aligned in every terminal. */
static const char *THEME_NAME[NTHEMES] = {
    "Fruit", "Egypt", "Space", "Pirate", "Jungle", "Diamond",
    "Western", "Aztec", "Neon", "Deep Sea", "Classic"};
static const char *THEME_SYM[NTHEMES][SYMS] = {
    {"Ch","Lm","Bl","St","Dm","77"},
    {"An","Sc","Ey","Ra","Ok","Ph"},
    {"Cm","Mn","St","Ro","Al","Bh"},
    {"Sk","Mp","Rm","Cn","Pr","Cx"},
    {"Ln","Mk","Sn","Pr","Tg","Id"},
    {"Cl","Sp","Em","Rb","Dm","Cr"},
    {"Ht","Bt","Hs","Cl","Sh","Go"},
    {"Sn","Jg","Ma","Te","Gd","Cd"},
    {"Ar","Cr","Sq","Tr","Cb","Nx"},
    {"Fs","Cr","Sh","Oc","Pl","Tr"},
    {"Ch","Lm","Bl","St","Dm","77"}};

static const char **SYM;
static const int   WEIGHT[SYMS] = {30, 25, 20, 13, 8, 4};
static const int   PAY3[SYMS]   = {10, 20, 40, 80, 250, 1000};
static const int   PAY2[SYMS]   = {1, 2, 3, 5, 12, 40};

static int spin_reel(void)
{
    int total = 0, i, r;
    for (i = 0; i < SYMS; i++) total += WEIGHT[i];
    r = rnd(total);
    for (i = 0; i < SYMS; i++) {
        if (r < WEIGHT[i]) return i;
        r -= WEIGHT[i];
    }
    return SYMS - 1;
}

static const char *THEME_LABEL;

static void render(int a, int b, int c, int credits, int bet, const char *msg)
{
    int i;
    char sub[90];
    snprintf(sub, sizeof sub, "%s reels - Enter spins, Up/Down changes bet, Q quits",
             THEME_LABEL);
    draw_title("SLOT MACHINE", sub);
    draw_box(7, 30, 5, 20, C_YELLOW);
    draw_textf(9, 34, " %s   %s   %s ", SYM[a], SYM[b], SYM[c]);
    draw_textf(13, 30, "Credits: %-6d  Bet: %-4d", credits, bet);
    draw_textf(15, 54, "%sPaytable (x bet)%s", C_GREY, C_RESET);
    for (i = 0; i < SYMS; i++)
        draw_textf(16 + i, 54, "%s %s x3 = %-5d  x2 = %d%s",
                   SYM[i], "", PAY3[i], PAY2[i], C_RESET);
    draw_text(15, 26, "                                          ");
    if (msg) draw_textf(15, 30, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_slots(const GParams *p)
{
    int theme = -1, i;
    int credits = 100, bet = 5;

    for (i = 0; i < NTHEMES; i++)
        if (p->theme && strcmp(p->theme, THEME_NAME[i]) == 0) theme = i;
    if (theme < 0) theme = (p->variant >= 0 && p->variant < NTHEMES) ? p->variant : 0;
    SYM = THEME_SYM[theme];
    THEME_LABEL = THEME_NAME[theme];
    int a = 0, b = 0, c = 0;

    while (credits > 0) {
        int k;
        render(a, b, c, credits, bet, NULL);
        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        if (k == KEY_UP   && bet + 5 <= credits && bet < 50) bet += 5;
        if (k == KEY_DOWN && bet > 5)                        bet -= 5;
        if (k != KEY_ENTER && k != ' ') continue;
        if (bet > credits) bet = credits;

        credits -= bet;
        {
            int frame;
            /* Ten frames with a widening gap reads as reels slowing to a
             * stop; much longer than this and the machine feels sluggish. */
            for (frame = 0; frame < 10; frame++) {
                a = spin_reel(); b = spin_reel(); c = spin_reel();
                render(a, b, c, credits, bet, "Spinning...");
                sleep_ms(35 + frame * 9);
            }
        }
        {
            int win = 0;
            char m[80];
            if (a == b && b == c)      win = PAY3[a] * bet / 5;
            else if (a == b)           win = PAY2[a] * bet / 5;
            else if (b == c)           win = PAY2[b] * bet / 5;
            else if (a == c)           win = PAY2[a] * bet / 5;
            credits += win;
            if (win) snprintf(m, sizeof m, "You win %d credits!", win);
            else     snprintf(m, sizeof m, "No win.");
            render(a, b, c, credits, bet, m);
            score_report(p->title ? p->title : "slots", credits);
            sleep_ms(450);
        }
    }
    scr_clear();
    draw_centered(12, 80, credits > 0 ? C_BOLD "Cashed out." C_RESET
                                      : C_BOLD C_RED "Out of credits." C_RESET);
    pause_msg("Press any key...");
}
