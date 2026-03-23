#include "game.h"

int main() {
int choice;
while (1) {
clear_screen();
print_header("C GAMES MENU");
printf(ANSI_BOLD ANSI_BLUE " === SELECT GAME === " ANSI_RESET "\n\n");
printf("  1. Pacman\n");
printf("  2. Hangman\n");
printf("  3. Rock Paper Scissors\n");
printf("  4. Snakes and Ladders\n");
printf("  5. Dino Game\n");
printf("  6. Virtual Piano\n");
printf("  7. Snake Game\n");
printf("  8. 2048 Game\n");
printf("  0. Exit\n\n");
print_footer("Enter choice (0-8): ");
scanf("%d", &choice);
if (choice == 0) break;
switch (choice) {
case 1: play_pacman(); break;
case 2: play_hangman(); break;
case 3: play_rps(); break;
case 4: play_snakes_ladders(); break;
case 5: play_dino(); break;
case 6: play_piano(); break;
case 7: play_snake(); break;
case 8: play_2048(); break;
default: printf("Invalid choice!\n"); sleep_ms(1000); break;
}
}
clear_screen();
printf(ANSI_GREEN "Thanks for playing!" ANSI_RESET "\n");
return 0;
}

