/* main.c - the terminal front end: a searchable browser over all 1000
 * catalog entries, launching the 49 that are implemented.
 */
#include "engine.h"

#define PAGE 18

static int entry_playable(const CatalogEntry *e);

static char   filter[64];
static char   genre_filter[24];
static int    only_playable;
static int    view[1024];      /* catalog indices matching the filters */
static int    view_n;

static int ci_contains(const char *hay, const char *needle)
{
    size_t nl = strlen(needle), i;
    size_t hl = strlen(hay);
    if (!nl) return 1;
    if (nl > hl) return 0;
    for (i = 0; i + nl <= hl; i++) {
        size_t j;
        for (j = 0; j < nl; j++)
            if (tolower((unsigned char)hay[i + j]) != tolower((unsigned char)needle[j])) break;
        if (j == nl) return 1;
    }
    return 0;
}

static void rebuild_view(void)
{
    int i;
    view_n = 0;
    for (i = 0; i < CATALOG_COUNT && view_n < 1024; i++) {
        const CatalogEntry *e = &CATALOG[i];
        if (only_playable && !entry_playable(e)) continue;
        if (genre_filter[0] && strcmp(e->genre, genre_filter) != 0) continue;
        if (filter[0] &&
            !ci_contains(e->title, filter) &&
            !ci_contains(e->genre, filter) &&
            !ci_contains(e->mechanic, filter) &&
            !ci_contains(e->blurb, filter)) continue;
        view[view_n++] = i;
    }
}

/* Playable means two things: the engine exists, and it actually honours the
 * parameters this entry specifies. The generator works the second part out
 * and records it, so a single hard-coded engine cannot claim credit for
 * variants it would silently ignore. */
static int entry_playable(const CatalogEntry *e)
{
    return e->playable && family_find(e->family) != NULL;
}

static void launch(const CatalogEntry *e)
{
    const FamilyEntry *f = family_find(e->family);
    if (!f) return;
    /* A pinned seed makes the entry reproducible: every player opening
     * "Daily Sudoku 3" gets the same grid, which is what distinguishes it
     * from the ordinary 9x9 entry it otherwise shares every parameter with. */
    if (e->params.seed) rng_seed((unsigned)e->params.seed);
    f->run(&e->params);
}

static void show_detail(const CatalogEntry *e)
{
    int i;
    scr_clear();
    draw_title(e->title, e->genre);
    draw_textf(7,  10, "%sMechanic%s    %s", C_GREY, C_RESET, e->mechanic);
    draw_textf(8,  10, "%sPlayers%s     %s", C_GREY, C_RESET, e->players);
    scr_move(9, 10);
    printf("%sDifficulty%s  ", C_GREY, C_RESET);
    for (i = 0; i < 5; i++)
        printf("%s%s%s", i < e->difficulty ? C_YELLOW : C_GREY, "●", C_RESET);
    draw_textf(10, 10, "%sSlug%s        %s", C_GREY, C_RESET, e->slug);
    draw_textf(11, 10, "%sEngine%s      %s", C_GREY, C_RESET, e->family);
    draw_textf(12, 10, "%s", e->blurb);

    if (entry_playable(e)) {
        int best = score_load(e->slug);
        draw_textf(14, 10, "%sStatus%s      %sPlayable%s", C_GREY, C_RESET, C_GREEN, C_RESET);
        if (best) draw_textf(15, 10, "%sBest score%s  %d", C_GREY, C_RESET, best);
        draw_text(18, 10, "Press Enter to play, or any other key to go back.");
    } else {
        draw_textf(14, 10, "%sStatus%s      %sAwaiting the %s engine%s",
                   C_GREY, C_RESET, C_YELLOW, e->family, C_RESET);
        draw_text(16, 10, "This entry is fully specified and already bound to its engine");
        draw_text(17, 10, "and parameters. It becomes playable the moment that engine lands.");
        draw_text(19, 10, "Press any key to go back.");
    }
    scr_flush();
}

/* How many catalogue entries currently have a parameter-aware engine. */
static int playable_total(void)
{
    static int cached = -1;
    int i;
    if (cached >= 0) return cached;
    cached = 0;
    for (i = 0; i < CATALOG_COUNT; i++)
        if (entry_playable(&CATALOG[i])) cached++;
    return cached;
}

static void draw_menu(int sel, int top)
{
    int i, rows, cols;
    char sub[160];
    scr_size(&rows, &cols);
    scr_clear();

    snprintf(sub, sizeof sub, "%d of %d shown  |  %d playable%s%s%s%s",
             view_n, CATALOG_COUNT, playable_total(),
             genre_filter[0] ? "   genre: " : "", genre_filter,
             filter[0] ? "   search: " : "", filter);
    draw_title("GAMES IN C", sub);

    for (i = 0; i < PAGE && top + i < view_n; i++) {
        const CatalogEntry *e = &CATALOG[view[top + i]];
        int row = 6 + i;
        int is_sel = (top + i == sel);
        scr_move(row, 4);
        {
            int ok = entry_playable(e);
            printf("%s%s%4d %s%-28.28s %s%-11.11s %s%-24.24s %s%s%s",
                   is_sel ? C_REV : "",
                   ok ? C_GREEN : C_GREY,
                   top + i + 1,
                   ok ? C_BOLD C_WHITE : C_GREY, e->title,
                   C_CYAN, e->genre,
                   C_GREY, e->mechanic,
                   ok ? C_GREEN : C_GREY,
                   ok ? "PLAY" : "soon",
                   C_RESET);
        }
    }
    for (; i < PAGE; i++) { scr_move(6 + i, 4); printf("\033[2K"); }

    draw_hline(6 + PAGE, 2, cols - 2, C_GREY);
    draw_textf(7 + PAGE, 4,
        "%sUp/Down%s move  %sPgUp/PgDn%s page  %sEnter%s open  %s/%s search  "
        "%sG%s genre  %sP%s playable-only  %sQ%s quit",
        C_BOLD, C_RESET, C_BOLD, C_RESET, C_BOLD, C_RESET, C_BOLD, C_RESET,
        C_BOLD, C_RESET, C_BOLD, C_RESET, C_BOLD, C_RESET);
    if (only_playable)
        draw_textf(8 + PAGE, 4, "%sShowing playable games only.%s", C_YELLOW, C_RESET);
    scr_flush();
}

