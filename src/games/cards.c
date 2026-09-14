/* cards.c - shared 52-card deck helpers used by the card games. */
#include "engine.h"
#include "cards.h"

const char *RANK_NAME[13] = {"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
const char *SUIT_SYM[4]   = {"♠","♥","♦","♣"};

const char *suit_colour(int card)
{
    int s = card / 13;
    return (s == 1 || s == 2) ? C_RED : C_WHITE;
}

void card_name(int card, char *buf, int n)
{
    snprintf(buf, n, "%s%s", RANK_NAME[card % 13], SUIT_SYM[card / 13]);
}

void deck_init(int *deck)
{
    int i;
    for (i = 0; i < 52; i++) deck[i] = i;
    shuffle_int(deck, 52);
}

void draw_card(int row, int col, int card, int face_up)
{
    char nm[16];
    if (face_up) card_name(card, nm, sizeof nm);
    draw_textf(row,     col, "%s┌────┐%s", C_GREY, C_RESET);
    if (face_up) draw_textf(row + 1, col, "%s│%s%-4s%s│%s", C_GREY, suit_colour(card), nm, C_GREY, C_RESET);
    else         draw_textf(row + 1, col, "%s│▒▒▒▒│%s", C_BLUE, C_RESET);
    draw_textf(row + 2, col, "%s└────┘%s", C_GREY, C_RESET);
}
