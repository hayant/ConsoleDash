#include "ConsoleDash.h"
#include "LevelLoader.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <string>

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

int main(int argc, char** argv) {
    init_input();

    consoledash::ConsoleDash game;
    consoledash::LevelParameters level_parameters;
    consoledash::LevelLoader level_loader;
    if (argc >= 2) {
        const std::string level_path = argv[1];
        if (!level_loader.load_from_file(level_path, game, level_parameters)) {
            restore_input();
            std::cerr << "Error: Could not open level file: " << level_path << '\n';
            return 1;
        }
    } else {
        level_loader.build_test_level(game);
    }

    const auto game_tick_interval = std::chrono::milliseconds(level_parameters.GAME_TICK_INTERVAL.value_or(250));
    const auto animation_tick_interval = std::chrono::milliseconds(level_parameters.ANIMATION_TICK_INTERVAL.value_or(200));
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
        std::cout << std::endl << "You win! Diamonds: " << game.diamonds_collected() << std::endl;
    else if (game.game_over())
        std::cout << std::endl << "Game Over!" << std::endl;

    return 0;
}
