#include "ConsoleDash.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <fstream>
#include <string>
#include <vector>

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

static void sample_input(int& dx, int& dy, bool& reach, bool& quit) {
    dx = 0;
    dy = 0;
    reach = false;
    quit = false;

    // Reach arm window: space must be pressed before movement key.
    static bool reach_armed = false;
    static auto reach_armed_at = std::chrono::steady_clock::now();
    constexpr auto reach_arm_timeout = std::chrono::milliseconds(350);

    const auto now = std::chrono::steady_clock::now();
    if (now - reach_armed_at > reach_arm_timeout) {
        reach_armed = false;
    }

    int key = 0;
    while ((key = get_key_nonblock()) != 0) {
        if (key == 'q' || key == 'Q') {
            quit = true;
            return;
        }
        if (key == ' ') {
            reach_armed = true;
            reach_armed_at = std::chrono::steady_clock::now();
            continue;
        }

        int ndx = 0, ndy = 0;
        if (key == 'w' || key == 'W') ndy = -1;
        else if (key == 's' || key == 'S') ndy = 1;
        else if (key == 'a' || key == 'A') ndx = -1;
        else if (key == 'd' || key == 'D') ndx = 1;
        else continue;

        dx = ndx;
        dy = ndy;
        reach = reach_armed;
        reach_armed = false; // consume arm on first movement key
    }
}

static void build_test_level(consoledash::ConsoleDash& game) {
    using namespace consoledash;
    const int W = consoledash::DEFAULT_WIDTH;
    const int H = consoledash::DEFAULT_HEIGHT;
    game.set_level_size(W, H);

    for (int x = 0; x < W; ++x) {
        game.set_cell(x, 0, Tile::TITANIUM_WALL);
        game.set_cell(x, H - 1, Tile::TITANIUM_WALL);
    }
    for (int y = 0; y < H; ++y) {
        game.set_cell(0, y, Tile::TITANIUM_WALL);
        game.set_cell(W - 1, y, Tile::TITANIUM_WALL);
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
    game.set_cell(24, 10, Tile::BUTTERFLY, static_cast<uint8_t>(consoledash::Direction::UP));

    game.set_cell(20, 12, Tile::AMOEBA);
    game.set_cell(22, 14, Tile::MAGIC_WALL);

    game.set_cell(W - 3, H - 3, Tile::EXIT);
    game.set_diamonds_required(3);
}

static bool load_level_from_file(const std::string& path, consoledash::ConsoleDash& game) {
    using namespace consoledash;
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    int level_height = static_cast<int>(lines.size());
    int level_width = 0;
    for (const std::string& ln : lines) {
        int w = static_cast<int>(ln.size());
        if (w > level_width) level_width = w;
    }

    if (level_width <= 0 || level_height <= 0) {
        return false;
    }
    if (level_width > WIDTH || level_height > HEIGHT) {
        std::cerr << "Error: Level size " << level_width << "x" << level_height
                  << " exceeds maximum " << WIDTH << "x" << HEIGHT << '\n';
        return false;
    }
    if (!game.set_level_size(level_width, level_height)) {
        return false;
    }

    bool has_rockford = false;
    for (int y = 0; y < level_height; ++y) {
        for (int x = 0; x < level_width; ++x) {
            // Missing columns in shorter lines are treated as empty space.
            char ch = ' ';
            if (y < static_cast<int>(lines.size()) && x < static_cast<int>(lines[y].size())) {
                ch = lines[y][x];
            }

            switch (ch) {
                case '@':
                    game.set_cell(x, y, Tile::SPACE);
                    game.set_rockford(x, y);
                    has_rockford = true;
                    break;
                case '#':
                    game.set_cell(x, y, Tile::TITANIUM_WALL);
                    break;
                case 'W':
                    game.set_cell(x, y, Tile::WALL);
                    break;
                case 'R':
                    game.set_cell(x, y, Tile::ROCK);
                    break;
                case 'F':
                    game.set_cell(x, y, Tile::FIREFLY);
                    break;
                case 'B':
                    game.set_cell(x, y, Tile::BUTTERFLY);
                    break;
                case 'A':
                    game.set_cell(x, y, Tile::AMOEBA);
                    break;
                case 'M':
                    game.set_cell(x, y, Tile::MAGIC_WALL);
                    break;
                case 'E':
                    game.set_cell(x, y, Tile::EXIT);
                    break;
                case ' ':
                    game.set_cell(x, y, Tile::SPACE);
                    break;
                case '.':
                    game.set_cell(x, y, Tile::DIRT);
                    break;
                case 'D': // backwards-compatible convenience for existing level files
                    game.set_cell(x, y, Tile::DIAMOND);
                    break;
                default:
                    // Unknown characters are interpreted as dirt.
                    game.set_cell(x, y, Tile::DIRT);
                    break;
            }
        }
    }

    if (!has_rockford) {
        game.set_rockford(1, 1);
    }
    game.set_diamonds_required(3);
    return true;
}

int main(int argc, char** argv) {
    init_input();

    consoledash::ConsoleDash game;
    if (argc >= 2) {
        const std::string level_path = argv[1];
        if (!load_level_from_file(level_path, game)) {
            restore_input();
            std::cerr << "Error: Could not open level file: " << level_path << '\n';
            return 1;
        }
    } else {
        build_test_level(game);
    }

    constexpr auto game_tick_interval = std::chrono::milliseconds(250);
    constexpr auto animation_tick_interval = std::chrono::milliseconds(200);
    auto next_game_tick = std::chrono::steady_clock::now();

    std::atomic<bool> animation_running{true};
    std::thread animation_thread([&] {
        auto next_anim_tick = std::chrono::steady_clock::now();
        while (animation_running.load(std::memory_order_relaxed)) {
            game.advance_animation();
            game.render();
            next_anim_tick += animation_tick_interval;
            std::this_thread::sleep_until(next_anim_tick);
        }
    });
    while (game.is_alive()) {
        int dx = 0, dy = 0;
        bool reach = false;
        bool quit = false;
        sample_input(dx, dy, reach, quit);
        if (quit) break;

        game.set_input(dx, dy, reach);
        game.tick();
        // Ensure gameplay movement/state updates are shown immediately
        // at each game tick boundary, independent of animation cadence.
        game.render();
        next_game_tick += game_tick_interval;
        std::this_thread::sleep_until(next_game_tick);
    }

    animation_running.store(false, std::memory_order_relaxed);
    if (animation_thread.joinable()) {
        animation_thread.join();
    }

    restore_input();
    game.render();
    if (game.player_wins())
        std::cout << "You win! Diamonds: " << game.diamonds_collected() << std::endl;
    else if (game.game_over())
        std::cout << "Game Over!" << std::endl;

    return 0;
}
