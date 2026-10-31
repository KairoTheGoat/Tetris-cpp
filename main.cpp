#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <random>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int CELL = 32;
const int PANEL = 160;

const float BW = BOARD_WIDTH * CELL;   // board width  = 320
const float LW = BW + PANEL;           // full width   = 480
const float LH = BOARD_HEIGHT * CELL;  // full height  = 640

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
float flashT[BOARD_HEIGHT][BOARD_WIDTH] = {0};

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
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, // I
    {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}, // O
    {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}, // T
    {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}}, // S
    {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}, // Z
    {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}, // J
    {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}, // L
};

const int TILE = 8;

sf::Texture make_atlas() {
    sf::Image img(sf::Vector2u(TILE * 8, TILE), sf::Color::Black);

    for (unsigned id = 0; id < 8; id++) {
        sf::Color base = COLORS[id];
        sf::Color dark(base.r / 2, base.g / 2, base.b / 2);

        for (unsigned y = 0; y < TILE; y++) {
            for (unsigned x = 0; x < TILE; x++) {
                sf::Color c = base;
                if (id != 0) {
                    if (x == TILE - 1 || y == TILE - 1) c = dark;
                    if ((x == 1 && (y == 1 || y == 2)) || (y == 1 && x == 2))
                        c = sf::Color::White;
                }
                img.setPixel(sf::Vector2u(id * TILE + x, y), c);
            }
        }
    }
    return sf::Texture(img);
}

void draw_block(sf::RenderWindow& window, const sf::Texture& tex, int id,
                float px, float py, float size, std::uint8_t alpha = 255) {
    sf::Sprite s(tex);
    s.setTextureRect(sf::IntRect({id * TILE, 0}, {TILE, TILE}));
    s.setScale({(size - 2) / float(TILE), (size - 2) / float(TILE)});
    s.setPosition({px + 1, py + 1});
    s.setColor(sf::Color(255, 255, 255, alpha));
    window.draw(s);
}

void draw_board(sf::RenderWindow& window, const sf::Texture& tex) {
    sf::RectangleShape glow({CELL - 2.f, CELL - 2.f});
    for (int y = 0; y < BOARD_HEIGHT; y++)
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int id = board[y][x];
            float px = x * float(CELL), py = y * float(CELL);
            draw_block(window, tex, id, px, py, CELL, id ? 255 : 190);
            if (flashT[y][x] > 0.f) {
                glow.setPosition({px + 1, py + 1});
                glow.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(200 * flashT[y][x])));
                window.draw(glow);
            }
        }
}

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
float frand(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); }

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

void draw_piece(sf::RenderWindow& window, const sf::Texture& tex, const Piece& p,
                std::uint8_t alpha = 255) {
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            if (p.shape[j][i])
                draw_block(window, tex, p.id, (p.x + i) * float(CELL), (p.y + j) * float(CELL), CELL, alpha);
}

bool collides(const Piece& p) {
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++) {
            if (!p.shape[j][i]) continue;
            int bx = p.x + i, by = p.y + j;
            if (bx < 0 || bx >= BOARD_WIDTH || by >= BOARD_HEIGHT) return true;
            if (board[by][bx]) return true;
        }
    return false;
}

void lock_piece(const Piece& p) {
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            if (p.shape[j][i])
                board[p.y + j][p.x + i] = p.id;
}

Piece ghost_of(const Piece& p) {
    Piece g = p;
    while (true) {
        Piece t = g;
        t.y++;
        if (collides(t)) break;
        g = t;
    }
    return g;
}

// ===================== PARTICLES =====================
struct Particle {
    sf::Vector2f pos, vel;
    float angle, spin;
    float life, maxLife;
    float size;
    sf::Color color;
};

std::vector<Particle> particles;

