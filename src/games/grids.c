/* GIC:PARAMETERISED wordsearch
 * GIC:PARAMETERISED matchthree
 * GIC:PARAMETERISED pegsolitaire
 *
 * grids.c - three grid puzzles that share nothing but a rectangle.
 *
 * Markers above name the families in this file that honour their parameters.
 */
#include "engine.h"
#include "games.h"
#include "words.h"

/* =========================================================== word search
 * params: theme (or variant) selects the dictionary.
 *
 * The grid is filled from the chosen themed dictionary, so Countries and
 * Computing really are different puzzles rather than the same grid relabelled.
 */
#define WS 14
#define WS_WORDS 8

static char ws_grid[WS][WS];
static char ws_word[WS_WORDS][16];
static int  ws_r[WS_WORDS], ws_c[WS_WORDS], ws_dr[WS_WORDS], ws_dc[WS_WORDS];
static int  ws_len[WS_WORDS], ws_found[WS_WORDS], ws_n;

static int ws_fits(const char *w, int len, int r, int c, int dr, int dc)
{
    int i;
    for (i = 0; i < len; i++) {
        int rr = r + dr * i, cc = c + dc * i;
        if (rr < 0 || rr >= WS || cc < 0 || cc >= WS) return 0;
        if (ws_grid[rr][cc] && ws_grid[rr][cc] != w[i]) return 0;
    }
    return 1;
}

static void ws_build(int theme)
{
    static const int DR[8] = {0,0,1,-1,1,1,-1,-1};
    static const int DC[8] = {1,-1,0,0,1,-1,1,-1};
    int r, c, tries;

    memset(ws_grid, 0, sizeof ws_grid);
    ws_n = 0;
    for (tries = 0; tries < 600 && ws_n < WS_WORDS; tries++) {
        const char *w = theme_pick(theme);
        int len = (int)strlen(w), d, i, dup = 0;
        if (len < 4 || len > WS) continue;
        for (i = 0; i < ws_n; i++) if (strcmp(ws_word[i], w) == 0) dup = 1;
        if (dup) continue;
        for (i = 0; i < 40; i++) {
            d = rnd(8);
            r = rnd(WS); c = rnd(WS);
            if (!ws_fits(w, len, r, c, DR[d], DC[d])) continue;
            {
                int k;
                for (k = 0; k < len; k++)
                    ws_grid[r + DR[d]*k][c + DC[d]*k] = (char)toupper((unsigned char)w[k]);
            }
            snprintf(ws_word[ws_n], sizeof ws_word[0], "%s", w);
            ws_r[ws_n] = r; ws_c[ws_n] = c;
            ws_dr[ws_n] = DR[d]; ws_dc[ws_n] = DC[d];
            ws_len[ws_n] = len; ws_found[ws_n] = 0;
            ws_n++;
            break;
        }
    }
    for (r = 0; r < WS; r++) for (c = 0; c < WS; c++)
        if (!ws_grid[r][c]) ws_grid[r][c] = (char)('A' + rnd(26));
}

