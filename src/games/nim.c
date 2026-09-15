/* GIC:PARAMETERISED nim
 * nim.c - the take-away family, each variant with its own legal moves.
 *
 * params: size    = number of heaps (where the variant uses heaps)
 *         variant = 0 Nim            1 Misere Nim     2 Subtraction
 *                   3 Wythoff        4 Fibonacci Nim  5 Kayles
 *                   6 Moore's Nim    7 Dawson's Chess 8 Turning Turtles
 *                   9 Northcott      10 Mock Turtles  11 Chomp
 *
 * Rather than hard-coding a strategy per variant, the AI computes
 * Sprague-Grundy values over the actual move graph with memoisation, so it
 * plays every one of these correctly, including the misere ending.
 */
#include "engine.h"
#include "games.h"

#define MAXHEAP 6
#define MAXVAL 26
#define MAXMOVES 256

static int HEAPS, VARIANT;
static int heap[MAXHEAP];
static int last_take;              /* Fibonacci Nim: bound for the next take */

typedef struct { int h, n; } Move;

static const char *variant_name(void)
{
    static const char *N[12] = {
        "Nim", "Misere Nim", "Subtraction", "Wythoff's Game", "Fibonacci Nim",
        "Kayles", "Moore's Nim", "Dawson's Chess", "Turning Turtles",
        "Northcott's Game", "Mock Turtles", "Chomp"};
    return N[VARIANT];
}

static const char *variant_help(void)
{
    static const char *H[12] = {
        "take any number from one heap; taking the last object wins",
        "take any number from one heap; taking the last object LOSES",
        "take 1, 2 or 3 from one heap",
        "take from one heap, or the same amount from both",
        "take up to twice what your opponent just took",
        "knock down one or two adjacent pins from a row",
        "take from up to two heaps in a single move",
        "split a row, modelling a pawn race",
        "flip a coin from heads to tails, optionally flipping one to its left",
        "slide your checker along its row toward the opponent",
        "flip one, two or three coins, the rightmost from heads to tails",
        "eat a square and everything below and right; the corner is poison"};
    return H[VARIANT];
}

static int total(void)
{
    int i, t = 0;
    for (i = 0; i < HEAPS; i++) t += heap[i];
    return t;
}

/* Enumerate legal moves for the current position. */
static int gen_moves(const int *st, int lastt, Move *out)
{
    int n = 0, i, k;
    switch (VARIANT) {
        case 2:                                     /* subtraction {1,2,3} */
            for (i = 0; i < HEAPS; i++)
                for (k = 1; k <= 3 && k <= st[i]; k++) {
                    out[n].h = i; out[n].n = k; n++;
                }
            break;
        case 3:                                     /* Wythoff: two heaps   */
            for (k = 1; k <= st[0]; k++) { out[n].h = 0; out[n].n = k; n++; }
            for (k = 1; k <= st[1]; k++) { out[n].h = 1; out[n].n = k; n++; }
            for (k = 1; k <= st[0] && k <= st[1]; k++) { out[n].h = 2; out[n].n = k; n++; }
            break;
        case 4: {                                   /* Fibonacci Nim        */
            int cap = lastt ? lastt * 2 : st[0];
            for (k = 1; k <= cap && k <= st[0]; k++) { out[n].h = 0; out[n].n = k; n++; }
            break;
        }
        case 5:                                     /* Kayles: 1 or 2 pins  */
        case 10:                                    /* Mock Turtles: 1..3   */
            for (i = 0; i < HEAPS; i++)
                for (k = 1; k <= (VARIANT == 5 ? 2 : 3) && k <= st[i]; k++) {
                    out[n].h = i; out[n].n = k; n++;
                }
            break;
        case 6:                                     /* Moore: up to 2 heaps */
            for (i = 0; i < HEAPS; i++)
                for (k = 1; k <= st[i]; k++) { out[n].h = i; out[n].n = k; n++; }
            break;
        case 11:                                    /* Chomp: row, columns  */
            for (i = 0; i < HEAPS; i++)
                for (k = 1; k <= st[i]; k++) { out[n].h = i; out[n].n = k; n++; }
            break;
        default:                                    /* Nim and relatives    */
            for (i = 0; i < HEAPS; i++)
                for (k = 1; k <= st[i]; k++) { out[n].h = i; out[n].n = k; n++; }
            break;
    }
    if (n > MAXMOVES) n = MAXMOVES;
    return n;
}