static void pick_genre(void)
{
    const char *genres[32];
    int ng = 0, i, sel = 0;

    for (i = 0; i < CATALOG_COUNT && ng < 31; i++) {
        int j, dup = 0;
        for (j = 0; j < ng; j++) if (strcmp(genres[j], CATALOG[i].genre) == 0) dup = 1;
        if (!dup) genres[ng++] = CATALOG[i].genre;
    }

    for (;;) {
        int k;
        scr_clear();
        draw_title("FILTER BY GENRE", "Up/Down to choose, Enter to apply, C to clear, Q to cancel");
        for (i = 0; i < ng; i++) {
            int count = 0, j;
            for (j = 0; j < CATALOG_COUNT; j++)
                if (strcmp(CATALOG[j].genre, genres[i]) == 0) count++;
            draw_textf(6 + i, 28, "%s%-14s %s%4d games%s",
                       i == sel ? C_REV : "", genres[i], C_GREY, count, C_RESET);
        }
        scr_flush();
        k = key_get();
        if (k == 'q' || k == 'Q' || k == KEY_ESC) return;
        if (k == 'c' || k == 'C') { genre_filter[0] = '\0'; rebuild_view(); return; }
        if (k == KEY_UP)   sel = (sel + ng - 1) % ng;
        if (k == KEY_DOWN) sel = (sel + 1) % ng;
        if (k == KEY_ENTER || k == ' ') {
            strncpy(genre_filter, genres[sel], sizeof genre_filter - 1);
            genre_filter[sizeof genre_filter - 1] = '\0';
            rebuild_view();
            return;
        }
    }
}

int main(int argc, char **argv)
{
    int sel = 0, top = 0;

    /* Seeding from the clock means every run inside the same second deals the
     * same cards and builds the same maze, so a test suite that launches a
     * thousand games in a few seconds was re-testing a handful of layouts.
     * GIC_SEED pins it instead, which lets the stress harness sweep seeds and
     * lets any failure it finds be reproduced exactly. */
    {
        const char *seed = getenv("GIC_SEED");
        rng_seed(seed && *seed ? (unsigned)strtoul(seed, NULL, 10)
                               : (unsigned)time(NULL));
    }
    scr_init();
    atexit(scr_shutdown);

    /* Direct launch: ./games <slug> */
    if (argc > 1) {
        int i;
        for (i = 0; i < CATALOG_COUNT; i++) {
            if (strcmp(CATALOG[i].slug, argv[1]) != 0) continue;
            if (!entry_playable(&CATALOG[i])) {
                scr_shutdown();
                fprintf(stderr, "'%s' is catalogued but its %s engine is not built yet.\n",
                        argv[1], CATALOG[i].family);
                return 2;
            }
            launch(&CATALOG[i]);
            scr_shutdown();
            scr_clear();
            return 0;
        }
        scr_shutdown();
        fprintf(stderr, "No catalogue entry with slug '%s'.\n", argv[1]);
        fprintf(stderr, "Run without arguments to browse the catalogue.\n");
        return 1;
    }

    filter[0] = genre_filter[0] = '\0';
    only_playable = 0;
    rebuild_view();

    for (;;) {
        int k;
        if (sel >= view_n) sel = view_n ? view_n - 1 : 0;
        if (sel < 0) sel = 0;
        if (sel < top) top = sel;
        if (sel >= top + PAGE) top = sel - PAGE + 1;
        if (top < 0) top = 0;

        draw_menu(sel, top);
        k = key_get();

        if (k == 'q' || k == 'Q' || k == KEY_ESC) break;
        if (k == KEY_UP)    sel--;
        if (k == KEY_DOWN)  sel++;
        if (k == KEY_PGUP)  sel -= PAGE;
        if (k == KEY_PGDN)  sel += PAGE;
        if (k == KEY_HOME)  sel = 0;
        if (k == KEY_END)   sel = view_n - 1;
        if (k == 'p' || k == 'P') { only_playable = !only_playable; rebuild_view(); sel = 0; top = 0; }
        if (k == 'g' || k == 'G') { pick_genre(); sel = 0; top = 0; }
        if (k == '/') {
            int rows, cols;
            scr_size(&rows, &cols);
            scr_move(rows - 1, 1);
            printf("\033[2K%sSearch: %s", C_CYAN, C_RESET);
            scr_flush();
            read_line(filter, sizeof filter);
            rebuild_view();
            sel = 0; top = 0;
        }
        if ((k == KEY_ENTER || k == ' ') && view_n > 0) {
            const CatalogEntry *e = &CATALOG[view[sel]];
            show_detail(e);
            {
                int k2 = key_get();
                if (entry_playable(e) && (k2 == KEY_ENTER || k2 == ' ')) {
                    launch(e);
                    key_flush();
                }
            }
        }
    }

    scr_shutdown();
    scr_clear();
    printf("%sThanks for playing.%s\n", C_GREEN, C_RESET);
    return 0;
}
