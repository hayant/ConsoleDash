#include "ConsoleDash.h"
#include <algorithm>

namespace consoledash {

ConsoleDash::ConsoleDash() {
    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y) {
            grid_[x][y] = Cell{};
            moved_[x][y] = false;
        }
    last_time_update_ = std::chrono::steady_clock::now();
}

bool ConsoleDash::in_bounds(int x, int y) const {
    return x >= 0 && x < level_width_ && y >= 0 && y < level_height_;
}

bool ConsoleDash::set_level_size(int width, int height) {
    if (width <= 0 || height <= 0 || width > WIDTH || height > HEIGHT) {
        return false;
    }

    level_width_ = width;
    level_height_ = height;

    for (int x = 0; x < WIDTH; ++x) {
        for (int y = 0; y < HEIGHT; ++y) {
            grid_[x][y] = Cell{};
            moved_[x][y] = false;
        }
    }

    rockford_x_ = 0;
    rockford_y_ = 0;
    pending_dx_ = 0;
    pending_dy_ = 0;
    pending_reach_ = false;
    diamonds_collected_ = 0;
    time_remaining_ = time_limit_;
    last_time_update_ = std::chrono::steady_clock::now();
    game_over_ = false;
    player_wins_ = false;
    amoeba_current_size_ = 0;
    magic_wall_timer_ = 0;
    animation_counter_.store(0, std::memory_order_relaxed);
    return true;
}

void ConsoleDash::set_time_limit(int seconds) {
    if (seconds <= 0) return;
    time_limit_ = seconds;
    time_remaining_ = seconds;
    last_time_update_ = std::chrono::steady_clock::now();
}

void ConsoleDash::set_amoeba_max_size(int max_size) {
    if (max_size <= 0) {
        return;
    }
    amoeba_max_size_ = max_size;
}

void ConsoleDash::set_amoeba_growth_factor(int growth_factor) {
    if (growth_factor <= 0) {
        return;
    }
    amoeba_growth_factor_ = growth_factor;
}

void ConsoleDash::set_magic_wall_duration(int duration_ticks) {
    if (duration_ticks <= 0) {
        return;
    }
    magic_wall_duration_ = duration_ticks;
}

bool ConsoleDash::is_space(int x, int y) const {
    if (!in_bounds(x, y)) return false;
    return grid_[x][y].tile == Tile::SPACE;
}

bool ConsoleDash::is_empty_or_dirt(int x, int y) const {
    if (!in_bounds(x, y)) return false;
    Tile t = grid_[x][y].tile;
    return t == Tile::SPACE || t == Tile::DIRT;
}

bool ConsoleDash::is_blocking(int x, int y) const {
    if (!in_bounds(x, y)) return true;
    Tile t = grid_[x][y].tile;
    return t == Tile::TITANIUM_WALL || t == Tile::WALL || t == Tile::ROCK || t == Tile::DIAMOND ||
           t == Tile::MAGIC_WALL || t == Tile::EXIT || t == Tile::EXPLOSION ||
           t == Tile::DIRT || t == Tile::ROCKFORD || t == Tile::AMOEBA;
}

bool ConsoleDash::can_roll_over(int x, int y) const {
    if (!in_bounds(x, y)) return true;
    Tile t = grid_[x][y].tile;
    return t == Tile::WALL || t == Tile::ROCK || t == Tile::DIAMOND;
}

bool ConsoleDash::can_roll_into(int x, int y) const {
    return in_bounds(x, y) && grid_[x][y].tile == Tile::SPACE;
}

void ConsoleDash::mark_moved(int x, int y) {
    if (in_bounds(x, y)) moved_[x][y] = true;
}

bool ConsoleDash::was_moved(int x, int y) const {
    return in_bounds(x, y) && moved_[x][y];
}

void ConsoleDash::set_cell_internal(int x, int y, Tile t, uint8_t facing, bool falling) {
    if (!in_bounds(x, y)) return;
    grid_[x][y].tile = t;
    grid_[x][y].facing = facing;
    grid_[x][y].was_falling = falling;
    if (t != Tile::EXPLOSION) {
        grid_[x][y].explosion_stage = 0;
        grid_[x][y].explosion_source = Tile::SPACE;
        grid_[x][y].explosion_result = Tile::SPACE;
    }
}

