#include "ConsoleDash.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdio>

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
static void init_input() {}
static void restore_input() {}
static int get_key_nonblock() {
    if (!_kbhit()) return 0;
    return _getch();
}
#else
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>

static struct termios s_saved;
static bool s_raw = false;

static void init_input() {
    if (!isatty(STDIN_FILENO)) return;
    tcgetattr(STDIN_FILENO, &s_saved);
    struct termios raw = s_saved;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    s_raw = true;
}

static void restore_input() {
    if (s_raw && isatty(STDIN_FILENO))
        tcsetattr(STDIN_FILENO, TCSANOW, &s_saved);
    s_raw = false;
}

static int get_key_nonblock() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = { 0, 0 };
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0)
        return 0;
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return 0;
    return c;
}
#endif

static void sample_input(int& dx, int& dy, bool& quit) {
    dx = 0;
    dy = 0;
    quit = false;
    int key = 0;
    int last = 0;
    while ((key = get_key_nonblock()) != 0)
        last = key;
    if (last == 0) return;
    if (last == 'q' || last == 'Q') { quit = true; return; }
    if (last == 'w' || last == 'W') dy = -1;
    else if (last == 's' || last == 'S') dy = 1;
    else if (last == 'a' || last == 'A') dx = -1;
    else if (last == 'd' || last == 'D') dx = 1;
}

static void build_test_level(consoledash::ConsoleDash& game) {
    using namespace consoledash;
    const int W = consoledash::WIDTH;
    const int H = consoledash::HEIGHT;

    for (int x = 0; x < W; ++x) {
        game.set_cell(x, 0, Tile::WALL);
        game.set_cell(x, H - 1, Tile::WALL);
    }
    for (int y = 0; y < H; ++y) {
        game.set_cell(0, y, Tile::WALL);
        game.set_cell(W - 1, y, Tile::WALL);
    }

    for (int y = 1; y < H - 1; ++y)
        for (int x = 1; x < W - 1; ++x)
            game.set_cell(x, y, Tile::DIRT);

    game.set_cell(2, 2, Tile::SPACE);
    game.set_rockford(2, 2);

    game.set_cell(10, 5, Tile::ROCK);
    game.set_cell(12, 5, Tile::ROCK);
    game.set_cell(14, 8, Tile::DIAMOND);
    game.set_cell(16, 8, Tile::DIAMOND);
    game.set_cell(18, 8, Tile::DIAMOND);

    game.set_cell(25, 6, Tile::FIREFLY, static_cast<uint8_t>(consoledash::Direction::LEFT));
    game.set_cell(28, 10, Tile::BUTTERFLY, static_cast<uint8_t>(consoledash::Direction::UP));

    game.set_cell(20, 12, Tile::AMOEBA);
    game.set_cell(22, 14, Tile::MAGIC_WALL);

    game.set_cell(W - 3, H - 3, Tile::EXIT);
    game.set_diamonds_required(3);
}

int main() {
    init_input();

    consoledash::ConsoleDash game;
    build_test_level(game);

    constexpr auto tick_interval = std::chrono::milliseconds(120);
    auto next_tick = std::chrono::steady_clock::now();

    game.render();
    while (game.is_alive()) {
        int dx = 0, dy = 0;
        bool quit = false;
        sample_input(dx, dy, quit);
        if (quit) break;

        game.set_input(dx, dy);
        game.tick();
        game.render();

        next_tick += tick_interval;
        std::this_thread::sleep_until(next_tick);
    }

    restore_input();
    game.render();
    if (game.player_wins())
        std::cout << "You win! Diamonds: " << game.diamonds_collected() << std::endl;
    else if (game.game_over())
        std::cout << "Game Over!" << std::endl;

    return 0;
}
