#include "game.h"

#define WIDTH 20
#define HEIGHT 10

int game_over;
int score;
int tailx[100], taily[100];
int tail_length;
int x, y;
int fruitx, fruity;
int dir_x, dir_y;

void init_game(void) {
game_over = 0;
score = 0;
tail_length = 0;
x = WIDTH / 2;
y = HEIGHT / 2;
fruitx = rand() % WIDTH;
fruity = rand() % HEIGHT;
dir_x = 1;
dir_y = 0;
}

void print_snake(void) {
clear_screen();
print_header("Snake Game");
for (int i = 0; i < HEIGHT; i++) {
for (int j = 0; j < WIDTH; j++) {
if (i == y && j == x) printf(ANSI_GREEN "O " ANSI_RESET);
else if (i == fruity && j == fruitx) printf(ANSI_RED "F " ANSI_RESET);
else printf(". ");
}
printf("\n");
}
printf("Score: %d\n", score);
printf("WASD arrow, Q quit\n");
}

void update_snake(void) {
tailx[tail_length] = x;
taily[tail_length] = y;
tail_length++;
for (int i = tail_length - 1; i > 0; i--) {
tailx[i] = tailx[i-1];
taily[i] = taily[i-1];
}
tailx[0] = x;
taily[0] = y;
tail_length--;

if (x == fruitx && y == fruity) {
score += 10;
fruitx = rand() % WIDTH;
fruity = rand() % HEIGHT;
}
x += dir_x;
y += dir_y;
if (x >= WIDTH || x < 0 || y >= HEIGHT || y < 0) game_over = 1;
// Tail collision stub
}

void play_snake(void) {
init_game();
char input;
while (!game_over) {
print_snake();
print_footer("Input: ");
input = toupper(GETCH());
switch (input) {
case 'A': dir_x = -1; dir_y = 0; break;
case 'D': dir_x = 1; dir_y = 0; break;
case 'W': dir_x = 0; dir_y = -1; break;
case 'S': dir_x = 0; dir_y = 1; break;
case 'Q': game_over = 1; break;
}
update_snake();
sleep_ms(200);
}
clear_screen();
printf("Game Over! Score: %d\n", score);
print_footer("Enter to return");
GETCH();
}

