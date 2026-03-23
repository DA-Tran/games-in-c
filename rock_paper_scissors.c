#include "game.h"

int get_player_choice(void) {
int choice;
while (1) {
clear_screen();
print_header("Rock Paper Scissors");
printf("1. Rock\n2. Paper\n3. Scissors\n0. Quit\n");
print_footer("Choice: ");
scanf("%d", &choice);
if (choice >= 0 && choice <= 3) return choice;
}
}

int get_computer_choice(void) {
return rand() % 3 + 1;
}

const char *get_name(int choice) {
switch (choice) {
case 1: return "Rock";
case 2: return "Paper";
case 3: return "Scissors";
}
return "??";
}

int get_winner(int p, int c) {
if (p == c) return 0;  // tie
if ((p == 1 && c == 3) || (p == 2 && c == 1) || (p == 3 && c == 2)) return 1;  // player wins
return 2;  // computer wins
}

void play_rps(void) {
srand(time(NULL));
int score_p = 0, score_c = 0;
int choice;
printf(ANSI_YELLOW "Rock Paper Scissors - Best of infinite!\n" ANSI_RESET);

while (1) {
int p = get_player_choice();
if (p == 0) break;
int c = get_computer_choice();
int winner = get_winner(p, c);
clear_screen();
print_header("Rock Paper Scissors");
printf("You: %s\n", get_name(p));
printf("Computer: %s\n", get_name(c));
if (winner == 1) {
printf(ANSI_GREEN "You WIN round!\n" ANSI_RESET);
score_p++;
} else if (winner == 2) {
printf(ANSI_RED "Computer wins round!\n" ANSI_RESET);
score_c++;
} else {
printf("Tie!\n");
}
printf("Score: You %d - Computer %d\n\n", score_p, score_c);
print_footer("Enter to continue, Q quit: ");
char ch = toupper(GETCH());
if (ch == 'Q') break;
}
}

