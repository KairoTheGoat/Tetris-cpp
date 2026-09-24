#include <SFML/Graphics.hpp>

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};

int main() {
    sf::RenderWindow window(sf::VideoMode({400, 800}), "Tetris");

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear(sf::Color::Black);
        window.display();
    }

    return 0;
}