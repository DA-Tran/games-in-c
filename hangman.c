#include "game.h"

static const char *words[] = {"PROGRAMMING", "COMPUTER", "ALGORITHM", "HANGMAN", "KEYBOARD", "FUNCTION", "LOOP"};
static const int NUM_WORDS = 7;

static const char *hangman_parts[7] = {
"  +---+\n  |   |\n      |\n      |\n      |\n      |",
"  +---+\n  |   |\n  O   |\n      |\n      |\n      |",
"  +---+\n  |   |\n  O   |\n /    |\n      |\n      |",
"  +---+\n  |   |\n  O   |\n /|   |\n      |\n      |",
"  +---+\n  |   |\n  O   |\n /|\\  |\n      |\n      |",
"  +---+\n  |   |\n  O   |\n /|\\  |\n /    |\n      |",
"  +---+\n  |   |\n  O   |\n /|\\  |\n / \\  |\n      |"
};

void play_hangman(void) {
srand(time(NULL));
int idx = rand() % NUM_WORDS;
char word[50];
strcpy(word, words[idx]);
int len = strlen(word);
char display[50];
memset(display, '_', len);
display[len] = 0;
int errors = 0;
bool letter_used[26] = {false};
bool finished = false;

while (!finished && errors < 7) {
clear_screen();
print_header("Hangman");
printf(ANSI_RED "%s\n\n" ANSI_RESET, hangman_parts[errors]);
printf("Word: %s\n", display);
printf("Errors: %d/6\n", errors);
printf("Used: ");
for (int i = 0; i < 26; i++) if (letter_used[i]) printf("%c ", 'A' + i);
printf("\n\n");
print_footer("Guess letter (A-Z) or Q quit: ");
fflush(stdout);
char guess = toupper(GETCH());
if (guess == 'Q') break;
int pos = guess - 'A';
if (pos < 0 || pos > 25 || letter_used[pos]) {
sleep_ms(500);
continue;
}
letter_used[pos] = true;
bool found = false;
for (int i = 0; i < len; i++) {
if (word[i] == guess) {
display[i] = guess;
found = true;
}
}
if (!found) errors++;

if (strspn(display, "_") == 0) {
clear_screen();
print_header("Hangman");
printf(ANSI_GREEN "WIN! Word: %s\n" ANSI_RESET, word);
print_footer("Enter to return");
GETCH();
finished = true;
}
}
if (errors >= 7) {
clear_screen();
print_header("Hangman");
printf(ANSI_RED "LOSE! Word: %s\n" ANSI_RESET, word);
print_footer("Enter to return");
GETCH();
}
}

