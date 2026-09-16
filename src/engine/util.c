/* util.c - RNG and persistent high scores. */
#include "engine.h"
#include <limits.h>

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
/* Flat "slug score" text file kept beside the binary, held in memory while
 * the process runs.
 *
 * The table is not an optimisation for its own sake. Four games drew the
 * standing best inside their render loop, so every frame opened the file,
 * parsed it and closed it again - which for the real-time ones meant a disk
 * round trip sixty times a second, and made an 8x8 merge board spend about
 * two milliseconds on a keystroke that does nothing. Reading once and writing
 * through removes that entirely, and any future caller gets it for free. */

#define SCORE_FILE  ".gic_scores"
#define SCORE_MAX   512
#define SLUG_MAX     64

static char  sc_name[SCORE_MAX][SLUG_MAX];
static int   sc_val[SCORE_MAX];
static int   sc_count;
static int   sc_loaded;

/* Read the score file.
 *
 * Parsed a line at a time, splitting on the *last* space, because the key is
 * whatever the caller passed and almost every caller passes the entry's title:
 * "Nine Men's Morris", not "nine-mens-morris". The previous "%63s %d" read the
 * key as a single whitespace-delimited token, which broke that in two ways at
 * once. The game never got its score back, because "Nine" never matched the
 * key it asked for. Worse, fscanf returning 1 instead of 2 ended the loop, so
 * every entry *below* the first spaced name was dropped - and the next save
 * rewrote the file from the truncated cache, destroying them. One game with a
 * space in its title silently wiped every score recorded after it.
 *
 * A malformed line is now skipped rather than ending the read, so a file that
 * has been hand-edited or half-written costs at most the lines that are
 * actually broken. The format on disk is unchanged, so existing files load. */
static void score_cache_init(void)
{
    FILE *f;
    char line[SLUG_MAX + 32];

    if (sc_loaded) return;
    sc_loaded = 1;
    sc_count = 0;
    f = fopen(SCORE_FILE, "r");
    if (!f) return;

    while (sc_count < SCORE_MAX && fgets(line, (int)sizeof line, f)) {
        char *sp, *end;
        long v;
        size_t n = strlen(line);

        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = '\0';
        if (n == 0) continue;

        sp = strrchr(line, ' ');
        if (!sp || !sp[1]) continue;              /* no value on this line   */

        v = strtol(sp + 1, &end, 10);
        if (*end != '\0') continue;               /* value is not a number   */
        if (v < INT_MIN || v > INT_MAX) continue; /* and has to fit          */

        *sp = '\0';
        if (sp == line || strlen(line) >= SLUG_MAX) continue;  /* no/long key */

        strcpy(sc_name[sc_count], line);
        sc_val[sc_count] = (int)v;
        sc_count++;
    }
    fclose(f);
}

static void score_cache_write(void)
{
    FILE *f = fopen(SCORE_FILE, "w");
    int i;
    if (!f) return;                 /* a read-only directory is not fatal */
    for (i = 0; i < sc_count; i++) fprintf(f, "%s %d\n", sc_name[i], sc_val[i]);
    fclose(f);
}

/* Forget the in-memory table, so the next read comes from disk.
 *
 * The table is loaded once and written through, which is what made the render
 * loops cheap. That assumes this process is the only writer - fine while a
 * player runs one game at a time, and not true if a second copy is open in
 * another terminal, or if the file is edited by hand. Callers that care can
 * ask for a fresh read; the unit tests use it to check what actually reached
 * the disk rather than what the cache remembers. */
void score_reload(void)
{
    sc_loaded = 0;
    sc_count = 0;
}

int score_load(const char *slug)
{
    int i, best = 0;
    if (!slug) return 0;
    score_cache_init();
    for (i = 0; i < sc_count; i++)
        if (strcmp(sc_name[i], slug) == 0 && sc_val[i] > best) best = sc_val[i];
    return best;
}

void score_save(const char *slug, int score)
{
    int i;
    if (!slug) return;
    score_cache_init();
    for (i = 0; i < sc_count; i++)
        if (strcmp(sc_name[i], slug) == 0) {
            if (score <= sc_val[i]) return;   /* nothing new to persist */
            sc_val[i] = score;
            score_cache_write();
            return;
        }
    if (sc_count >= SCORE_MAX) return;
    strncpy(sc_name[sc_count], slug, SLUG_MAX - 1);
    sc_name[sc_count][SLUG_MAX - 1] = '\0';
    sc_val[sc_count++] = score;
    score_cache_write();
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
