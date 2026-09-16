/* engine_test.c - unit tests for the engine primitives every game shares.
 *
 * The other suites drive whole games and infer the engine from their
 * behaviour. That works for anything a game visibly does, and fails for
 * anything it only does sometimes. The high-score file is the clearest case:
 * writing it back is what destroys it, and whether a scripted run scores at
 * all depends on the game, so a per-game check finds the bug only if it
 * happens to pick a game that scored. Running one game a thousand ways is not
 * a substitute for testing the thing directly.
 *
 * These assert contracts rather than remembered values - that rnd(n) lands
 * inside [0,n) for every n, not that seed 7 gives 583 - so they stay true when
 * the implementation changes and still fail when it breaks.
 *
 *   make unit
 */
#include "engine.h"
#include <stdio.h>
#include <limits.h>

/* The engine's display helpers are not linked in: these are the only symbols
 * util.c needs from the rest of the build. */
void scr_size(int *r, int *c) { *r = 24; *c = 80; }
void draw_centered(int r, int c, const char *s) { (void)r; (void)c; (void)s; }
void scr_flush(void) { }
const FamilyEntry FAMILY_REGISTRY[1] = { { 0, 0 } };
const int FAMILY_REGISTRY_COUNT = 0;

static int failures, checks;

#define CHECK(cond, ...)                                                    \
    do {                                                                    \
        checks++;                                                           \
        if (!(cond)) {                                                      \
            failures++;                                                     \
            printf("  FAIL  %s:%d  ", __FILE__, __LINE__);                  \
            printf(__VA_ARGS__);                                            \
            printf("\n");                                                   \
        }                                                                   \
    } while (0)

#define SCORES ".gic_scores"

static void write_file(const char *text)
{
    FILE *f = fopen(SCORES, "w");
    if (f) { fputs(text, f); fclose(f); }
}

static int line_count(void)
{
    FILE *f = fopen(SCORES, "r");
    int n = 0, c, last = '\n';
    if (!f) return 0;
    while ((c = fgetc(f)) != EOF) { if (c == '\n') n++; last = c; }
    if (last != '\n') n++;
    fclose(f);
    return n;
}

/* ------------------------------------------------------------- rng ----- */

static void test_rng(void)
{
    int i, n;
    unsigned s;

    /* Range. The contract is [0,n); nothing here depends on which values. */
    for (n = 1; n <= 64; n++) {
        for (i = 0; i < 200; i++) {
            int v = rnd(n);
            CHECK(v >= 0 && v < n, "rnd(%d) returned %d, outside [0,%d)", n, v, n);
        }
    }

    /* Documented guards: a non-positive bound must not divide by zero. */
    CHECK(rnd(0) == 0, "rnd(0) should be 0, got %d", rnd(0));
    CHECK(rnd(-5) == 0, "rnd(-5) should be 0, got %d", rnd(-5));

    /* rnd_range is inclusive at both ends and tolerates reversed arguments. */
    for (i = 0; i < 400; i++) {
        int v = rnd_range(-7, 7);
        CHECK(v >= -7 && v <= 7, "rnd_range(-7,7) gave %d", v);
        v = rnd_range(9, 2);
        CHECK(v >= 2 && v <= 9, "rnd_range(9,2) should still be in [2,9], got %d", v);
        v = rnd_range(3, 3);
        CHECK(v == 3, "rnd_range(3,3) should be 3, got %d", v);
    }

    for (i = 0; i < 400; i++) {
        double d = rnd_f();
        CHECK(d >= 0.0 && d < 1.0, "rnd_f() gave %f, outside [0,1)", d);
    }

    /* Reproducibility, which is what the seeded catalogue entries rest on. */
    for (s = 1; s <= 5; s++) {
        int a[32], b[32];
        rng_seed(s);
        for (i = 0; i < 32; i++) a[i] = rnd(10000);
        rng_seed(s);
        for (i = 0; i < 32; i++) b[i] = rnd(10000);
        for (i = 0; i < 32; i++)
            CHECK(a[i] == b[i], "seed %u draw %d: %d then %d", s, i, a[i], b[i]);
    }

    /* Seeding with zero must not leave the generator stuck: xorshift can never
     * leave a zero state, so the implementation substitutes a constant. */
    rng_seed(0);
    {
        int same = 1, first = rnd(100000);
        for (i = 0; i < 20; i++) if (rnd(100000) != first) same = 0;
        CHECK(!same, "rng_seed(0) leaves the generator emitting one value");
    }

    /* A shuffle has to be a permutation - every element still present once. */
    {
        int arr[64], seen[64], k;
        for (k = 0; k < 64; k++) arr[k] = k;
        rng_seed(12345);
        shuffle_int(arr, 64);
        for (k = 0; k < 64; k++) seen[k] = 0;
        for (k = 0; k < 64; k++) {
            CHECK(arr[k] >= 0 && arr[k] < 64, "shuffle produced %d", arr[k]);
            if (arr[k] >= 0 && arr[k] < 64) seen[arr[k]]++;
        }
        for (k = 0; k < 64; k++)
            CHECK(seen[k] == 1, "shuffle lost or duplicated %d (seen %d times)",
                  k, seen[k]);
    }
}

