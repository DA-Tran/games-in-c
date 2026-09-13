/* cards.h - shared playing-card helpers.
 * A card is 0..51: suit = card / 13 (0 spades, 1 hearts, 2 diamonds, 3 clubs),
 * rank = card % 13 (0 = ace, 12 = king).
 */
#ifndef GIC_CARDS_H
#define GIC_CARDS_H

#include "engine.h"

extern const char *RANK_NAME[13];
extern const char *SUIT_SYM[4];

const char *suit_colour(int card);
void card_name(int card, char *buf, int n);
void deck_init(int *deck);
void draw_card(int row, int col, int card, int face_up);

#endif
