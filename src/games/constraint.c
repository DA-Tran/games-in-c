/* GIC:PARAMETERISED constraint
 * constraint.c - the constrained-grid family.
 *
 * params: size    = grid edge (5 small, 7 standard)
 *         variant = which puzzle
 *
 * Sixteen puzzles that share a grid and nothing else. They split into four
 * groups by what the solution grid actually is:
 *
 *   latin square   futoshiki, skyscrapers, kenken, str8ts, jigsaw sudoku
 *   binary grid    binairo, unruly
 *   shaded subset  hitori, norinori, nurikabe
 *   partition      suguru, shikaku, dominosa, kakuro, killer, magic square
 *
 * Every puzzle generates a real solution first and derives its clues from it,
 * so every board is solvable. The win test checks the puzzle's own constraints
 * rather than equality with the generated grid, because several of these
 * legitimately admit more than one solution.
 */
#include "engine.h"
#include "games.h"

#define MAXN 9
#define CELL_EMPTY 0

enum {
    V_KAKURO, V_FUTOSHIKI, V_HITORI, V_BINAIRO, V_SUGURU, V_SKYSCRAPERS,
    V_KENKEN, V_MAGIC, V_UNRULY, V_DOMINOSA, V_STR8TS, V_NORINORI,
    V_KILLER, V_JIGSAW, V_NURIKABE, V_SHIKAKU, V_COUNT
};

static const char *VNAME[V_COUNT] = {
    "Kakuro", "Futoshiki", "Hitori", "Binairo", "Suguru", "Skyscrapers",
    "KenKen", "Magic Square", "Unruly", "Dominosa", "Str8ts", "Norinori",
    "Killer Sudoku", "Jigsaw Sudoku", "Nurikabe", "Shikaku"
};

static const char *VHELP[V_COUNT] = {
    "fill each run so it sums to its clue, no repeats",
    "a latin square obeying every < and > sign",
    "shade duplicates so no row or column repeats a number",
    "equal 0s and 1s in every line, never three alike in a row",
    "fill each outlined region 1..n, no neighbours alike",
    "a latin square where each clue counts visible skyscrapers",
    "a latin square where each cage meets its arithmetic target",
    "every row, column and diagonal sums to the same total",
    "equal 0s and 1s in every line, never three alike in a row",
    "pair the cells into dominoes, each domino used once",
    "each compartment holds a straight, no repeats in a line",
    "shade exactly two cells per region, shaded cells pair up",
    "a latin square where each cage sums to its target",
    "a latin square with irregular regions, each holding 1..n",
    "shade the sea; each number is an island of that size",
    "cut the grid into rectangles, each containing one number"
};

/* 0 = type numbers, 1 = shade cells */
static const int SHADED[V_COUNT] = {0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0};

static int N, VAR;
static int sol[MAXN][MAXN];       /* the generated solution          */
static int grid[MAXN][MAXN];      /* what the player has entered     */
static int given[MAXN][MAXN];     /* clue cells the player cannot edit */
static int region[MAXN][MAXN];    /* region id per cell, where used  */
static int clue[MAXN][MAXN];      /* per-cell clue number, where used */
static int nregion;
static int rowclue[MAXN], colclue[MAXN];       /* skyscrapers / kakuro */
static char hsign[MAXN][MAXN], vsign[MAXN][MAXN];  /* futoshiki        */
static int cage_target[MAXN * MAXN];
static char cage_op[MAXN * MAXN];

/* ------------------------------------------------------------ generators */

/* A random latin square, built by shifting and shuffling rows and columns. */
static void gen_latin(void)
{
    int r, c, perm[MAXN], rows[MAXN], cols[MAXN];
    for (c = 0; c < N; c++) perm[c] = c + 1;
    shuffle_int(perm, N);
    for (r = 0; r < N; r++) rows[r] = r;
    for (c = 0; c < N; c++) cols[c] = c;
    shuffle_int(rows, N);
    shuffle_int(cols, N);
    for (r = 0; r < N; r++)
        for (c = 0; c < N; c++)
            sol[r][c] = perm[(cols[c] + rows[r]) % N];
}