/* ---------------------------------------------------------- scores ----- */

static void test_scores(void)
{
    /* Keys really are titles: almost every caller passes p->title, so spaces
     * and apostrophes are the normal case, not an edge case. */
    static const char *keys[] = {
        "snake", "Nine Men's Morris", "Zz Canary Three Words", "2048"
    };
    int i;

    write_file("");
    for (i = 0; i < 4; i++) score_save(keys[i], 100 + i);
    for (i = 0; i < 4; i++)
        CHECK(score_load(keys[i]) == 100 + i,
              "%s saved %d, loaded %d", keys[i], 100 + i, score_load(keys[i]));

    /* Only an improvement is kept. */
    score_save("snake", 50);
    CHECK(score_load("snake") == 100, "a lower score overwrote a higher one");
    score_save("snake", 900);
    CHECK(score_load("snake") == 900, "a higher score was not kept");

    /* An unknown key is zero, not whatever happened to be adjacent. */
    CHECK(score_load("never-played") == 0, "an unknown key returned non-zero");
    CHECK(score_load(NULL) == 0, "score_load(NULL) should be 0");

    /* The file survives a round trip through a fresh process image. This is
     * the case that used to lose data: a key with a space ended the read, and
     * the next write then persisted the truncated table over the file. */
    write_file("snake 500\n"
               "Nine Men's Morris 50\n"
               "tetris 9000\n"
               "pacman 7777\n");
    CHECK(line_count() == 4, "setup wrote %d lines, expected 4", line_count());

    /* Re-read from disk by forcing the cache to reload - a separate binary
     * would do the same thing, and this keeps the test to one process. */
    score_reload();
    CHECK(score_load("snake") == 500, "snake lost: got %d", score_load("snake"));
    CHECK(score_load("Nine Men's Morris") == 50,
          "a key with spaces did not round-trip: got %d",
          score_load("Nine Men's Morris"));
    CHECK(score_load("tetris") == 9000,
          "an entry below a spaced key was lost: got %d", score_load("tetris"));
    CHECK(score_load("pacman") == 7777,
          "an entry below a spaced key was lost: got %d", score_load("pacman"));

    /* Saving must not drop anybody else. */
    score_save("snake", 600);
    score_reload();
    CHECK(score_load("tetris") == 9000, "a save destroyed an unrelated score");
    CHECK(score_load("pacman") == 7777, "a save destroyed an unrelated score");
    CHECK(score_load("Nine Men's Morris") == 50, "a save destroyed a spaced key");

    /* A damaged line costs that line, not the rest of the file. */
    write_file("snake 500\n"
               "!!! not a score line !!!\n"
               "\n"
               "no-value-here \n"
               "tetris 9000\n"
               "huge 99999999999999999999\n"
               "pacman 7777\n");
    score_reload();
    CHECK(score_load("tetris") == 9000, "a garbage line discarded what followed");
    CHECK(score_load("pacman") == 7777, "a garbage line discarded what followed");
    CHECK(score_load("huge") == 0, "an out-of-range value was accepted");

    remove(SCORES);
}

/* ----------------------------------------------------------- params ---- */

static void test_params(void)
{
    CHECK(gp_int(7, 3) == 7, "gp_int should prefer the supplied value");
    CHECK(gp_int(0, 3) == 3, "gp_int should fall back when unset");
    CHECK(gp_int(-1, 3) == 3, "gp_int should fall back on a negative");
    CHECK(strcmp(gp_str("a", "b"), "a") == 0, "gp_str should prefer the value");
    CHECK(strcmp(gp_str(NULL, "b"), "b") == 0, "gp_str should fall back on NULL");
    CHECK(strcmp(gp_str("", "b"), "b") == 0, "gp_str should fall back on empty");
    CHECK(family_find("definitely-not-a-family") == NULL,
          "family_find invented a family");
}

int main(void)
{
    printf("engine unit tests\n");
    test_rng();
    test_scores();
    test_params();
    printf("\n%d checks, %d failure(s)\n", checks, failures);
    if (failures) return 1;
    printf("rng stays in range and reproduces, shuffles permute, the score file "
           "round-trips\nkeys with spaces and survives a damaged line\n");
    return 0;
}
