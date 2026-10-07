#include <SFML/Graphics.hpp>

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int CELL = 32;

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0}; 
const sf::Color COLORS[] ={
	sf::Color(30, 30, 40),
	sf::Color(0, 200, 255),
	sf::Color(255, 220, 0),
	sf::Color(170, 0, 255),
	sf::Color(0, 220, 80),
	sf::Color(255, 50, 50),    
        sf::Color(40, 80, 255),            
	sf::Color(255, 140, 0),    
};

const int SHAPES[7][4][4] = {
	{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, // I
	{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}, 
	{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
	{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
	{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
	{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
	{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
};

struct Piece {
	int shape[4][4];
	int id;
	int x, y;
};

Piece spawn(int type) {
	Piece p;
	for (int j = 0; j < 4; j++)
		for (int i = 0; i < 4; i++)
			p.shape[j][i] = SHAPES[type][j][i];
	p.id = type + 1;
	p.x = 3;
	p.y = 0;
	return p;
}

void draw_board(sf::RenderWindow& window) {
    sf::RectangleShape cell({CELL - 2.f, CELL - 2.f});

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            cell.setPosition({x * float(CELL) + 1, y * float(CELL) + 1});
            cell.setFillColor(COLORS[board[y][x]]);

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