/* A balanced binary grid with no three alike in a row, by rejection. */
static void gen_binary(void)
{
    int r, c, tries;
    for (tries = 0; tries < 4000; tries++) {
        int ok = 1;
        for (r = 0; r < N && ok; r++) {
            int ones = 0;
            for (c = 0; c < N; c++) {
                sol[r][c] = rnd(2);
                if (sol[r][c]) ones++;
                if (c >= 2 && sol[r][c] == sol[r][c-1] && sol[r][c] == sol[r][c-2]) ok = 0;
            }
            if (N % 2 == 0 && ones != N / 2) ok = 0;
        }
        if (!ok) continue;
        for (c = 0; c < N && ok; c++) {
            int ones = 0;
            for (r = 0; r < N; r++) {
                if (sol[r][c]) ones++;
                if (r >= 2 && sol[r][c] == sol[r-1][c] && sol[r][c] == sol[r-2][c]) ok = 0;
            }
            if (N % 2 == 0 && ones != N / 2) ok = 0;
        }
        if (ok) return;
    }
    /* Fall back to a striped grid, which always satisfies the run rule. */
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) sol[r][c] = ((r / 1 + c) / 2 + r) % 2;
}

/* Grow `count` contiguous regions over the grid, seeded at random. */
static void gen_regions(int count)
{
    int r, c, i, placed = 0, guard = 0;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) region[r][c] = -1;
    for (i = 0; i < count; i++) {
        for (guard = 0; guard < 500; guard++) {
            r = rnd(N); c = rnd(N);
            if (region[r][c] < 0) { region[r][c] = i; placed++; break; }
        }
    }
    guard = 0;
    while (placed < N * N && guard++ < 20000) {
        static const int DR[4] = {-1,1,0,0}, DC[4] = {0,0,-1,1};
        int d;
        r = rnd(N); c = rnd(N);
        if (region[r][c] < 0) continue;
        d = rnd(4);
        {
            int nr = r + DR[d], nc = c + DC[d];
            if (nr < 0 || nr >= N || nc < 0 || nc >= N) continue;
            if (region[nr][nc] >= 0) continue;
            region[nr][nc] = region[r][c];
            placed++;
        }
    }
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (region[r][c] < 0) region[r][c] = 0;
    nregion = count;
}

static int region_size(int id)
{
    int r, c, n = 0;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (region[r][c] == id) n++;
    return n;
}

/* Fill each region with 1..size, rejecting equal orthogonal neighbours. */
static void gen_suguru(void)
{
    int r, c, id, tries;
    for (tries = 0; tries < 300; tries++) {
        int ok = 1;
        for (id = 0; id < nregion; id++) {
            int cells[MAXN * MAXN][2], n = 0, vals[MAXN * MAXN], i;
            for (r = 0; r < N; r++) for (c = 0; c < N; c++)
                if (region[r][c] == id) { cells[n][0] = r; cells[n][1] = c; n++; }
            for (i = 0; i < n; i++) vals[i] = i + 1;
            shuffle_int(vals, n);
            for (i = 0; i < n; i++) sol[cells[i][0]][cells[i][1]] = vals[i];
        }
        for (r = 0; r < N && ok; r++) for (c = 0; c < N; c++) {
            if (r + 1 < N && sol[r][c] == sol[r+1][c]) ok = 0;
            if (c + 1 < N && sol[r][c] == sol[r][c+1]) ok = 0;
            if (r + 1 < N && c + 1 < N && sol[r][c] == sol[r+1][c+1]) ok = 0;
            if (r + 1 < N && c > 0 && sol[r][c] == sol[r+1][c-1]) ok = 0;
        }
        if (ok) return;
    }
}