void spawn_shatter(int gx, int gy, int id, float power) {
    sf::Color base = COLORS[id];
    sf::Color light((base.r + 255) / 2, (base.g + 255) / 2, (base.b + 255) / 2);
    float cx = gx * float(CELL) + CELL / 2.f;
    float cy = gy * float(CELL) + CELL / 2.f;

    for (int sy = -1; sy <= 1; sy += 2)
        for (int sx = -1; sx <= 1; sx += 2) {
            Particle p;
            p.pos = {cx + sx * CELL / 4.f, cy + sy * CELL / 4.f};
            p.vel = {sx * frand(40, 150) * power + frand(-40, 40),
                     sy * frand(20, 110) * power - frand(100, 280) * power};
            p.size = CELL / 2.f - 2;
            p.angle = frand(0, 90);
            p.spin = frand(-420, 420);
            p.maxLife = p.life = frand(0.9f, 1.6f);
            p.color = base;
            particles.push_back(p);
        }

    for (int k = 0; k < 3; k++) {
        Particle p;
        float a = frand(0, 6.2832f);
        float sp = frand(80, 260) * power;
        p.pos = {cx, cy};
        p.vel = {std::cos(a) * sp, std::sin(a) * sp - 80};
        p.size = frand(2, 4);
        p.angle = 0; p.spin = 0;
        p.maxLife = p.life = frand(0.4f, 0.9f);
        p.color = light;
        particles.push_back(p);
    }
}

int shatter_full_rows() {
    int n = 0;
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; x++)
            if (!board[y][x]) { full = false; break; }
        if (!full) continue;
        n++;
        for (int x = 0; x < BOARD_WIDTH; x++)
            spawn_shatter(x, y, board[y][x], 1.0f);
    }
    return n;
}

void landing_debris(const Piece& p) {
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++) {
            if (!p.shape[j][i]) continue;
            if (j < 3 && p.shape[j + 1][i]) continue;
            int bx = p.x + i, by = p.y + j;
            bool supported = by + 1 >= BOARD_HEIGHT || board[by + 1][bx];
            if (!supported) continue;

            sf::Color base = COLORS[p.id];
            sf::Color light((base.r + 255) / 2, (base.g + 255) / 2, (base.b + 255) / 2);
            float cx = bx * float(CELL) + CELL / 2.f;
            float fy = (by + 1) * float(CELL);
            for (int k = 0; k < 5; k++) {
                Particle c;
                c.pos = {cx + frand(-14, 14), fy - 4};
                c.vel = {frand(-170, 170), -frand(80, 270)};
                c.size = frand(5, 10);
                c.angle = frand(0, 90);
                c.spin = frand(-500, 500);
                c.maxLife = c.life = frand(0.5f, 1.0f);
                c.color = (k % 2) ? base : light;
                particles.push_back(c);
            }
        }
}

void update_particles(float dt) {
    for (auto& p : particles) {
        p.vel.y += 900.f * dt;
        p.pos += p.vel * dt;
        p.angle += p.spin * dt;
        p.life -= dt;
        float floorY = LH - p.size / 2;
        if (p.pos.y > floorY && p.vel.y > 0) {
            p.pos.y = floorY;
            p.vel.y *= -0.35f;
            p.vel.x *= 0.8f;
            p.spin *= 0.7f;
        }
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle& p) { return p.life <= 0.f; }),
                    particles.end());
}

void draw_particles(sf::RenderWindow& window) {
    sf::RectangleShape r;
    for (const auto& p : particles) {
        float t = std::clamp(p.life / p.maxLife, 0.f, 1.f);
        float s = p.size * (0.4f + 0.6f * t);
        r.setSize({s, s});
        r.setOrigin({s / 2, s / 2});
        r.setPosition(p.pos);
        r.setRotation(sf::degrees(p.angle));
        sf::Color c = p.color;
        c.a = static_cast<std::uint8_t>(255 * std::min(1.f, t * 1.6f));
        r.setFillColor(c);
        window.draw(r);
    }
}

