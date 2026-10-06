#include <SFML/Graphics.hpp>

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int CELL = 32;

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0}; 

const

void draw_board(sf::RenderWindow& window) {
    sf::RectangleShape cell({CELL - 2.f, CELL - 2.f});

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            cell.setPosition({x * float(CELL) + 1, y * float(CELL) + 1});
            cell.setFillColor(board[y][x] ? sf::Color(0, 200, 255)
                                          : sf::Color(30, 30, 40));
            window.draw(cell);
        }
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode({BOARD_WIDTH * CELL, BOARD_HEIGHT * CELL}), "Tetris");
    window.setFramerateLimit(60);

    // hardcoded 2x2 square for now
    board[0][4] = 1;
    board[0][5] = 1;
    board[1][4] = 1;
    board[1][5] = 1;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        window.clear(sf::Color(15, 15, 20));
        draw_board(window);
        window.display();
    }
}