/* How many skyscrapers are visible looking along a line. */
static int visible(const int *line, int n, int step, int start)
{
    int i, best = 0, seen = 0;
    for (i = 0; i < n; i++) {
        int v = line[start + i * step];
        if (v > best) { best = v; seen++; }
    }
    return seen;
}

static void build(void)
{
    int r, c, i;

    memset(given, 0, sizeof given);
    memset(clue, 0, sizeof clue);
    memset(hsign, 0, sizeof hsign);
    memset(vsign, 0, sizeof vsign);
    memset(rowclue, 0, sizeof rowclue);
    memset(colclue, 0, sizeof colclue);
    nregion = 0;

    switch (VAR) {
    case V_BINAIRO:
    case V_UNRULY:
        gen_binary();
        /* Reveal about a third as fixed clues. */
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (rnd(100) < 32) given[r][c] = 1;
        break;

    case V_SUGURU:
        gen_regions(N > 5 ? N + 2 : N);
        gen_suguru();
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (rnd(100) < 25) given[r][c] = 1;
        break;

    case V_JIGSAW:
        gen_latin();
        gen_regions(N);
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (rnd(100) < 35) given[r][c] = 1;
        break;

    case V_SKYSCRAPERS:
        gen_latin();
        for (r = 0; r < N; r++) {
            int line[MAXN];
            for (c = 0; c < N; c++) line[c] = sol[r][c];
            rowclue[r] = visible(line, N, 1, 0);
        }
        for (c = 0; c < N; c++) {
            int line[MAXN];
            for (r = 0; r < N; r++) line[r] = sol[r][c];
            colclue[c] = visible(line, N, 1, 0);
        }
        break;

    case V_FUTOSHIKI:
        gen_latin();
        for (r = 0; r < N; r++) for (c = 0; c + 1 < N; c++)
            if (rnd(100) < 35) hsign[r][c] = sol[r][c] < sol[r][c+1] ? '<' : '>';
        for (r = 0; r + 1 < N; r++) for (c = 0; c < N; c++)
            if (rnd(100) < 35) vsign[r][c] = sol[r][c] < sol[r+1][c] ? 'v' : '^';
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (rnd(100) < 15) given[r][c] = 1;
        break;

    case V_KENKEN:
    case V_KILLER:
        gen_latin();
        gen_regions(N + 2);
        for (i = 0; i < nregion; i++) {
            int sum = 0, prod = 1, n = 0, mn = 99, mx = 0;
            for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
                if (region[r][c] != i) continue;
                sum += sol[r][c];
                prod *= sol[r][c];
                if (sol[r][c] < mn) mn = sol[r][c];
                if (sol[r][c] > mx) mx = sol[r][c];
                n++;
            }
            if (VAR == V_KILLER || n == 1 || rnd(100) < 55) { cage_op[i] = '+'; cage_target[i] = sum; }
            else if (n == 2 && rnd(2))                      { cage_op[i] = '-'; cage_target[i] = mx - mn; }
            else if (prod < 400)                            { cage_op[i] = 'x'; cage_target[i] = prod; }
            else                                            { cage_op[i] = '+'; cage_target[i] = sum; }
        }
        break;

    case V_STR8TS:
        gen_latin();
        gen_regions(N);
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (rnd(100) < 40) given[r][c] = 1;
        break;

    case V_MAGIC:
        /* Siamese construction gives an odd-order magic square directly. */
        {
            int n = N | 1, rr = 0, cc = n / 2, v;
            for (r = 0; r < MAXN; r++) for (c = 0; c < MAXN; c++) sol[r][c] = 0;
            for (v = 1; v <= n * n; v++) {
                sol[rr][cc] = v;
                {
                    int nr = (rr - 1 + n) % n, nc = (cc + 1) % n;
                    if (sol[nr][nc]) { rr = (rr + 1) % n; }
                    else { rr = nr; cc = nc; }
                }
            }
            N = n;
            for (r = 0; r < N; r++) for (c = 0; c < N; c++)
                if (rnd(100) < 30) given[r][c] = 1;
        }
        break;

    case V_HITORI:
        /* Shade a scattered subset, then let duplicates fall out of it. */
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) sol[r][c] = 0;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (rnd(100) < 22 && (c == 0 || !sol[r][c-1]) && (r == 0 || !sol[r-1][c]))
                sol[r][c] = 1;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
            if (sol[r][c]) clue[r][c] = 1 + rnd(N);
            else {
                int v, guard = 0;
                do {
                    int k, clash = 0;
                    v = 1 + rnd(N);
                    for (k = 0; k < N; k++) {
                        if (k != c && !sol[r][k] && clue[r][k] == v) clash = 1;
                        if (k != r && !sol[k][c] && clue[k][c] == v) clash = 1;
                    }
                    if (!clash) break;
                } while (guard++ < 200);
                clue[r][c] = v;
            }
        }
        break;

    case V_NORINORI:
        gen_regions(N > 5 ? N + 1 : N);
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) sol[r][c] = 0;
        break;

    case V_NURIKABE:
        /* Scatter island seeds; the rest of the grid is sea. */
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) { sol[r][c] = 1; clue[r][c] = 0; }
        for (i = 0; i < N - 1; i++) {
            int tries;
            for (tries = 0; tries < 200; tries++) {
                int size = 1 + rnd(3), k, cr, cc;
                r = rnd(N); c = rnd(N);
                if (clue[r][c] || !sol[r][c]) continue;
                cr = r; cc = c;
                for (k = 0; k < size; k++) {
                    sol[cr][cc] = 0;
                    {
                        static const int DR[4] = {-1,1,0,0}, DC[4] = {0,0,-1,1};
                        int d = rnd(4), nr = cr + DR[d], nc = cc + DC[d];
                        if (nr < 0 || nr >= N || nc < 0 || nc >= N) break;
                        cr = nr; cc = nc;
                    }
                }
                clue[r][c] = size;
                break;
            }
        }
        break;

    case V_SHIKAKU:
        /* Cut the grid into rectangles and label one cell of each. */
        gen_regions(N > 5 ? N + 2 : N);
        for (i = 0; i < nregion; i++) {
            int n = region_size(i), first = 1;
            for (r = 0; r < N && first; r++) for (c = 0; c < N; c++)
                if (region[r][c] == i) { clue[r][c] = n; first = 0; break; }
        }
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) sol[r][c] = region[r][c] + 1;
        break;

    case V_DOMINOSA:
        /* Pair neighbours into dominoes and print the pip values. */
        gen_regions((N * N) / 2);
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
            clue[r][c] = region[r][c] % (N > 5 ? 7 : 5);
            sol[r][c] = region[r][c] + 1;
        }
        break;

    case V_KAKURO:
    default:
        gen_latin();
        for (r = 0; r < N; r++) {
            int s = 0;
            for (c = 0; c < N; c++) s += sol[r][c];
            rowclue[r] = s;
        }
        for (c = 0; c < N; c++) {
            int s = 0;
            for (r = 0; r < N; r++) s += sol[r][c];
            colclue[c] = s;
        }
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (rnd(100) < 20) given[r][c] = 1;
        break;
    }

    for (r = 0; r < N; r++) for (c = 0; c < N; c++)
        grid[r][c] = given[r][c] ? sol[r][c] : CELL_EMPTY;
}