void ConsoleDash::clear_cell(int x, int y) {
    if (!in_bounds(x, y)) return;
    grid_[x][y] = Cell{};
}

void ConsoleDash::set_cell(int x, int y, Tile t, uint8_t facing) {
    set_cell_internal(x, y, t, facing, false);
}

void ConsoleDash::set_rockford(int x, int y) {
    if (!in_bounds(x, y)) return;
    for (int ix = 0; ix < level_width_; ++ix)
        for (int iy = 0; iy < level_height_; ++iy)
            if (grid_[ix][iy].tile == Tile::ROCKFORD) {
                grid_[ix][iy] = Cell{};
                break;
            }
    rockford_x_ = x;
    rockford_y_ = y;
    set_cell_internal(x, y, Tile::ROCKFORD);
}

void ConsoleDash::set_exit(int x, int y) {
    set_cell(x, y, Tile::EXIT);
}

void ConsoleDash::set_input(int dx, int dy, bool reach) {
    pending_dx_ = dx;
    pending_dy_ = dy;
    pending_reach_ = reach;
}

void ConsoleDash::advance_animation() {
    animation_counter_.fetch_add(1, std::memory_order_relaxed);
}

void ConsoleDash::tick() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!player_wins_) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(now - last_time_update_).count();
        if (elapsed_seconds > 0) {
            if (!game_over_) {
                if (elapsed_seconds >= time_remaining_) {
                    time_remaining_ = 0;
                    game_over_ = true;
                } else {
                    time_remaining_ -= static_cast<int>(elapsed_seconds);
                }
            } else {
                // Keep time scrolling after death for display purposes
                if (time_remaining_ > 0)
                    time_remaining_ = static_cast<int>(std::max(0LL,
                        static_cast<long long>(time_remaining_) - elapsed_seconds));
            }
            last_time_update_ += std::chrono::seconds(elapsed_seconds);
        }
    }
    advance_explosions();

    for (int x = 0; x < level_width_; ++x)
        for (int y = 0; y < level_height_; ++y)
            moved_[x][y] = false;

    if (!game_over_ && !player_wins_) {
        if (pending_reach_) {
            try_reach_rockford(pending_dx_, pending_dy_);
        } else {
            try_move_rockford(pending_dx_, pending_dy_);
        }
        pending_dx_ = 0;
        pending_dy_ = 0;
        pending_reach_ = false;
    }

    // If we were already dead when this tick started, keep simulating the world
    // (creatures, amoeba, rocks keep moving). If we die *during* this tick,
    // stop processing further cells as usual.
    const bool was_dead = game_over_;
    for (int y = 0; y < level_height_ && (was_dead || !game_over_) && !player_wins_; ++y)
        for (int x = 0; x < level_width_; ++x)
            if (!was_moved(x, y))
                process_cell(x, y);

    post_tick_amoeba();

    if (magic_wall_timer_ > 0)
        magic_wall_timer_--;
}

void ConsoleDash::process_cell(int x, int y) {
    Tile t = grid_[x][y].tile;
    switch (t) {
        case Tile::SPACE:
        case Tile::DIRT:
        case Tile::TITANIUM_WALL:
        case Tile::WALL:
        case Tile::EXPLOSION:
        case Tile::EXIT:
            break;
        case Tile::ROCK:
        case Tile::DIAMOND:
            process_rock_or_diamond(x, y);
            break;
        case Tile::FIREFLY:
            process_firefly(x, y);
            break;
        case Tile::BUTTERFLY:
            process_butterfly(x, y);
            break;
        case Tile::AMOEBA:
            process_amoeba(x, y);
            break;
        case Tile::MAGIC_WALL:
            process_magic_wall(x, y);
            break;
        case Tile::ROCKFORD:
            process_rockford(x, y);
            break;
    }
}

} // namespace consoledash
