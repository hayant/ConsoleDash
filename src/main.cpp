#include "ConsoleDash.h"
#include "InputHelper.h"
#include "LevelLoader.h"
#include "MainMenu.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    consoledash::InputHelper input_helper;
    input_helper.init_input();

    consoledash::MainMenu main_menu;
    if (main_menu.show(input_helper) == consoledash::MainMenuResult::Quit) {
        input_helper.restore_input();
        return 0;
    }

    consoledash::ConsoleDash game;
    consoledash::LevelParameters level_parameters;
    consoledash::LevelLoader level_loader;
    level_loader.build_test_level(game);

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
        input_helper.sample_input(dx, dy, reach, quit);
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

    input_helper.restore_input();
    game.render();
    if (game.player_wins())
        std::cout << std::endl << "You win! Diamonds: " << game.diamonds_collected() << std::endl;
    else if (game.game_over())
        std::cout << std::endl << "Game Over!" << std::endl;

    return 0;
}
