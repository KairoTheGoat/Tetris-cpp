#include <SFML/Graphics.hpp>
#include <random>

const int BOARD_WIDTH = 10; 
const int BOARD_HEIGHT = 20;
const int CELL = 32;

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};

const sf::Color COLORS[] = {
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
	{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},  // I
	{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}, // O
        {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}, // T
        {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}}, // S
        {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}, // Z
        {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}, // J
        {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}, // L
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

std::mt19937 rng{std::random_device{}()};
int random_type() { return std::uniform_int_distribution<int>(0, 6)(rng); }

Piece rotated(const Piece& p) {
    Piece r = p;
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            r.shape[i][3 - j] = p.shape[j][i]; 
    return r;
}

int clear_lines() {
    int cleared = 0;
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; x++)
            if (!board[y][x]) { full = false; break; }
        if (!full) continue;

        for (int yy = y; yy > 0; yy--)
            for (int x = 0; x < BOARD_WIDTH; x++)
                board[yy][x] = board[yy - 1][x];
        for (int x = 0; x < BOARD_WIDTH; x++) board[0][x] = 0;

        cleared++;
        y++; 
    }
    return cleared;
}

void draw_board(sf::RenderWindow& window) {
    sf::RectangleShape cell({CELL - 2.f, CELL - 2.f});
    for (int y = 0; y < BOARD_HEIGHT; y++)
        for (int x = 0; x < BOARD_WIDTH; x++) {
            cell.setPosition({x * float(CELL) + 1, y * float(CELL) + 1});
            cell.setFillColor(COLORS[board[y][x]]);
            window.draw(cell);
        }
}

void draw_piece(sf::RenderWindow& window, const Piece& p) {
    sf::RectangleShape cell({CELL - 2.f, CELL - 2.f});
    cell.setFillColor(COLORS[p.id]);
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            if (p.shape[j][i]) {
                cell.setPosition({(p.x + i) * float(CELL) + 1, (p.y + j) * float(CELL) + 1});
                window.draw(cell);
            }
}

bool collides(const Piece& p) {
	for (int j = 0; j < 4; j++)
		for (int i = 0; i < 4; i++) {
			if (!p.shape[j][i]) continue;
			int bx = p.x + i, by = p.y + j;
			if (bx < 0 || bx >= BOARD_WIDTH || by >= BOARD_HEIGHT) return true;
			if (by >= 0 && board[by][bx]) return true;
		}
	return false;
}

void lock_piece(const Piece& p) {
	for (int j = 0; j < 4; j++)
		for (int i = 0; i < 4; i++)
			if (p.shape[j][i] && p.y + j >= 0)
				board[p.y + j][p.x + i] = p.id;
}

int main() {
    sf::RenderWindow window(sf::VideoMode({BOARD_WIDTH * CELL, BOARD_HEIGHT * CELL}), "Tetris");
    window.setFramerateLimit(60);

    Piece cur = spawn(random_type());
    sf::Clock clock;
    float dropInterval = 0.5f;

auto lock_and_next = [&]() {
	lock_piece(cur);
	cur = spawn(random_type());
	if (collides(cur))  // if the stack reached the top, reset.
		for (auto& row : board)
			for (auto& c : row) c = 0;
};

    auto try_move = [&](int dx, int dy) {
        Piece t = cur; t.x += dx; t.y += dy;
        if (collides(t)) return false;
        cur = t;
        return true;
    };

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                using K = sf::Keyboard::Key;
                if (key->code == K::Left)  try_move(-1, 0);
                if (key->code == K::Right) try_move(1, 0);
                if (key->code == K::Down)  try_move(0, 1);
                if (key->code == K::Up) {
                    Piece r = rotated(cur);
                    for (int kick : {0, -1, 1, -2, 2}) { 
                        Piece t = r; t.x += kick;
                        if (!collides(t)) { cur = t; break; }
                    }
                } 
                if (key->code == K::Space) {
                    while (try_move(0, 1)) {}
                    lock_and_next();
                    clock.restart();
                }
            }
        }

        if (clock.getElapsedTime().asSeconds() >= dropInterval) {
            if (!try_move(0, 1)) lock_and_next();
            clock.restart();
        }

        window.clear(sf::Color(15, 15, 20));
        draw_board(window);
        draw_piece(window, cur);
        window.display();
    }
}