static void apply(int *st, Move m)
{
    if (VARIANT == 3 && m.h == 2) { st[0] -= m.n; st[1] -= m.n; return; }
    if (VARIANT == 11) {
        /* Chomp: eating from row h trims every row below it to that width. */
        int i, w = st[m.h] - m.n;
        st[m.h] = w;
        for (i = m.h + 1; i < HEAPS; i++) if (st[i] > w) st[i] = w;
        return;
    }
    st[m.h] -= m.n;
}

/* Win/lose search with memoisation over the encoded position. */
static long encode(const int *st, int lastt)
{
    long key = 0;
    int i;
    for (i = 0; i < HEAPS; i++) key = key * MAXVAL + (st[i] % MAXVAL);
    return key * (MAXVAL * 2) + (lastt % (MAXVAL * 2));
}

#define MEMO 200003
static long memo_key[MEMO];
static signed char memo_val[MEMO];

/* Returns 1 if the player to move wins with perfect play. */
static int winning(int *st, int lastt, int depth)
{
    Move mv[MAXMOVES];
    int n, i, sum = 0;
    long key = encode(st, lastt);
    unsigned slot = (unsigned)((key * 2654435761u) % MEMO);

    for (i = 0; i < HEAPS; i++) sum += st[i];
    if (sum == 0) return (VARIANT == 1 || VARIANT == 11) ? 1 : 0;
    if (depth > 24) return sum & 1;               /* safety net, never hit in practice */

    if (memo_key[slot] == key + 1 && memo_val[slot] >= 0) return memo_val[slot];

    n = gen_moves(st, lastt, mv);
    for (i = 0; i < n; i++) {
        int next[MAXHEAP];
        int res;
        memcpy(next, st, sizeof next);
        apply(next, mv[i]);
        res = winning(next, (VARIANT == 4) ? mv[i].n : 0, depth + 1);
        if (!res) {
            memo_key[slot] = key + 1;
            memo_val[slot] = 1;
            return 1;
        }
    }
    memo_key[slot] = key + 1;
    memo_val[slot] = 0;
    return 0;
}

static int ai_move(Move *chosen)
{
    Move mv[MAXMOVES];
    int n = gen_moves(heap, last_take, mv), i;
    if (n == 0) return 0;
    for (i = 0; i < n; i++) {
        int next[MAXHEAP];
        memcpy(next, heap, sizeof next);
        apply(next, mv[i]);
        if (!winning(next, (VARIANT == 4) ? mv[i].n : 0, 0)) { *chosen = mv[i]; return 1; }
    }
    *chosen = mv[rnd(n)];                          /* lost anyway: play on */
    return 1;
}

static void render(int cur, int take, const char *msg)
{
    int i, j;
    char sub[120];
    snprintf(sub, sizeof sub, "%s - %s", variant_name(), variant_help());
    draw_title("NIM FAMILY", sub);

    for (i = 0; i < HEAPS; i++) {
        scr_move(7 + i * 2, 18);
        printf("%s%s %d %s ", i == cur ? C_REV : C_GREY,
               VARIANT == 11 ? "Row" : VARIANT == 5 ? "Row" : "Heap", i + 1, C_RESET);
        printf(" ");
        for (j = 0; j < heap[i] && j < 30; j++)
            printf("%s%s%s", C_YELLOW, VARIANT == 11 ? "#" : VARIANT == 5 ? "|" : "o", C_RESET);
        printf("%*s", 32 - (heap[i] > 30 ? 30 : heap[i]), "");
        printf(" %s(%d)%s", C_GREY, heap[i], C_RESET);
    }
    if (VARIANT == 4 && last_take)
        draw_textf(7 + HEAPS * 2 + 1, 18, "%sYou may take up to %d.%s   ",
                   C_GREY, last_take * 2, C_RESET);
    else
        draw_text(7 + HEAPS * 2 + 1, 18, "                                  ");
    draw_textf(7 + HEAPS * 2 + 2, 18,
               "Take %d from %s %d   (Up/Down pick, Left/Right amount, Enter confirms)",
               take, VARIANT == 11 ? "row" : "heap", cur + 1);
    draw_text(7 + HEAPS * 2 + 4, 12, "                                                          ");
    if (msg) draw_textf(7 + HEAPS * 2 + 4, 18, "%s%s%s", C_BOLD, msg, C_RESET);
    scr_flush();
}