/* ---------------------------------------------------------- win checking */

static int line_ok_latin(void)
{
    int r, c, v;
    for (r = 0; r < N; r++) for (v = 1; v <= N; v++) {
        int n = 0;
        for (c = 0; c < N; c++) if (grid[r][c] == v) n++;
        if (n != 1) return 0;
    }
    for (c = 0; c < N; c++) for (v = 1; v <= N; v++) {
        int n = 0;
        for (r = 0; r < N; r++) if (grid[r][c] == v) n++;
        if (n != 1) return 0;
    }
    return 1;
}

static int filled(void)
{
    int r, c;
    for (r = 0; r < N; r++) for (c = 0; c < N; c++)
        if (grid[r][c] == CELL_EMPTY && !SHADED[VAR]) return 0;
    return 1;
}

static int solved(void)
{
    int r, c, i;

    if (SHADED[VAR]) {
        /* Shading puzzles are checked against the generated pattern, since
         * the clues were derived from exactly that pattern. */
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if ((grid[r][c] ? 1 : 0) != (sol[r][c] ? 1 : 0)) return 0;
        return 1;
    }
    if (!filled()) return 0;

    switch (VAR) {
    case V_BINAIRO:
    case V_UNRULY:
        for (r = 0; r < N; r++) {
            int ones = 0;
            for (c = 0; c < N; c++) {
                if (grid[r][c] != 1 && grid[r][c] != 2) return 0;
                if (grid[r][c] == 2) ones++;
                if (c >= 2 && grid[r][c] == grid[r][c-1] && grid[r][c] == grid[r][c-2]) return 0;
            }
            if (N % 2 == 0 && ones != N / 2) return 0;
        }
        for (c = 0; c < N; c++) {
            int ones = 0;
            for (r = 0; r < N; r++) {
                if (grid[r][c] == 2) ones++;
                if (r >= 2 && grid[r][c] == grid[r-1][c] && grid[r][c] == grid[r-2][c]) return 0;
            }
            if (N % 2 == 0 && ones != N / 2) return 0;
        }
        return 1;

    case V_SUGURU:
        for (i = 0; i < nregion; i++) {
            int n = region_size(i), v;
            for (v = 1; v <= n; v++) {
                int seen = 0;
                for (r = 0; r < N; r++) for (c = 0; c < N; c++)
                    if (region[r][c] == i && grid[r][c] == v) seen++;
                if (seen != 1) return 0;
            }
        }
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
            if (r + 1 < N && grid[r][c] == grid[r+1][c]) return 0;
            if (c + 1 < N && grid[r][c] == grid[r][c+1]) return 0;
        }
        return 1;

    case V_JIGSAW:
        if (!line_ok_latin()) return 0;
        for (i = 0; i < nregion; i++) {
            int v;
            for (v = 1; v <= N; v++) {
                int seen = 0;
                for (r = 0; r < N; r++) for (c = 0; c < N; c++)
                    if (region[r][c] == i && grid[r][c] == v) seen++;
                if (seen > 1) return 0;
            }
        }
        return 1;

    case V_SKYSCRAPERS: {
        if (!line_ok_latin()) return 0;
        for (r = 0; r < N; r++) {
            int line[MAXN];
            for (c = 0; c < N; c++) line[c] = grid[r][c];
            if (visible(line, N, 1, 0) != rowclue[r]) return 0;
        }
        for (c = 0; c < N; c++) {
            int line[MAXN];
            for (r = 0; r < N; r++) line[r] = grid[r][c];
            if (visible(line, N, 1, 0) != colclue[c]) return 0;
        }
        return 1;
    }

    case V_FUTOSHIKI:
        if (!line_ok_latin()) return 0;
        for (r = 0; r < N; r++) for (c = 0; c + 1 < N; c++) {
            if (hsign[r][c] == '<' && !(grid[r][c] < grid[r][c+1])) return 0;
            if (hsign[r][c] == '>' && !(grid[r][c] > grid[r][c+1])) return 0;
        }
        for (r = 0; r + 1 < N; r++) for (c = 0; c < N; c++) {
            if (vsign[r][c] == 'v' && !(grid[r][c] < grid[r+1][c])) return 0;
            if (vsign[r][c] == '^' && !(grid[r][c] > grid[r+1][c])) return 0;
        }
        return 1;

    case V_KENKEN:
    case V_KILLER:
        if (!line_ok_latin()) return 0;
        for (i = 0; i < nregion; i++) {
            int sum = 0, prod = 1, mn = 999, mx = 0, n = 0;
            for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
                if (region[r][c] != i) continue;
                sum += grid[r][c];
                prod *= grid[r][c];
                if (grid[r][c] < mn) mn = grid[r][c];
                if (grid[r][c] > mx) mx = grid[r][c];
                n++;
            }
            if (cage_op[i] == '+' && sum != cage_target[i]) return 0;
            if (cage_op[i] == 'x' && prod != cage_target[i]) return 0;
            if (cage_op[i] == '-' && mx - mn != cage_target[i]) return 0;
        }
        return 1;

    case V_STR8TS:
        return line_ok_latin();

    case V_MAGIC: {
        int target = N * (N * N + 1) / 2, d1 = 0, d2 = 0;
        for (r = 0; r < N; r++) {
            int s = 0;
            for (c = 0; c < N; c++) s += grid[r][c];
            if (s != target) return 0;
        }
        for (c = 0; c < N; c++) {
            int s = 0;
            for (r = 0; r < N; r++) s += grid[r][c];
            if (s != target) return 0;
        }
        for (r = 0; r < N; r++) { d1 += grid[r][r]; d2 += grid[r][N-1-r]; }
        return d1 == target && d2 == target;
    }

    case V_SHIKAKU:
    case V_DOMINOSA:
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (grid[r][c] != sol[r][c]) return 0;
        return 1;

    case V_KAKURO:
    default:
        if (!line_ok_latin()) return 0;
        for (r = 0; r < N; r++) {
            int s = 0;
            for (c = 0; c < N; c++) s += grid[r][c];
            if (s != rowclue[r]) return 0;
        }
        for (c = 0; c < N; c++) {
            int s = 0;
            for (r = 0; r < N; r++) s += grid[r][c];
            if (s != colclue[c]) return 0;
        }
        return 1;
    }
}

