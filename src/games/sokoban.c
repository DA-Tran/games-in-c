/* GIC:PARAMETERISED sokoban
 * sokoban.c - crate pushing across six built-in levels, with undo.
 * params: level = which level to start from (0..5)
 */
#include "engine.h"
#include "games.h"

#define MAXW 24
#define MAXH 14
#define UNDO 256

/* # wall, @ player, $ crate, . goal, * crate on goal, + player on goal */
static const char *LEVELS[][MAXH] = {
{ "    #####          ",
  "    #   #          ",
  "    #$  #          ",
  "  ###  $##         ",
  "  #  $ $ #         ",
  "### # ## #   ######",
  "#   # ## #####  ..#",
  "# $  $          ..#",
  "##### ### #@##  ..#",
  "    #     #########",
  "    #######        ", NULL },

{ "############  ",
  "#..  #     ###",
  "#..  # $  $  #",
  "#..  #$####  #",
  "#..    @ ##  #",
  "#..  # #  $ ##",
  "###### ##$ $ #",
  "  # $  $ $ $ #",
  "  #    #     #",
  "  ############", NULL },

{ "        ######## ",
  "        #     @# ",
  "        # $#$ ## ",
  "        # $  $#  ",
  "        ##$ $ #  ",
  "######### $ # ###",
  "#....  ## $  $  #",
  "##...    $  $   #",
  "#....  ##########",
  "########         ", NULL },

{ "  ####       ",
  "  #  ###     ",
  "  #    #     ",
  "  # $$ #     ",
  "### ## ##### ",
  "#  $ $     # ",
  "# @ $ $$$  # ",
  "#### ..... # ",
  "   # ..... # ",
  "   #########  ", NULL },

{ "#####    ",
  "#   #####",
  "# $ #   #",
  "# $$# $ #",
  "#  .. $ #",
  "##..@####",
  " #..$#   ",
  " #####   ", NULL },

{ "####################",
  "#..    #           #",
  "#..    # $  $      #",
  "#..    #$###    ## #",
  "#..      @ ##   $  #",
  "#..    # #  $ $ #  #",
  "#..    # #  $   #  #",
  "#....### ###$#### ##",
  "#      #        #  #",
  "####################", NULL },
};
#define NLEVELS ((int)(sizeof(LEVELS) / sizeof(LEVELS[0])))

static char map[MAXH][MAXW];      /* walls and goals only */
static char box[MAXH][MAXW];      /* crates               */
static int  pr, pc, rows, cols;
static char undo_box[UNDO][MAXH][MAXW];
static int  undo_pr[UNDO], undo_pc[UNDO], undo_n;

static void load(int lv)
{
    int r, c;
    rows = cols = 0;
    memset(map, ' ', sizeof map);
    memset(box, ' ', sizeof box);
    for (r = 0; r < MAXH && LEVELS[lv][r]; r++) {
        int len = (int)strlen(LEVELS[lv][r]);
        if (len > cols) cols = len;
        for (c = 0; c < len && c < MAXW; c++) {
            char ch = LEVELS[lv][r][c];
            switch (ch) {
                case '#': map[r][c] = '#'; break;
                case '.': map[r][c] = '.'; break;
                case '$': box[r][c] = '$'; break;
                case '*': map[r][c] = '.'; box[r][c] = '$'; break;
                case '@': pr = r; pc = c; break;
                case '+': map[r][c] = '.'; pr = r; pc = c; break;
                default: break;
            }
        }
        rows = r + 1;
    }
    undo_n = 0;
}

static int solved(void)
{
    int r, c;
    for (r = 0; r < rows; r++) for (c = 0; c < cols; c++)
        if (box[r][c] == '$' && map[r][c] != '.') return 0;
    return 1;
}

static void save_undo(void)
{
    if (undo_n >= UNDO) return;
    memcpy(undo_box[undo_n], box, sizeof box);
    undo_pr[undo_n] = pr;
    undo_pc[undo_n] = pc;
    undo_n++;
}

static void step(int dr, int dc)
{
    int nr = pr + dr, nc = pc + dc;
    if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) return;
    if (map[nr][nc] == '#') return;
    if (box[nr][nc] == '$') {
        int br = nr + dr, bc = nc + dc;
        if (br < 0 || br >= rows || bc < 0 || bc >= cols) return;
        if (map[br][bc] == '#' || box[br][bc] == '$') return;
        save_undo();
        box[nr][nc] = ' ';
        box[br][bc] = '$';
    } else {
        save_undo();
    }
    pr = nr; pc = nc;
}

static void render(int lv, int moves, const char *msg)
{
    int r, c;
    char sub[96];
    snprintf(sub, sizeof sub, "Level %d of %d - arrows push, U undo, R restart, Q quits",
             lv + 1, NLEVELS);
    draw_title("SOKOBAN", sub);
    for (r = 0; r < rows; r++) {
        scr_move(6 + r, 28);
        for (c = 0; c < cols; c++) {
            if (r == pr && c == pc)        printf("%s@%s", C_CYAN, C_RESET);
            else if (box[r][c] == '$')     printf("%s%s%s", map[r][c] == '.' ? C_GREEN : C_YELLOW,
                                                  map[r][c] == '.' ? "*" : "$", C_RESET);
            else if (map[r][c] == '#')     printf("%s#%s", C_GREY, C_RESET);
            else if (map[r][c] == '.')     printf("%s.%s", C_GREEN, C_RESET);
            else                           putchar(' ');
        }
    }
    draw_textf(6 + rows + 1, 28, "Moves: %d    ", moves);
    draw_text(6 + rows + 3, 24, "                                                  ");
    if (msg) draw_textf(6 + rows + 3, 28, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_sokoban(const GParams *p)
{
    int lv = (p->level >= 0 && p->level < NLEVELS) ? p->level : 0;
    for (;;) {
        int moves = 0;
        load(lv);
        while (!solved()) {
            int k;
            render(lv, moves, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == 'r' || k == 'R') { load(lv); moves = 0; continue; }
            if (k == 'u' || k == 'U') {
                if (undo_n > 0) {
                    undo_n--;
                    memcpy(box, undo_box[undo_n], sizeof box);
                    pr = undo_pr[undo_n];
                    pc = undo_pc[undo_n];
                    if (moves) moves--;
                }
                continue;
            }
            if (k == KEY_UP)    { step(-1, 0); moves++; }
            if (k == KEY_DOWN)  { step(1, 0);  moves++; }
            if (k == KEY_LEFT)  { step(0, -1); moves++; }
            if (k == KEY_RIGHT) { step(0, 1);  moves++; }
        }
        render(lv, moves, "Level complete!");
        score_report("sokoban", (lv + 1) * 200 - moves);
        sleep_ms(900);
        lv++;
        if (lv >= NLEVELS) {
            render(NLEVELS - 1, moves, "All levels cleared - well done.");
            pause_msg("Press any key...");
            return;
        }
    }
}
