#include "ConsoleDash.h"
#include "ColorHelper.h"
#include <cstdlib>
#include <string>
#include <cstdio>

namespace consoledash {

namespace {

char firefly_mark(int f)   { switch(f){case 0:return '|'; case 1:return '<'; case 2:return '>'; default:return '|';} }
char butterfly_mark(int f) { switch(f){case 0:return '|'; case 1:return '('; case 2:return ')'; default:return '|';} }
char firefly_explosion_mark(int stage)   { return (stage == 0) ? '+' : (stage == 1 ? 'x' : '*'); }
char butterfly_explosion_mark(int stage) { return (stage == 0) ? 'o' : (stage == 1 ? 'O' : '@'); }

// Renders the game to stdout. Must be called while the caller holds state_mutex_.
void do_render(const ConsoleDash& game) {
    using namespace Colors;

    const uint64_t counter = game.animation_counter();
    const bool anim_even        = (counter % 2) == 0;
    const int  anim_frame       = static_cast<int>(counter % 3);

    std::string frame;
    frame.reserve(static_cast<size_t>((game.level_width() + 1) * game.level_height() * 8));

    auto add_colored      = [&](const char* color, const std::string& glyph) { frame += color; frame += glyph; frame += RESET; };
    auto add_colored_char = [&](const char* color, char glyph)               { frame += color; frame += glyph; frame += RESET; };

#if defined(_WIN32) || defined(_WIN64)
    std::system("cls");
#else
    std::system("clear");
#endif
    frame += "\033[H";

    for (int y = 0; y < game.level_height(); ++y) {
        for (int x = 0; x < game.level_width(); ++x) {
            const Cell& cell = game.cell_at(x, y);
            switch (cell.tile) {
                case Tile::SPACE:         frame += ' '; break;
                case Tile::DIRT:          add_colored(C_DIM_YELLOW, "·"); break;
                case Tile::TITANIUM_WALL: add_colored_char(C_WHITE, '#'); break;
                case Tile::WALL:          add_colored_char(C_BLUE, '%'); break;
                case Tile::EXPLOSION: {
                    const int stage = (cell.explosion_stage >= 2) ? 2 : cell.explosion_stage;
                    if (cell.explosion_source == Tile::BUTTERFLY)
                        add_colored_char(C_MAGENTA, butterfly_explosion_mark(stage));
                    else
                        add_colored_char(C_BRIGHT_YELLOW, firefly_explosion_mark(stage));
                    break;
                }
                case Tile::ROCK:      add_colored_char(C_GRAY, 'O'); break;
                case Tile::DIAMOND:   add_colored_char(anim_even ? C_BRIGHT_CYAN : C_CYAN, '*'); break;
                case Tile::FIREFLY:   add_colored_char(C_BRIGHT_YELLOW, firefly_mark(anim_frame)); break;
                case Tile::BUTTERFLY: add_colored_char(C_MAGENTA, butterfly_mark(anim_frame)); break;
                case Tile::AMOEBA:    add_colored_char(C_BRIGHT_GREEN, anim_even ? '~' : '-'); break;
                case Tile::MAGIC_WALL:
                    if (game.magic_wall_active())
                        add_colored(C_BLUE, anim_even ? "%" : "°");
                    else
                        add_colored_char(C_BLUE, '%');
                    break;
                case Tile::ROCKFORD: add_colored_char(C_BRIGHT_GREEN, '@'); break;
                case Tile::EXIT: {
                    const bool open = game.diamonds_collected() >= game.diamonds_required();
                    add_colored_char(C_WHITE, open ? (anim_even ? ' ' : '#') : '#');
                    break;
                }
            }
        }
        frame += '\n';
    }

    frame += "Diamonds: ";
    frame += std::to_string(game.diamonds_collected());
    frame += '/';
    frame += std::to_string(game.diamonds_required());
    frame += "  Time: ";
    frame += std::to_string(game.time_remaining());

    if (game.player_wins())
        frame += "\nYOU WIN! Press Q to return";
    else if (game.game_over())
        frame += "  GAME OVER - Press Q to return";
    else
        frame += "  [WASD] Move  [Q] Quit";

    fwrite(frame.data(), 1, frame.size(), stdout);
    fflush(stdout);
}

} // namespace

void ConsoleDash::render() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    do_render(*this);
}

} // namespace consoledash
