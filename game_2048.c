#include "game.h"

int grid[4][4];
int score = 0;

void init_grid(void) {
memset(grid, 0, sizeof(grid));
score = 0;
}

void add_random_tile(void) {
int empty[16], count = 0;
for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) if (grid[i][j] == 0) empty[count++] = i*4+j;
if (count > 0) {
int idx = rand() % count;
int row = idx / 4, col = idx % 4;
grid[row][col] = (rand() % 10 < 9) ? 2 : 4;
}
}

void print_grid(void) {
printf("\nScore: %d\n", score);
for (int i = 0; i < 4; i++) {
for (int j = 0; j < 4; j++) {
if (grid[i][j] == 0) printf("   . ");
else printf("%4d ", grid[i][j]);
}
printf("\n");
}
printf("\nWASD to move, Q quit\n");
}

int move_left(void) {
int moved = 0;
for (int i = 0; i < 4; i++) {
int write = 0;
for (int read = 0; read < 4; read++) {
if (grid[i][read] != 0) {
if (write > 0 && grid[i][write-1] == grid[i][read]) {
grid[i][write-1] *= 2;
score += grid[i][write-1];
moved = 1;
} else {
grid[i][write++] = grid[i][read];
if (write != read) moved = 1;
}
}
}
for (int j = write; j < 4; j++) grid[i][j] = 0;
}
return moved;
}

 // Similar for right/up/down...

int move_right(void) {
int moved = 0;
for (int i = 0; i < 4; i++) {
int write = 3;
for (int read = 3; read >= 0; read--) {
if (grid[i][read] != 0) {
if (write < 3 && grid[i][write+1] == grid[i][read]) {
grid[i][write+1] *= 2;
score += grid[i][write+1];
moved = 1;
write++;
} else {
grid[i][write--] = grid[i][read];
if (write != read) moved = 1;
}
}
}
for (int j = write - 1; j >= 0; j--) grid[i][j] = 0;
}
return moved;
}

void play_2048(void) {
init_grid();
add_random_tile();
add_random_tile();
char dir;
while (1) {
clear_screen();
print_header("2048");
print_grid();
print_footer("Move: ");
dir = toupper(GETCH());
if (dir == 'Q') break;
int moved = 0;
switch (dir) {
case 'A': 
for (int j = 0; j < 4; j++) for (int i = 1; i < 4; i++) if (grid[i][j] != 0) grid[i-1][j] = grid[i][j];  // Stub
break;
case 'D':
move_left();  // Stub for down etc
break;
case 'W': move_right(); break;
case 'S': move_right(); break;  // Rotate for simplicity
}
if (moved) add_random_tile();
}
}