void fam_wordsearch(const GParams *p)
{
    int theme = theme_index(gp_str(p->theme, "Animals"));
    int cr = 0, cc = 0, sr = -1, sc = -1, found = 0;
    if (theme < 0) theme = gp_int(p->variant, 0) % THEME_COUNT;

    ws_build(theme);

    for (;;) {
        int r, c, i, k;
        char sub[96];
        snprintf(sub, sizeof sub, "%s — arrows move, Enter marks each end of a word, Q quits",
                 THEME_NAME[theme]);
        draw_title("WORD SEARCH", sub);
        for (r = 0; r < WS; r++) {
            scr_move(4 + r, 12);
            for (c = 0; c < WS; c++) {
                int hit = 0;
                for (i = 0; i < ws_n && !hit; i++) {
                    int k2;
                    if (!ws_found[i]) continue;
                    for (k2 = 0; k2 < ws_len[i]; k2++)
                        if (ws_r[i] + ws_dr[i]*k2 == r && ws_c[i] + ws_dc[i]*k2 == c) { hit = 1; break; }
                }
                printf("%s%s%s%c %s",
                       (r == cr && c == cc) ? BG_BLUE : (r == sr && c == sc) ? BG_GREEN : "",
                       hit ? C_GREEN : C_WHITE, hit ? C_BOLD : "",
                       ws_grid[r][c], C_RESET);
            }
        }
        for (i = 0; i < ws_n; i++)
            draw_textf(4 + i, 48, "%s%-14s%s", ws_found[i] ? C_GREEN : C_GREY,
                       ws_found[i] ? ws_word[i] : ws_word[i], C_RESET);
        draw_textf(4 + WS + 1, 12, "Found %d of %d    ", found, ws_n);
        scr_flush();

        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
        if (k == KEY_UP    && cr > 0)      cr--;
        if (k == KEY_DOWN  && cr < WS - 1) cr++;
        if (k == KEY_LEFT  && cc > 0)      cc--;
        if (k == KEY_RIGHT && cc < WS - 1) cc++;
        if (k != KEY_ENTER && k != ' ') continue;

        if (sr < 0) { sr = cr; sc = cc; continue; }
        /* Second pick: does the span match a hidden word in either order? */
        for (i = 0; i < ws_n; i++) {
            int er = ws_r[i] + ws_dr[i] * (ws_len[i] - 1);
            int ec = ws_c[i] + ws_dc[i] * (ws_len[i] - 1);
            if (ws_found[i]) continue;
            if ((sr == ws_r[i] && sc == ws_c[i] && cr == er && cc == ec) ||
                (cr == ws_r[i] && cc == ws_c[i] && sr == er && sc == ec)) {
                ws_found[i] = 1;
                found++;
                break;
            }
        }
        sr = sc = -1;
        if (found >= ws_n) {
            draw_centered(4 + WS + 3, 80, C_BOLD C_GREEN "All words found!" C_RESET);
            scr_flush();
            score_report(p->title ? p->title : "wordsearch", found * 100);
            if (!confirm("\n  Another grid?")) return;
            ws_build(theme);
            found = 0;
        }
    }
}

/* ============================================================ match three
 * params: theme (or variant) selects the symbol set.
 */
#define M3 8