// ===================== BACKGROUND DECORATIONS =====================
struct Star { float x, y, speed, phase; };
struct Floater { int type; float x, y, vy; };

enum class State { Menu, Settings, Playing, GameOver };   // NEW: Settings

void center_text(sf::Text& t, float x, float y) {
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x / 2.f,
                 b.position.y + b.size.y / 2.f});
    t.setPosition({x, y});
}

// NEW: same idea, but (x, y) becomes the RIGHT edge of the text (top-right corner)
void right_text(sf::Text& t, float x, float y) {
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x, 0.f});
    t.setPosition({x, y});
}

int main() {
    sf::RenderWindow window(sf::VideoMode({BOARD_WIDTH * CELL + PANEL, BOARD_HEIGHT * CELL}), "Tetris");
    window.setFramerateLimit(60);

    sf::View view(sf::FloatRect({0.f, 0.f}, {LW, LH}));
    auto update_view = [&](sf::Vector2u size) {
        float winRatio  = size.x / float(size.y);
        float viewRatio = LW / LH;
        float sx = 1.f, sy = 1.f, px = 0.f, py = 0.f;
        if (winRatio > viewRatio) {
            sx = viewRatio / winRatio;
            px = (1.f - sx) / 2.f;
        } else {
            sy = winRatio / viewRatio;
            py = (1.f - sy) / 2.f;
        }
        view.setViewport(sf::FloatRect({px, py}, {sx, sy}));
    };
    update_view(window.getSize());

    int nextType = random_type();
    Piece cur = spawn(random_type());
    sf::Clock clock;
    float dropInterval = 0.5f;

    int score = 0, lines = 0, level = 1;
    float shownScore = 0.f;
    const int POINTS[5] = {0, 100, 300, 500, 800};
    float shake = 0.f;

    // ---- sound effects ----
    sf::SoundBuffer landBuffer;
    if (!landBuffer.loadFromFile("assets/land.mp3")) {
        std::cerr << "Couldn't load assets/land.mp3\n";
        return 1;
    }

    sf::SoundBuffer clearBuffer;
    if (!clearBuffer.loadFromFile("assets/clear.mp3")) {
        std::cerr << "Couldn't load assets/clear.mp3\n";
        return 1;
    }
    sf::Sound clearSound(clearBuffer);
    sf::Sound landSound(landBuffer);

    // ---- NEW: settings values (0..100) and background music ----
    bool  musicOn  = true;
    float musicVol = 50.f;
    float sfxVol   = 80.f;
    float musicMul = 0.45f;     // fades between "quiet" (menus) and 1.0 (playing)

    auto apply_sfx = [&]() {
        clearSound.setVolume(sfxVol);
        landSound.setVolume(sfxVol);
    };
    apply_sfx();

    sf::Music music;                                   // streamed from disk, unlike sf::Sound
    bool hasMusic = music.openFromFile("assets/music.mp3");
    if (hasMusic) {
        music.setLooping(true);                        // restart automatically when it ends
        music.play();
    } else {
        std::cerr << "Couldn't load assets/music.mp3 (game runs without music)\n";
    }

    sf::Texture atlas = make_atlas();

    sf::Font font;
    if (!font.openFromFile("assets/font.ttf")) {
        std::cerr << "Couldn't load assets/font.ttf\n";
        return 1;
    }

    const float PX = BW + 20;

    sf::VertexArray bg(sf::PrimitiveType::TriangleStrip, 4);
    bg[0].position = {0, 0};   bg[0].color = sf::Color(10, 10, 22);
    bg[1].position = {LW, 0};  bg[1].color = sf::Color(10, 10, 22);
    bg[2].position = {0, LH};  bg[2].color = sf::Color(30, 14, 46);
    bg[3].position = {LW, LH}; bg[3].color = sf::Color(30, 14, 46);

    std::vector<Star> stars;
    for (int i = 0; i < 80; i++)
        stars.push_back({frand(0, LW), frand(0, LH), frand(4, 18), frand(0, 6.28f)});

    std::vector<Floater> floaters;
    for (int i = 0; i < 12; i++)
        floaters.push_back({random_type(), frand(0, LW - 80), frand(-100, LH), frand(25, 65)});

    auto make_text = [&](const std::string& str, unsigned size, sf::Color color, float x, float y) {
        sf::Text t(font, str, size);
        t.setFillColor(color);
        t.setPosition({x, y});
        return t;
    };
    const sf::Color GRAY(130, 130, 140);
    const sf::Color CYAN(0, 200, 255);
    const sf::Color LIGHT(200, 200, 210);

    // ---- menu texts ----
    sf::Text title(font, "TETRIS", 56);
    title.setFillColor(CYAN);
    title.setOutlineColor(sf::Color(0, 60, 100));
    title.setOutlineThickness(4.f);
    center_text(title, LW / 2.f, LH * 0.3f);

    // NEW: menu is now a list of buttons
    const std::vector<std::string> menuLabels = {"PLAY", "SETTINGS", "QUIT"};
    std::vector<sf::Text> menuItems;
    for (const auto& s : menuLabels)
        menuItems.push_back(make_text(s, 26, LIGHT, 0, 0));

    sf::Text menuHint = make_text("UP/DOWN + ENTER, or use the mouse", 13, GRAY, 0, 0);
    center_text(menuHint, LW / 2.f, LH * 0.9f);

    // ---- NEW: settings screen texts ----
    sf::Text setTitle(font, "SETTINGS", 40);
    setTitle.setFillColor(CYAN);
    setTitle.setOutlineColor(sf::Color(0, 60, 100));
    setTitle.setOutlineThickness(3.f);
    center_text(setTitle, LW / 2.f, 110.f);

    auto row_y = [&](int i) { return 200.f + i * 85.f; };   // top of settings row i

    std::vector<sf::Text> rowLabels;
    rowLabels.push_back(make_text("MUSIC",        20, LIGHT, 60, row_y(0)));
    rowLabels.push_back(make_text("MUSIC VOLUME", 20, LIGHT, 60, row_y(1)));
    rowLabels.push_back(make_text("SFX VOLUME",   20, LIGHT, 60, row_y(2)));
    rowLabels.push_back(make_text("BACK",         24, LIGHT, 0, 0));
    center_text(rowLabels[3], LW / 2.f, row_y(3) + 10.f);

    std::vector<sf::Text> rowValues;                         // ON/OFF, music %, sfx %
    for (int i = 0; i < 3; i++) rowValues.push_back(make_text("", 20, sf::Color::White, 0, 0));

    sf::Text setHint = make_text("UP/DOWN select    LEFT/RIGHT change\nESC back", 13, GRAY, 0, 0);
    center_text(setHint, LW / 2.f, LH * 0.93f);

    // clickable areas
    auto row_rect = [&](int i) {
        float y = row_y(i);
        if (i == 3) return sf::FloatRect({140.f, y - 8.f}, {200.f, 40.f});
        return sf::FloatRect({50.f, y - 8.f}, {380.f, 64.f});
    };
    auto slider_rect = [&](int i) { return sf::FloatRect({50.f, row_y(i) + 24.f}, {380.f, 32.f}); };

    // slider geometry: track from x=60 to x=420
    auto set_vol_from_x = [&](int row, float mx) {
        float v = std::clamp((mx - 60.f) / 360.f, 0.f, 1.f) * 100.f;
        if (row == 1) musicVol = v;
        else { sfxVol = v; apply_sfx(); }
    };

    // keyboard left/right on a row
    auto adjust = [&](int row, int dir) {
        if (row == 0) {
            musicOn = !musicOn;
        } else if (row == 1) {
            musicVol = std::clamp(musicVol + dir * 10.f, 0.f, 100.f);
        } else if (row == 2) {
            sfxVol = std::clamp(sfxVol + dir * 10.f, 0.f, 100.f);
            apply_sfx();
            landSound.play();                                // so you hear the new volume
        }
    };

    sf::RectangleShape track({360.f, 8.f}), fill({0.f, 8.f}), knob({10.f, 20.f}), rowBg({380.f, 64.f});

    // ---- game over texts ----
    sf::Text overTitle(font, "GAME OVER", 40);
    overTitle.setFillColor(sf::Color(255, 50, 50));
    overTitle.setOutlineColor(sf::Color(80, 0, 0));
    overTitle.setOutlineThickness(3.f);
    center_text(overTitle, BW / 2.f, LH * 0.38f);

    sf::Text overScore(font, "", 22);
    sf::Text overHint(font, "ENTER - play again\nESC - menu", 16);
    overHint.setFillColor(LIGHT);
    center_text(overHint, BW / 2.f, LH * 0.55f);

    sf::RectangleShape dim({LW, LH});
    dim.setFillColor(sf::Color(0, 0, 0, 150));

    // ---- side panel ----
    sf::RectangleShape panelBg({float(PANEL), LH});
    panelBg.setPosition({BW, 0});
    panelBg.setFillColor(sf::Color(18, 18, 28, 215));

    sf::RectangleShape divider({3.f, LH});
    divider.setPosition({BW, 0});
    divider.setFillColor(sf::Color(0, 200, 255, 120));

    sf::RectangleShape boardFrame({BW, LH});
    boardFrame.setFillColor(sf::Color::Transparent);
    boardFrame.setOutlineThickness(-2.f);
    boardFrame.setOutlineColor(sf::Color(0, 200, 255, 90));

    sf::RectangleShape nextBox({120.f, 90.f});
    nextBox.setPosition({PX, 40});
    nextBox.setFillColor(sf::Color(30, 30, 40));
    nextBox.setOutlineThickness(2.f);
    nextBox.setOutlineColor(sf::Color(60, 60, 80));

    sf::Text lblNext  = make_text("NEXT",  14, GRAY, PX, 14);
    sf::Text lblScore = make_text("SCORE", 14, GRAY, PX, 160);
    sf::Text lblLines = make_text("LINES", 14, GRAY, PX, 230);
    sf::Text lblLevel = make_text("LEVEL", 14, GRAY, PX, 300);
    sf::Text valScore = make_text("0", 24, sf::Color::White, PX, 178);
    sf::Text valLines = make_text("0", 24, sf::Color::White, PX, 248);
    sf::Text valLevel = make_text("1", 24, CYAN, PX, 318);
    sf::Text controls = make_text("< > move\n^ rotate\nv soft drop\nSPACE drop\nM music\nESC menu",
                                  13, GRAY, PX, LH - 138);

    State state = State::Menu;
    int menuSel = 0;          // NEW: highlighted menu button
    int setSel = 0;           // NEW: highlighted settings row
    int dragging = -1;        // NEW: which slider the mouse is dragging (-1 = none)
    sf::Clock appClock;
    sf::Clock frameClock;

    auto start_game = [&]() {
        for (auto& row : board)
            for (auto& c : row) c = 0;
        for (auto& row : flashT)
            for (auto& f : row) f = 0.f;
        particles.clear();
        score = 0; lines = 0; level = 1; shownScore = 0.f;
        dropInterval = 0.5f;
        nextType = random_type();
        cur = spawn(random_type());
        clock.restart();
        state = State::Playing;
    };

    auto menu_activate = [&](int i) {
        if (i == 0) start_game();
        else if (i == 1) { state = State::Settings; setSel = 0; }
        else window.close();
    };

    auto lock_and_next = [&]() {
        lock_piece(cur);

        int full = shatter_full_rows();
        if (full == 0) {
            landing_debris(cur);
            for (int j = 0; j < 4; j++)
                for (int i = 0; i < 4; i++)
                    if (cur.shape[j][i]) flashT[cur.y + j][cur.x + i] = 1.f;
        }
        int cleared = clear_lines();

        if (cleared > 0) {
            clearSound.play();
            score += POINTS[cleared] * level;
            lines += cleared;
            level = lines / 10 + 1;
            dropInterval = std::max(0.08f, 0.5f - (level - 1) * 0.04f);
            shake = std::max(shake, 6.f + cleared * 3.f);
        } else {
            landSound.play();
            shake = std::max(shake, 2.f);
        }

        cur = spawn(nextType);
        nextType = random_type();
        if (collides(cur)) {
            state = State::GameOver;
            for (int y = 0; y < BOARD_HEIGHT; y++)
                for (int x = 0; x < BOARD_WIDTH; x++)
                    if (board[y][x]) {
                        spawn_shatter(x, y, board[y][x], 1.4f);
                        board[y][x] = 0;
                    }
            shake = 14.f;
            overScore.setString("SCORE " + std::to_string(score));
            overScore.setFillColor(sf::Color(255, 220, 0));
            center_text(overScore, BW / 2.f, LH * 0.46f);
        }
    };

    auto try_move = [&](int dx, int dy) {
        Piece t = cur; t.x += dx; t.y += dy;
        if (collides(t)) return false;
        cur = t;
        return true;
    };

    while (window.isOpen()) {
        float dt = std::min(frameClock.restart().asSeconds(), 0.05f);
        float time = appClock.getElapsedTime().asSeconds();

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* rs = event->getIf<sf::Event::Resized>())
                update_view(rs->size);

            // ---- NEW: mouse (converted from window pixels to our 480x640 world coordinates) ----
            if (const auto* mm = event->getIf<sf::Event::MouseMoved>()) {
                sf::Vector2f m = window.mapPixelToCoords(mm->position, view);
                if (state == State::Menu) {
                    for (int i = 0; i < (int)menuItems.size(); i++)
                        if (menuItems[i].getGlobalBounds().contains(m)) menuSel = i;
                } else if (state == State::Settings) {
                    for (int i = 0; i < 4; i++)
                        if (row_rect(i).contains(m)) setSel = i;
                    if (dragging > 0) set_vol_from_x(dragging, m.x);   // slider follows the mouse
                }
            }
            if (const auto* mb = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left) {
                    sf::Vector2f m = window.mapPixelToCoords(mb->position, view);
                    if (state == State::Menu) {
                        for (int i = 0; i < (int)menuItems.size(); i++)
                            if (menuItems[i].getGlobalBounds().contains(m)) menu_activate(i);
                    } else if (state == State::Settings) {
                        if (row_rect(0).contains(m)) adjust(0, 1);
                        if (row_rect(3).contains(m)) state = State::Menu;
                        for (int i = 1; i <= 2; i++)
                            if (slider_rect(i).contains(m)) {
                                dragging = i;
                                set_vol_from_x(i, m.x);
                            }
                    }
                }
            }
            if (const auto* mr = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mr->button == sf::Mouse::Button::Left) {
                    if (dragging == 2) landSound.play();   // preview the SFX volume
                    dragging = -1;
                }
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                using K = sf::Keyboard::Key;

                if (key->code == K::M) musicOn = !musicOn;   // works in every state

                if (state == State::Menu) {
                    int n = (int)menuItems.size();
                    if (key->code == K::Up)     menuSel = (menuSel + n - 1) % n;   // +n so it never goes negative
                    if (key->code == K::Down)   menuSel = (menuSel + 1) % n;
                    if (key->code == K::Enter)  menu_activate(menuSel);
                    if (key->code == K::Escape) window.close();
                } else if (state == State::Settings) {
                    if (key->code == K::Up)     setSel = (setSel + 3) % 4;
                    if (key->code == K::Down)   setSel = (setSel + 1) % 4;
                    if (key->code == K::Left)   adjust(setSel, -1);
                    if (key->code == K::Right)  adjust(setSel, 1);
                    if (key->code == K::Enter) {
                        if (setSel == 0) adjust(0, 1);
                        if (setSel == 3) state = State::Menu;
                    }
                    if (key->code == K::Escape) state = State::Menu;
                } else if (state == State::GameOver) {
                    if (key->code == K::Enter)  start_game();
                    if (key->code == K::Escape) { state = State::Menu; menuSel = 0; }
                } else {
                    if (key->code == K::Escape) { state = State::Menu; menuSel = 0; }
                    if (key->code == K::Left)  try_move(-1, 0);
                    if (key->code == K::Right) try_move(1, 0);
                    if (key->code == K::Down)  try_move(0, 1);
                    if (key->code == K::Up && cur.id != 2) {
                        Piece r = rotated(cur);
                        for (int kick : {0, -1, 1, -2, 2}) {
                            Piece t = r; t.x += kick;
                            if (!collides(t)) { cur = t; break; }
                        }
                    }
                    if (key->code == K::Space) {
                        while (try_move(0, 1)) {}
                        shake = std::max(shake, 5.f);
                        lock_and_next();
                        clock.restart();
                    }
                }
            }
        }

        // ---------- update ----------
        if (state == State::Playing && clock.getElapsedTime().asSeconds() >= dropInterval) {
            if (!try_move(0, 1)) lock_and_next();
            clock.restart();
        }

        update_particles(dt);
        for (auto& row : flashT)
            for (auto& f : row) f = std::max(0.f, f - dt * 5.f);
        shake = std::max(0.f, shake - 40.f * dt);
        shownScore += (score - shownScore) * std::min(1.f, dt * 8.f);
        for (auto& s : stars) {
            s.y += s.speed * dt;
            if (s.y > LH) s.y -= LH;
        }
        for (auto& f : floaters) {
            f.y += f.vy * dt;
            if (f.y > LH) { f.y = -90; f.x = frand(0, LW - 80); f.type = random_type(); }
        }

        // NEW: music volume = slider * fade (quieter outside of gameplay), 0 when muted
        float targetMul = (state == State::Playing) ? 1.f : 0.45f;
        musicMul += (targetMul - musicMul) * std::min(1.f, dt * 3.f);
        if (hasMusic) music.setVolume(musicOn ? musicVol * musicMul : 0.f);

        view.setCenter({LW / 2.f + frand(-1, 1) * shake, LH / 2.f + frand(-1, 1) * shake});
        window.setView(view);

        // ---------- draw ----------
        window.clear(sf::Color(5, 5, 10));
        window.draw(bg);

        sf::RectangleShape starShape({2.f, 2.f});
        for (const auto& s : stars) {
            float a = 70 + 60 * std::sin(time * 2.f + s.phase);
            starShape.setPosition({s.x, s.y});
            starShape.setFillColor(sf::Color(180, 200, 255, static_cast<std::uint8_t>(a)));
            window.draw(starShape);
        }

        if (state == State::Menu || state == State::Settings) {
            for (const auto& f : floaters)
                for (int j = 0; j < 4; j++)
                    for (int i = 0; i < 4; i++)
                        if (SHAPES[f.type][j][i])
                            draw_block(window, atlas, f.type + 1, f.x + i * 20, f.y + j * 20, 20, 55);
        }

        if (state == State::Menu) {
            float pulse = 1.f + 0.04f * std::sin(time * 2.5f);
            title.setScale({pulse, pulse});
            window.draw(title);

            for (int i = 0; i < (int)menuItems.size(); i++) {
                bool sel = (i == menuSel);
                menuItems[i].setString(sel ? "> " + menuLabels[i] + " <" : menuLabels[i]);
                menuItems[i].setFillColor(sel ? CYAN : LIGHT);
                center_text(menuItems[i], LW / 2.f, LH * 0.5f + i * 52.f);
                float s = sel ? 1.f + 0.06f * std::sin(time * 6.f) : 1.f;   // selected button pulses
                menuItems[i].setScale({s, s});
                window.draw(menuItems[i]);
            }
            window.draw(menuHint);

        } else if (state == State::Settings) {
            window.draw(setTitle);

            // highlight bar behind the selected row
            rowBg.setSize(row_rect(setSel).size);
            rowBg.setPosition(row_rect(setSel).position);
            rowBg.setFillColor(sf::Color(255, 255, 255, 14));
            window.draw(rowBg);

            for (int i = 0; i < 4; i++) {
                rowLabels[i].setFillColor(i == setSel ? CYAN : LIGHT);
                window.draw(rowLabels[i]);
            }

            rowValues[0].setString(musicOn ? "ON" : "OFF");
            rowValues[0].setFillColor(musicOn ? sf::Color(0, 220, 80) : sf::Color(255, 50, 50));
            rowValues[1].setString(std::to_string(int(std::round(musicVol))) + "%");
            rowValues[2].setString(std::to_string(int(std::round(sfxVol))) + "%");
            for (int i = 0; i < 3; i++) right_text(rowValues[i], 420.f, row_y(i));
            for (auto& v : rowValues) window.draw(v);

            for (int i = 1; i <= 2; i++) {                     // the two sliders
                float v = (i == 1) ? musicVol : sfxVol;
                bool active = (i == 1) ? musicOn : true;       // music slider is grayed out when muted
                float y = row_y(i) + 36.f;

                track.setPosition({60.f, y});
                track.setFillColor(sf::Color(45, 45, 60));
                window.draw(track);

                fill.setSize({360.f * v / 100.f, 8.f});
                fill.setPosition({60.f, y});
                fill.setFillColor(active ? CYAN : sf::Color(90, 90, 100));
                window.draw(fill);

                knob.setPosition({60.f + 360.f * v / 100.f - 5.f, y - 6.f});
                knob.setFillColor(sf::Color::White);
                window.draw(knob);
            }
            window.draw(setHint);

        } else {
            draw_board(window, atlas);
            window.draw(boardFrame);
            if (state == State::Playing) {
                draw_piece(window, atlas, ghost_of(cur), 70);
                draw_piece(window, atlas, cur);
            }

            window.draw(panelBg);
            window.draw(divider);
            window.draw(lblNext);
            window.draw(nextBox);

            Piece np = spawn(nextType);
            int minI = 4, maxI = -1, minJ = 4, maxJ = -1;
            for (int j = 0; j < 4; j++)
                for (int i = 0; i < 4; i++)
                    if (np.shape[j][i]) {
                        minI = std::min(minI, i); maxI = std::max(maxI, i);
                        minJ = std::min(minJ, j); maxJ = std::max(maxJ, j);
                    }
            const float PS = 22.f;
            float ox = PX + (120.f - (maxI - minI + 1) * PS) / 2.f;
            float oy = 40  + (90.f  - (maxJ - minJ + 1) * PS) / 2.f;
            for (int j = 0; j < 4; j++)
                for (int i = 0; i < 4; i++)
                    if (np.shape[j][i])
                        draw_block(window, atlas, np.id, ox + (i - minI) * PS, oy + (j - minJ) * PS, PS);

            valScore.setString(std::to_string(int(std::round(shownScore))));
            valLines.setString(std::to_string(lines));
            valLevel.setString(std::to_string(level));
            window.draw(lblScore); window.draw(valScore);
            window.draw(lblLines); window.draw(valLines);
            window.draw(lblLevel); window.draw(valLevel);
            window.draw(controls);

            draw_particles(window);

            if (state == State::GameOver) {
                window.draw(dim);
                draw_particles(window);
                window.draw(overTitle);
                window.draw(overScore);
                window.draw(overHint);
            }
        }
        window.display();
    }
}