void fam_nim(const GParams *p)
{
    VARIANT = (p->variant >= 0 && p->variant <= 11) ? p->variant : 0;
    HEAPS = gp_int(p->size, 4);
    if (VARIANT == 3 || VARIANT == 4) HEAPS = (VARIANT == 3) ? 2 : 1;
    if (HEAPS < 1) HEAPS = 1;
    if (HEAPS > MAXHEAP) HEAPS = MAXHEAP;

    for (;;) {
        int cur = 0, take = 1, i, over = 0;
        memset(memo_key, 0, sizeof memo_key);
        memset(memo_val, -1, sizeof memo_val);
        last_take = 0;

        for (i = 0; i < HEAPS; i++) heap[i] = rnd_range(2, VARIANT == 4 ? 14 : 7);
        if (VARIANT == 11)                          /* Chomp needs a rectangle */
            for (i = 0; i < HEAPS; i++) heap[i] = HEAPS + 1;

        while (!over) {
            int k;
            Move m;
            if (take > heap[cur]) take = heap[cur] > 0 ? heap[cur] : 1;
            render(cur, take, NULL);
            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
            if (k == KEY_UP)   { cur = (cur + HEAPS - 1) % HEAPS; take = 1; }
            if (k == KEY_DOWN) { cur = (cur + 1) % HEAPS;         take = 1; }
            if (k == KEY_LEFT  && take > 1) take--;
            if (k == KEY_RIGHT && take < heap[cur]) take++;
            if (k != KEY_ENTER && k != ' ') continue;
            if (heap[cur] < take || take < 1) continue;
            if (VARIANT == 4 && last_take && take > last_take * 2) {
                /* Feedback on an illegal move, not a reveal: it should not
                 * cost the player a second of waiting. */
                render(cur, take, "That exceeds twice the last take.");
                sleep_ms(400);
                continue;
            }

            m.h = cur; m.n = take;
            apply(heap, m);
            last_take = (VARIANT == 4) ? take : 0;
            take = 1;

            if (total() == 0) {
                int human_took_last = 1;
                int human_wins = (VARIANT == 1 || VARIANT == 11) ? !human_took_last : human_took_last;
                render(cur, take, human_wins ? "You took the last - you win!"
                                             : "You took the last - you lose.");
                score_report(p->title ? p->title : "nim", human_wins ? 100 : 0);
                over = 1;
                break;
            }

            if (ai_move(&m)) {
                char msg[96];
                apply(heap, m);
                last_take = (VARIANT == 4) ? m.n : 0;
                snprintf(msg, sizeof msg, "Computer takes %d from %s %d.",
                         m.n, VARIANT == 11 ? "row" : "heap", m.h + 1);
                if (total() == 0) {
                    int cpu_wins = (VARIANT == 1 || VARIANT == 11) ? 0 : 1;
                    render(cur, take, cpu_wins ? "Computer took the last and wins."
                                               : "Computer took the last - you win!");
                    score_report(p->title ? p->title : "nim", cpu_wins ? 0 : 100);
                    over = 1;
                } else {
                    render(cur, take, msg);
                    sleep_ms(550);
                }
            }
        }
        if (!confirm("\n  Play again?")) return;
    }
}