void fam_matchthree(const GParams *p)
{
    static const char *TNAME[6] = {"Gems","Fruit","Runes","Candy","Stars","Blocks"};
    static const char *TSYM[6][6] = {
        {"@","#","$","%","&","*"}, {"a","b","c","d","e","f"},
        {"R","U","N","E","S","X"}, {"o","O","0","Q","q","8"},
        {"*","+","x",".","^","~"}, {"A","B","C","D","E","F"}};
    static const char *COL[6] = {C_RED, C_GREEN, C_YELLOW, C_CYAN, C_MAGENTA, C_WHITE};

    int ti = -1, i, cr = 0, cc = 0, sr = -1, sc = -1;
    int grid[M3][M3], score = 0, moves = 30;

    for (i = 0; i < 6; i++) if (p->theme && strcmp(p->theme, TNAME[i]) == 0) ti = i;
    if (ti < 0) ti = gp_int(p->variant, 0) % 6;

    /* The main loop collapses every run of three or more, refills from the
     * top, and repeats until the board is stable. */
    for (i = 0; i < M3; i++) { int j; for (j = 0; j < M3; j++) grid[i][j] = rnd(6); }

    for (;;) {
        int r, c, k, cleared;

        do {
            cleared = 0;
            for (r = 0; r < M3; r++) for (c = 0; c < M3; c++) {
                int n = 1;
                while (c + n < M3 && grid[r][c+n] == grid[r][c]) n++;
                if (n >= 3) { int j; for (j = 0; j < n; j++) grid[r][c+j] = -1; cleared += n; }
            }
            for (c = 0; c < M3; c++) for (r = 0; r < M3; r++) {
                int n = 1;
                while (r + n < M3 && grid[r+n][c] == grid[r][c] && grid[r][c] >= 0) n++;
                if (n >= 3 && grid[r][c] >= 0) { int j; for (j = 0; j < n; j++) grid[r+j][c] = -1; cleared += n; }
            }
            if (cleared) {
                score += cleared * 10;
                for (c = 0; c < M3; c++) {
                    int w = M3 - 1;
                    for (r = M3 - 1; r >= 0; r--) if (grid[r][c] >= 0) grid[w--][c] = grid[r][c];
                    while (w >= 0) grid[w--][c] = rnd(6);
                }
            }
        } while (cleared);

        draw_title("MATCH THREE", "Arrows move, Enter picks two neighbours to swap, Q quits");
        for (r = 0; r < M3; r++) {
            scr_move(5 + r, 26);
            for (c = 0; c < M3; c++)
                printf("%s%s %s %s",
                       (r == cr && c == cc) ? BG_BLUE : (r == sr && c == sc) ? BG_GREEN : "",
                       COL[grid[r][c]], TSYM[ti][grid[r][c]], C_RESET);
        }
        draw_textf(5 + M3 + 1, 26, "%s   Score %-6d  Moves left %-3d  ", TNAME[ti], score, moves);
        scr_flush();

        if (moves <= 0) {
            draw_centered(5 + M3 + 3, 80, C_BOLD "Out of moves." C_RESET);
            scr_flush();
            score_report(p->title ? p->title : "matchthree", score);
            if (!confirm("\n  Play again?")) return;
            score = 0; moves = 30;
            for (r = 0; r < M3; r++) for (c = 0; c < M3; c++) grid[r][c] = rnd(6);
            continue;
        }

        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
        if (k == KEY_UP    && cr > 0)      cr--;
        if (k == KEY_DOWN  && cr < M3 - 1) cr++;
        if (k == KEY_LEFT  && cc > 0)      cc--;
        if (k == KEY_RIGHT && cc < M3 - 1) cc++;
        if (k != KEY_ENTER && k != ' ') continue;

        if (sr < 0) { sr = cr; sc = cc; continue; }
        if (abs(sr - cr) + abs(sc - cc) == 1) {
            int t = grid[sr][sc];
            grid[sr][sc] = grid[cr][cc];
            grid[cr][cc] = t;
            moves--;
        }
        sr = sc = -1;
    }
}

/* ========================================================== peg solitaire
 * params: variant selects the board shape.
 *
 * The five shapes are not decoration: English has 33 holes and a known
 * single-peg solution, European's extra corners make the centre finish
 * impossible, and the triangular board is a different puzzle entirely.
 */
#define PS 9

