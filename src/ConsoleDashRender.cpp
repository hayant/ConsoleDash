#include "ConsoleDash.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstdio>

namespace consoledash {

namespace {
char firefly_mark(int f){ switch(f){case 0:return '|'; case 1:return '<'; case 2:return '>'; default:return '|';}}
char butterfly_mark(int f){ switch(f){case 0:return '|'; case 1:return '('; case 2:return ')'; default:return '|';}}
}

void ConsoleDash::render() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    std::string frame;
    const bool anim_even = (animation_counter_.load(std::memory_order_relaxed) % 2) == 0;
    const int anim_frame_per_three = animation_counter_.load(std::memory_order_relaxed) % 3;
    constexpr const char* RESET = "\033[0m";
    constexpr const char* C_DIM_YELLOW = "\033[2;33m";
    constexpr const char* C_WHITE = "\033[37m";
    constexpr const char* C_GRAY = "\033[90m";
    constexpr const char* C_BRIGHT_CYAN = "\033[96m";
    constexpr const char* C_BRIGHT_YELLOW = "\033[93m";
    constexpr const char* C_MAGENTA = "\033[35m";
    constexpr const char* C_BRIGHT_GREEN = "\033[92m";
    constexpr const char* C_BLUE = "\033[34m";
    constexpr const char* C_BRIGHT_RED = "\033[91m";

    auto add_colored = [&](const char* color, const std::string& glyph) { frame += color; frame += glyph; frame += RESET; };
    auto add_colored_char = [&](const char* color, char glyph) { frame += color; frame += glyph; frame += RESET; };

#if defined(_WIN32) || defined(_WIN64)
    std::system("cls");
#else
    std::system("clear");
#endif
    frame.reserve((level_width_ + 1) * level_height_ * 8);
    frame += "\033[H";

    for (int y = 0; y < level_height_; ++y) {
        for (int x = 0; x < level_width_; ++x) {
            switch (grid_[x][y].tile) {
                case Tile::SPACE: frame += ' '; break;
                case Tile::DIRT: add_colored(C_DIM_YELLOW, "·"); break;
                case Tile::TITANIUM_WALL: add_colored_char(C_WHITE, '#'); break;
                case Tile::WALL: add_colored_char(C_BLUE, '%'); break;
                case Tile::EXPLOSION: { char mark='a'; if(grid_[x][y].explosion_stage==1) mark='b'; else if(grid_[x][y].explosion_stage>=2) mark='c'; add_colored_char(C_BRIGHT_RED, mark); break; }
                case Tile::ROCK: add_colored_char(C_GRAY, 'O'); break;
                case Tile::DIAMOND: add_colored_char(C_BRIGHT_CYAN, '*'); break;
                case Tile::FIREFLY: add_colored_char(C_BRIGHT_YELLOW, firefly_mark(anim_frame_per_three)); break;
                case Tile::BUTTERFLY: add_colored_char(C_MAGENTA, butterfly_mark(anim_frame_per_three)); break;
                case Tile::AMOEBA: add_colored_char(C_BRIGHT_GREEN, (anim_even ? '~' : '-')); break;
                case Tile::MAGIC_WALL: add_colored_char(C_BLUE, 'M'); break;
                case Tile::ROCKFORD: add_colored_char(C_BRIGHT_GREEN, '@'); break;
                case Tile::EXIT: add_colored_char(C_WHITE, (diamonds_collected_ >= diamonds_required_ ? (anim_even ? ' ' : '#') : '#')); break;
            }
        }
        frame += '\n';
    }

    fwrite(frame.data(), 1, frame.size(), stdout);
    fflush(stdout);
    std::cout << "Diamonds: " << diamonds_collected_ << '/' << diamonds_required_ << "  [WASD] Move  [Q] Quit";
}

} // namespace consoledash
