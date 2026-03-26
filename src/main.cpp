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
    consoledash::LevelLoader level_loader;

    while (true) {
        const consoledash::MainMenuSelectionResult menu_result = main_menu.show(input_helper);
        if (menu_result.result == consoledash::MainMenuResult::Quit) {
            break;
        }

        consoledash::ConsoleDash game;
        consoledash::LevelParameters level_parameters;
        if (!level_loader.load_from_file(menu_result.selected_level_path, game, level_parameters)) {
            std::cerr << "Error: Could not open level file: " << menu_result.selected_level_path << '\n';
            continue;
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

        bool player_quit = false;
        while (game.is_alive()) {
            int dx = 0, dy = 0;
            bool reach = false;
            bool quit = false;
            input_helper.sample_input(dx, dy, reach, quit);
            if (quit) {
                player_quit = true;
                break;
            }

            game.set_input(dx, dy, reach);
            game.tick();
            game.render();
            next_game_tick += game_tick_interval;
            std::this_thread::sleep_until(next_game_tick);
        }

        // After death: keep the world simulating until the player presses Q.
        // The animation thread continues rendering; we just drive tick() here.
        while (game.game_over() && !player_quit) {
            bool quit = false, reach = false;
            int dx = 0, dy = 0;
            input_helper.sample_input(dx, dy, reach, quit);
            if (quit) {
                player_quit = true;
                break;
            }
            game.tick();
            next_game_tick += game_tick_interval;
            std::this_thread::sleep_until(next_game_tick);
        }

        // Player wins: keep the animation going on.
        while (game.player_wins()) {
            bool quit = false, reach = false;
            int dx = 0, dy = 0;
            input_helper.sample_input(dx, dy, reach, quit);
            if (quit) {
                player_quit = true;
                break;
            }
        }

        animation_running.store(false, std::memory_order_relaxed);
        if (animation_thread.joinable()) {
            animation_thread.join();
        }
    }

    input_helper.restore_input();
    return 0;
}
