/* words.h - shared dictionaries for the word games. */
#ifndef GIC_WORDS_H
#define GIC_WORDS_H

#define THEME_COUNT 6

extern const char *WORD5[];
extern const int   WORD5_COUNT;

extern const char *THEME_NAME[THEME_COUNT];
extern const char **THEME_WORDS[THEME_COUNT];
extern const int   THEME_SIZE[THEME_COUNT];

int word5_valid(const char *w);

#endif
