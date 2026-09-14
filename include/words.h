/* words.h - shared dictionaries for the word families. */
#ifndef GIC_WORDS_H
#define GIC_WORDS_H

#define THEME_COUNT 21
#define MINLEN 4
#define MAXLEN 8

/* Themed dictionaries, indexed by theme id (see THEME_NAME). */
extern const char *THEME_NAME[THEME_COUNT];
extern const char **THEME_WORDS[THEME_COUNT];
extern const int   THEME_SIZE[THEME_COUNT];

/* Guessable words grouped by length, for Wordle-style play. */
extern const char **WORDS_BY_LEN[MAXLEN + 1];
extern const int    WORDS_BY_LEN_COUNT[MAXLEN + 1];

/* Typing-drill passages, grouped by drill mode. */
#define TYPING_MODES 9
extern const char *TYPING_MODE_NAME[TYPING_MODES];
extern const char **TYPING_TEXT[TYPING_MODES];
extern const int    TYPING_TEXT_COUNT[TYPING_MODES];

int         theme_index(const char *name);
int         word_valid(const char *w, int len);
const char *theme_pick(int theme);

#endif
