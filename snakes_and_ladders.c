#include "game.h"

// Simple 10x10 board, snakes/ladders positions
int board[100] = {0};  // 1-100, negative snake, positive ladder

void init_board(void) {
 // Simple ladders
board[3] = 22;
board[5] = 14;
board[8] = 30;
board[21] = 43;
board[28] = 85;
board[36] = 45;
board[54] = 79;
board[62] = 97;
board[72] = 91;
board[88] = 100;
 // Snakes
board[17] = -7;
board[32] = -7;
board[48] = -20;
board[64] = -25;
board[81] = -40;
board[95] = -75;
board[98] = -80;
}

int get_pos(int pos) {
if (pos < 1) pos = 1;
if (pos > 100) pos = 100;
if (board[pos] != 0) {
printf("Chute/Ladder to %d!\n", board[pos]);
sleep_ms(1000);
pos = board[pos];
if (pos < 1) pos = 1;
}
return pos;
}

void print_board(int p1_pos, int p2_pos) {
printf("Snakes & Ladders - Pos P1:%d P2:%d\n", p1_pos, p2_pos);
for (int row = 10; row >= 1; row--) {
for (int col = 1; col <= 10; col++) {
int sq = (row-1)*10 + col;
if (sq == p1_pos) printf(ANSI_YELLOW "P1 " ANSI_RESET);
else if (sq == p2_pos) printf(ANSI_YELLOW "P2 " ANSI_RESET);
else printf("%2d ", sq);
}
printf("\n");
}
}

void play_snakes_ladders(void) {
init_board();
int p1_pos = 0, p2_pos = 0;
char player = '1';
int roll;
while (p1_pos < 100 && p2_pos < 100) {
clear_screen();
print_header("Snakes & Ladders");
print_board(p1_pos, p2_pos);
printf("\nPlayer %c turn. Roll dice (1-6 or r): ", player);
fflush(stdout);
char ch = GETCH();
roll = ch >= '1' && ch <= '6' ? ch - '0' : (rand() % 6 + 1);
int pos = (player == '1') ? p1_pos : p2_pos;
int *p_pos = (player == '1') ? &p1_pos : &p2_pos;
*p_pos = pos;

pos += roll;
pos = get_pos(pos);
printf("\nRolled %d, new pos %d\n", roll, pos);
print_footer("Enter to next player");
GETCH();
player = (player == '1') ? '2' : '1';
sleep_ms(500);
}
clear_screen();
print_header("Snakes & Ladders");
printf("Player %c wins!\n", p1_pos >= 100 ? '1' : '2');
print_footer("Enter to return");
GETCH();
}

