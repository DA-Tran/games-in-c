/* util.c - RNG and persistent high scores. */
#include "engine.h"

/* xorshift32: small, fast, and reproducible across platforms. */
static unsigned g_state = 2463534242u;

void rng_seed(unsigned s)
{
    g_state = s ? s : 2463534242u;
}

static unsigned xorshift(void)
{
    g_state ^= g_state << 13;
    g_state ^= g_state >> 17;
    g_state ^= g_state << 5;
    return g_state;
}

int rnd(int n)
{
    if (n <= 0) return 0;
    return (int)(xorshift() % (unsigned)n);
}

int rnd_range(int lo, int hi)
{
    if (hi < lo) { int t = lo; lo = hi; hi = t; }
    return lo + rnd(hi - lo + 1);
}

double rnd_f(void)
{
    return (double)(xorshift() >> 8) / 16777216.0;
}

void shuffle_int(int *a, int n)
{
    int i;
    for (i = n - 1; i > 0; i--) {
        int j = rnd(i + 1);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

/* --------------------------------------------------------- high scores */
/* Flat "slug score" text file kept beside the binary. */

#define SCORE_FILE ".gic_scores"

int score_load(const char *slug)
{
    FILE *f = fopen(SCORE_FILE, "r");
    char name[64];
    int  val, best = 0;
    if (!f) return 0;
    while (fscanf(f, "%63s %d", name, &val) == 2)
        if (strcmp(name, slug) == 0 && val > best) best = val;
    fclose(f);
    return best;
}

void score_save(const char *slug, int score)
{
    char names[512][64];
    int  vals[512];
    int  count = 0, i, found = 0;
    FILE *f = fopen(SCORE_FILE, "r");

    if (f) {
        while (count < 512 && fscanf(f, "%63s %d", names[count], &vals[count]) == 2)
            count++;
        fclose(f);
    }
    for (i = 0; i < count; i++) {
        if (strcmp(names[i], slug) == 0) {
            found = 1;
            if (score > vals[i]) vals[i] = score;
        }
    }
    if (!found && count < 512) {
        strncpy(names[count], slug, 63);
        names[count][63] = '\0';
        vals[count++] = score;
    }
    f = fopen(SCORE_FILE, "w");
    if (!f) return;
    for (i = 0; i < count; i++) fprintf(f, "%s %d\n", names[i], vals[i]);
    fclose(f);
}

/* Print "new best" or the standing record, then persist. */
void score_report(const char *slug, int score)
{
    int best = score_load(slug);
    int rows, cols;
    char line[128];
    scr_size(&rows, &cols);

    if (score > best) {
        snprintf(line, sizeof line, "%s%s NEW BEST: %d %s", C_BOLD, C_YELLOW, score, C_RESET);
        score_save(slug, score);
    } else {
        snprintf(line, sizeof line, "%sScore %d   (best %d)%s", C_GREY, score, best, C_RESET);
    }
    draw_centered(rows - 3, cols, line);
    scr_flush();
}

const FamilyEntry *family_find(const char *name)
{
    int i;
    for (i = 0; i < FAMILY_REGISTRY_COUNT; i++)
        if (strcmp(FAMILY_REGISTRY[i].family, name) == 0) return &FAMILY_REGISTRY[i];
    return NULL;
}

int gp_int(int value, int fallback) { return value > 0 ? value : fallback; }

const char *gp_str(const char *value, const char *fallback)
{
    return (value && *value) ? value : fallback;
}
