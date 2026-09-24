#include <ncurses.h>

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0}; // 0 = empty, 1 = filled

void draw_board() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 1) {
                mvprintw(y, x * 2, "[]");
            } else {
                mvprintw(y, x * 2, " .");
            }
        }
    }
}

int main() {
    initscr();  // start ncurses mode
    noecho();   // don't show typed characters
    curs_set(0);    //hide the cursor
    keypad(stdscr, TRUE);   //enable arrow key input

    // hardcode one piece into the board for now - a 2x2 square (0-piece)
    board[0][4] = 1;
    board[0][5] = 1;
    board[1][4] = 1;
    board[1][5] = 1;

    draw_board();  
    refresh();   //ncurses doesn't draw until you call this
    getch();    //wait for a keypress
    
    endwin();    //restore normal terminal mode
    return 0;
}