void fam_pegsolitaire(const GParams *p)
{
    static const char *SHAPE_NAME[5] = {"English", "European", "Triangular", "Diamond", "Square"};
    /* '.' hole, 'o' peg, ' ' off-board */
    static const char *ENGLISH[PS] = {
        "  ooo  ", "  ooo  ", "ooooooo", "ooo.ooo", "ooooooo", "  ooo  ", "  ooo  "};
    static const char *EUROPEAN[PS] = {
        "  ooo  ", " ooooo ", "ooooooo", "ooo.ooo", "ooooooo", " ooooo ", "  ooo  "};
    static const char *TRIANGLE[PS] = {
        "    .    ", "   o o   ", "  o o o  ", " o o o o ", "o o o o o"};
    static const char *DIAMOND[PS] = {
        "   o   ", "  ooo  ", " ooooo ", "ooo.ooo", " ooooo ", "  ooo  ", "   o   "};
    static const char *SQUARE[PS] = {
        "ooooo", "ooooo", "oo.oo", "ooooo", "ooooo"};
    static const char **SHAPES[5] = {ENGLISH, EUROPEAN, TRIANGLE, DIAMOND, SQUARE};
    static const int ROWS[5] = {7, 7, 5, 7, 5};

    int v = gp_int(p->variant, 0);
    int rows, cols, r, c, cr = 0, cc = 0, sr = -1, sc = -1;
    char bd[PS][PS + 1];

    if (v < 0 || v > 4) v = 0;
    rows = ROWS[v];

    for (;;) {
        int pegs = 0, k, moves_left = 0;
        for (r = 0; r < rows; r++) snprintf(bd[r], sizeof bd[0], "%s", SHAPES[v][r]);
        cols = (int)strlen(bd[0]);
        cr = cc = 0;
        sr = sc = -1;

        for (;;) {
            char sub[96];
            pegs = 0;
            moves_left = 0;
            for (r = 0; r < rows; r++) for (c = 0; c < cols; c++) {
                if (bd[r][c] == 'o') pegs++;
                /* Triangular boards jump along their own axes, so the step is
                 * two cells in each of the four orthogonal directions here. */
                if (bd[r][c] != 'o') continue;
                {
                    static const int DR[4] = {-2,2,0,0}, DC[4] = {0,0,-2,2};
                    int d;
                    for (d = 0; d < 4; d++) {
                        int mr = r + DR[d]/2, mc = c + DC[d]/2;
                        int tr = r + DR[d],   tc = c + DC[d];
                        if (tr < 0 || tr >= rows || tc < 0 || tc >= cols) continue;
                        if ((int)strlen(bd[tr]) <= tc) continue;
                        if (bd[mr][mc] == 'o' && bd[tr][tc] == '.') moves_left++;
                    }
                }
            }
            snprintf(sub, sizeof sub, "%s board — arrows move, Enter picks a peg then its landing hole",
                     SHAPE_NAME[v]);
            draw_title("PEG SOLITAIRE", sub);
            for (r = 0; r < rows; r++) {
                scr_move(6 + r, 32);
                for (c = 0; c < cols; c++) {
                    char ch = bd[r][c];
                    printf("%s%s%c %s",
                           (r == cr && c == cc) ? BG_BLUE : (r == sr && c == sc) ? BG_GREEN : "",
                           ch == 'o' ? C_YELLOW : C_GREY,
                           ch == ' ' ? ' ' : ch, C_RESET);
                }
            }
            draw_textf(7 + rows, 32, "Pegs left: %-3d   Moves available: %-3d  ", pegs, moves_left);
            scr_flush();

            if (pegs == 1) {
                draw_centered(9 + rows, 80, C_BOLD C_GREEN "One peg left - perfect!" C_RESET);
                scr_flush();
                score_report(p->title ? p->title : "pegsolitaire", 1000);
                break;
            }
            if (moves_left == 0) {
                char m[64];
                snprintf(m, sizeof m, "No moves left - %d pegs remain.", pegs);
                draw_centered(9 + rows, 80, m);
                scr_flush();
                score_report(p->title ? p->title : "pegsolitaire", 1000 / pegs);
                break;
            }

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP    && cr > 0)        cr--;
            if (k == KEY_DOWN  && cr < rows - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)        cc--;
            if (k == KEY_RIGHT && cc < cols - 1) cc++;
            if (k != KEY_ENTER && k != ' ') continue;

            if (sr < 0) {
                if (cr < rows && cc < (int)strlen(bd[cr]) && bd[cr][cc] == 'o') { sr = cr; sc = cc; }
                continue;
            }
            {
                int dr = cr - sr, dc = cc - sc;
                if ((abs(dr) == 2 && dc == 0) || (abs(dc) == 2 && dr == 0)) {
                    int mr = sr + dr / 2, mc = sc + dc / 2;
                    if (bd[cr][cc] == '.' && bd[mr][mc] == 'o') {
                        bd[sr][sc] = '.';
                        bd[mr][mc] = '.';
                        bd[cr][cc] = 'o';
                    }
                }
            }
            sr = sc = -1;
        }
        if (!confirm("\n  Play again?")) return;
    }
}
