#include "ConsoleDash.h"
#include "ColorHelper.h"
#include <cstdlib>
#include <string>
#include <cstdio>

namespace consoledash {

namespace {
char firefly_mark(int f){ switch(f){case 0:return '|'; case 1:return '<'; case 2:return '>'; default:return '|';}}
char butterfly_mark(int f){ switch(f){case 0:return '|'; case 1:return '('; case 2:return ')'; default:return '|';}}
char firefly_explosion_mark(int stage){ return (stage == 0) ? '+' : (stage == 1 ? 'x' : '*'); }
char butterfly_explosion_mark(int stage){ return (stage == 0) ? 'o' : (stage == 1 ? 'O' : '@'); }
}

void ConsoleDash::render() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    std::string frame;
    const bool anim_even = (animation_counter_.load(std::memory_order_relaxed) % 2) == 0;
    const int anim_frame_per_three = static_cast<int>(animation_counter_.load(std::memory_order_relaxed) % 3);
    using C = ColorHelper;

    auto add_colored = [&](const char* color, const std::string& glyph) { frame += color; frame += glyph; frame += C::RESET; };
    auto add_colored_char = [&](const char* color, char glyph) { frame += color; frame += glyph; frame += C::RESET; };

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
                case Tile::DIRT: add_colored(C::C_DIM_YELLOW, "·"); break;
                case Tile::TITANIUM_WALL: add_colored_char(C::C_WHITE, '#'); break;
                case Tile::WALL: add_colored_char(C::C_BLUE, '%'); break;
                case Tile::EXPLOSION: {
                    const int stage = (grid_[x][y].explosion_stage >= 2) ? 2 : grid_[x][y].explosion_stage;
                    if (grid_[x][y].explosion_source == Tile::BUTTERFLY) {
                        add_colored_char(C::C_MAGENTA, butterfly_explosion_mark(stage));
                    } else {
                        add_colored_char(C::C_BRIGHT_YELLOW, firefly_explosion_mark(stage));
                    }
                    break;
                }
                case Tile::ROCK: add_colored_char(C::C_GRAY, 'O'); break;
                case Tile::DIAMOND: add_colored_char(C::C_BRIGHT_CYAN, '*'); break;
                case Tile::FIREFLY: add_colored_char(C::C_BRIGHT_YELLOW, firefly_mark(anim_frame_per_three)); break;
                case Tile::BUTTERFLY: add_colored_char(C::C_MAGENTA, butterfly_mark(anim_frame_per_three)); break;
                case Tile::AMOEBA: add_colored_char(C::C_BRIGHT_GREEN, (anim_even ? '~' : '-')); break;
                case Tile::MAGIC_WALL: {
                    if (grid_[x][y].magic_timer > 0) {
                        add_colored(C::C_BLUE, (anim_even ? "%" : "°"));
                    } else {
                        add_colored_char(C::C_BLUE, '%');
                    }
                    break;
                }
                case Tile::ROCKFORD: add_colored_char(C::C_BRIGHT_GREEN, '@'); break;
                case Tile::EXIT: add_colored_char(C::C_WHITE, (diamonds_collected_ >= diamonds_required_ ? (anim_even ? ' ' : '#') : '#')); break;
            }
        }
        frame += '\n';
    }
    frame += "Diamonds: ";
    frame += std::to_string(diamonds_collected_);
    frame += '/';
    frame += std::to_string(diamonds_required_);
    frame += "  Time: ";
    frame += std::to_string(time_remaining_);

    if (player_wins_) {
        frame += "\nYOU WIN! Press Q to return";
    }
    else if (game_over_)
        frame += "  GAME OVER - Press Q to return";
    else
        frame += "  [WASD] Move  [Q] Quit";

    fwrite(frame.data(), 1, frame.size(), stdout);
    fflush(stdout);
}

} // namespace consoledash