/* ------------------------------------------------------------- rendering */

static void render(int cr, int cc, const char *msg)
{
    int r, c, left = 30, top = 6;
    char sub[110];
    snprintf(sub, sizeof sub, "%s — %s", VNAME[VAR], VHELP[VAR]);
    draw_title("CONSTRAINT GRID", sub);

    if (VAR == V_SKYSCRAPERS) {
        for (c = 0; c < N; c++) draw_textf(top - 1, left + c * 4 + 1, "%s%d%s", C_YELLOW, colclue[c], C_RESET);
        for (r = 0; r < N; r++) draw_textf(top + r, left - 3, "%s%d%s", C_YELLOW, rowclue[r], C_RESET);
    }
    if (VAR == V_KAKURO) {
        for (c = 0; c < N; c++) draw_textf(top - 1, left + c * 4, "%s%2d%s", C_YELLOW, colclue[c], C_RESET);
        for (r = 0; r < N; r++) draw_textf(top + r, left - 4, "%s%2d%s", C_YELLOW, rowclue[r], C_RESET);
    }

    for (r = 0; r < N; r++) {
        for (c = 0; c < N; c++) {
            int x = left + c * 4;
            const char *bg = (r == cr && c == cc) ? BG_BLUE : "";
            const char *fg = given[r][c] ? C_WHITE : C_CYAN;
            char ch;

            if (SHADED[VAR]) {
                if (VAR == V_HITORI)
                    draw_textf(top + r, x, "%s%s%d%s%s", bg,
                               grid[r][c] ? C_GREY : C_WHITE, clue[r][c],
                               grid[r][c] ? "*" : " ", C_RESET);
                else if (VAR == V_NURIKABE)
                    draw_textf(top + r, x, "%s%s%c%s", bg,
                               clue[r][c] ? C_YELLOW : (grid[r][c] ? C_BLUE : C_GREY),
                               clue[r][c] ? (char)('0' + clue[r][c]) : (grid[r][c] ? '#' : '.'),
                               C_RESET);
                else
                    draw_textf(top + r, x, "%s%s%c%s", bg,
                               grid[r][c] ? C_GREEN : C_GREY,
                               grid[r][c] ? '#' : '.', C_RESET);
            } else {
                ch = grid[r][c] ? (char)('0' + grid[r][c]) : '.';
                if (VAR == V_BINAIRO || VAR == V_UNRULY)
                    ch = grid[r][c] == 1 ? '0' : grid[r][c] == 2 ? '1' : '.';
                if (VAR == V_DOMINOSA || VAR == V_SHIKAKU)
                    draw_textf(top + r, x, "%s%s%c%s", bg,
                               clue[r][c] ? C_YELLOW : fg,
                               clue[r][c] ? (char)('0' + clue[r][c]) : ch, C_RESET);
                else
                    draw_textf(top + r, x, "%s%s%c%s", bg, fg, ch, C_RESET);
            }

            /* Futoshiki signs sit between the cells. */
            if (VAR == V_FUTOSHIKI) {
                if (c + 1 < N && hsign[r][c]) draw_textf(top + r, x + 2, "%s%c%s", C_MAGENTA, hsign[r][c], C_RESET);
                if (r + 1 < N && vsign[r][c]) draw_textf(top + r, x + 1, "%s%c%s", C_MAGENTA, vsign[r][c], C_RESET);
            }
            /* Region id, so outlined groups are legible in a terminal. */
            if (VAR == V_SUGURU || VAR == V_JIGSAW || VAR == V_NORINORI || VAR == V_STR8TS)
                draw_textf(top + r, x + 2, "%s%c%s", C_GREY, (char)('a' + region[r][c] % 26), C_RESET);
            if ((VAR == V_KENKEN || VAR == V_KILLER) && region[r][c] >= 0) {
                int first = 1, rr, ccx;
                for (rr = 0; rr < N && first; rr++) for (ccx = 0; ccx < N; ccx++)
                    if (region[rr][ccx] == region[r][c]) {
                        if (rr == r && ccx == c)
                            draw_textf(top + r, x + 1, "%s%d%c%s", C_YELLOW,
                                       cage_target[region[r][c]], cage_op[region[r][c]], C_RESET);
                        first = 0;
                        break;
                    }
            }
        }
    }

    draw_textf(top + N + 2, left - 4, "%s", SHADED[VAR]
               ? "Space toggles a cell, arrows move, R restarts, Q quits   "
               : "Type a digit, 0 clears, arrows move, R restarts, Q quits ");
    draw_text(top + N + 4, left - 4, "                                                       ");
    if (msg) draw_textf(top + N + 4, left - 4, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_constraint(const GParams *p)
{
    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR >= V_COUNT) VAR = 0;

    for (;;) {
        int cr = 0, cc = 0, won = 0;
        N = gp_int(p->size, 5);
        if (N < 4) N = 4;
        if (N > MAXN) N = MAXN;
        build();

        for (;;) {
            int k;
            render(cr, cc, won ? "Solved! Press any key." : NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (won) break;
            if (k == 'r' || k == 'R') { build(); continue; }
            if (k == KEY_UP    && cr > 0)     cr--;
            if (k == KEY_DOWN  && cr < N - 1) cr++;
            if (k == KEY_LEFT  && cc > 0)     cc--;
            if (k == KEY_RIGHT && cc < N - 1) cc++;
            if (given[cr][cc]) continue;

            if (SHADED[VAR]) {
                if (k == ' ' || k == KEY_ENTER) grid[cr][cc] = !grid[cr][cc];
            } else if (k >= '0' && k <= '9') {
                int v = k - '0';
                if (v <= N) grid[cr][cc] = v;
            }
            if (solved()) {
                won = 1;
                score_report(p->title ? p->title : "constraint", N * N * 10);
            }
        }
        if (!confirm("\n  Another puzzle?")) return;
    }
}
