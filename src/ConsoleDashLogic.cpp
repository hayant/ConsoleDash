#include "ConsoleDash.h"
#include <algorithm>
#include <random>

namespace consoledash {

namespace {
int dx_for(Direction d) { if (d == Direction::LEFT) return -1; if (d == Direction::RIGHT) return 1; return 0; }
int dy_for(Direction d) { if (d == Direction::UP) return -1; if (d == Direction::DOWN) return 1; return 0; }
Direction turn_left(Direction d) { return static_cast<Direction>((static_cast<uint8_t>(d) + 1) % 4); }
Direction turn_right(Direction d) { return static_cast<Direction>((static_cast<uint8_t>(d) + 3) % 4); }
Direction turn_right_cw(Direction d) { return static_cast<Direction>((static_cast<uint8_t>(d) + 3) % 4); }
Direction turn_left_cw(Direction d) { return static_cast<Direction>((static_cast<uint8_t>(d) + 1) % 4); }
}

void ConsoleDash::process_rock_or_diamond(int x, int y) {
    Tile tile = grid_[x][y].tile;
    bool was_falling = grid_[x][y].was_falling;
    int nx = x, ny = y + 1;
    if (!in_bounds(nx, ny)) { grid_[x][y].was_falling = false; return; }
    Cell& below = grid_[nx][ny];
    if (below.tile == Tile::ROCKFORD) { if (was_falling) game_over_ = true; grid_[x][y].was_falling = false; return; }
    if (is_space(nx, ny)) { set_cell_internal(nx, ny, tile, 0, true, 0); clear_cell(x, y); mark_moved(x, y); mark_moved(nx, ny); return; }
    if (below.tile == Tile::FIREFLY) {
        if (was_falling) { explode_firefly(nx, ny); mark_moved(x, y); mark_moved(nx, ny); }
        else grid_[x][y].was_falling = false;
        return;
    }
    if (below.tile == Tile::BUTTERFLY) {
        if (was_falling) { explode_butterfly(nx, ny); mark_moved(x, y); mark_moved(nx, ny); }
        else grid_[x][y].was_falling = false;
        return;
    }
    if (below.tile == Tile::MAGIC_WALL) {
        if (was_falling) {
            below.magic_timer = magic_wall_duration_;
            Tile converted = (tile == Tile::ROCK) ? Tile::DIAMOND : Tile::ROCK;
            int tx = nx, ty = ny + 1;
            if (in_bounds(tx, ty) && is_space(tx, ty)) {
                set_cell_internal(tx, ty, converted, 0, true, 0);
                clear_cell(x, y);
                mark_moved(x, y);
                mark_moved(tx, ty);
            }
        } else {
            grid_[x][y].was_falling = false;
        }
        return;
    }
    if (!can_roll_over(nx, ny)) { grid_[x][y].was_falling = false; return; }
    bool downLeft = can_roll_into(x - 1, y + 1);
    bool downRight = can_roll_into(x + 1, y + 1);
    bool leftFree = !is_blocking(x - 1, y);
    bool rightFree = !is_blocking(x + 1, y);
    if (downLeft && leftFree) { set_cell_internal(x - 1, y, tile, 0, true, 0); clear_cell(x, y); mark_moved(x, y); mark_moved(x - 1, y); return; }
    if (downRight && rightFree) { set_cell_internal(x + 1, y, tile, 0, true, 0); clear_cell(x, y); mark_moved(x, y); mark_moved(x + 1, y); return; }
    grid_[x][y].was_falling = false;
}

void ConsoleDash::process_firefly(int x, int y) {
    for (int d = 0; d < 4; ++d) {
        int dx = dx_for(static_cast<Direction>(d));
        int dy = dy_for(static_cast<Direction>(d));
        if (in_bounds(x + dx, y + dy) && grid_[x + dx][y + dy].tile == Tile::ROCKFORD) { explode_firefly(x, y); game_over_ = true; return; }
    }
    for (int d = 0; d < 4; ++d) {
        int dx = dx_for(static_cast<Direction>(d));
        int dy = dy_for(static_cast<Direction>(d));
        if (in_bounds(x + dx, y + dy) && grid_[x + dx][y + dy].tile == Tile::AMOEBA) { explode_firefly(x, y); return; }
    }
    Direction facing = static_cast<Direction>(grid_[x][y].facing);
    int lx = x + dx_for(turn_left(facing));
    int ly = y + dy_for(turn_left(facing));
    int fx = x + dx_for(facing);
    int fy = y + dy_for(facing);
    if (in_bounds(lx, ly) && is_space(lx, ly)) { set_cell_internal(lx, ly, Tile::FIREFLY, static_cast<uint8_t>(turn_left(facing)), false, 0); clear_cell(x, y); mark_moved(x, y); mark_moved(lx, ly); return; }
    if (in_bounds(fx, fy) && is_space(fx, fy)) { set_cell_internal(fx, fy, Tile::FIREFLY, grid_[x][y].facing, false, 0); clear_cell(x, y); mark_moved(x, y); mark_moved(fx, fy); return; }
    grid_[x][y].facing = static_cast<uint8_t>(turn_right(facing));
}

void ConsoleDash::process_butterfly(int x, int y) {
    for (int d = 0; d < 4; ++d) {
        int dx = dx_for(static_cast<Direction>(d));
        int dy = dy_for(static_cast<Direction>(d));
        if (in_bounds(x + dx, y + dy) && grid_[x + dx][y + dy].tile == Tile::ROCKFORD) { explode_butterfly(x, y); game_over_ = true; return; }
    }
    for (int d = 0; d < 4; ++d) {
        int dx = dx_for(static_cast<Direction>(d));
        int dy = dy_for(static_cast<Direction>(d));
        if (in_bounds(x + dx, y + dy) && grid_[x + dx][y + dy].tile == Tile::AMOEBA) { explode_butterfly(x, y); return; }
    }
    Direction facing = static_cast<Direction>(grid_[x][y].facing);
    Direction rightOf = turn_right_cw(facing);
    int rx = x + dx_for(rightOf);
    int ry = y + dy_for(rightOf);
    int fx = x + dx_for(facing);
    int fy = y + dy_for(facing);
    if (in_bounds(rx, ry) && is_space(rx, ry)) { set_cell_internal(rx, ry, Tile::BUTTERFLY, static_cast<uint8_t>(rightOf), false, 0); clear_cell(x, y); mark_moved(x, y); mark_moved(rx, ry); return; }
    if (in_bounds(fx, fy) && is_space(fx, fy)) { set_cell_internal(fx, fy, Tile::BUTTERFLY, grid_[x][y].facing, false, 0); clear_cell(x, y); mark_moved(x, y); mark_moved(fx, fy); return; }
    grid_[x][y].facing = static_cast<uint8_t>(turn_left_cw(facing));
}

void ConsoleDash::process_amoeba(int x, int y) {
    static std::mt19937 growth_rng(std::random_device{}());
    const int growth_upper = std::max(0, amoeba_growth_factor_ - (amoeba_current_size_ / amoeba_max_size_ * amoeba_growth_factor_));
    int rand_number = std::uniform_int_distribution<int>(0, growth_upper)(growth_rng);
    if (rand_number != 0) return;
    int order[4] = {0, 1, 2, 3};
    static std::mt19937 rng(std::random_device{}());
    std::shuffle(order, order + 4, rng);
    for (int i = 0; i < 4; ++i) {
        int dx = 0, dy = 0;
        if (order[i] == 0) dx = -1, dy = 0;
        if (order[i] == 1) dx = 1, dy = 0;
        if (order[i] == 2) dx = 0, dy = -1;
        if (order[i] == 3) dx = 0, dy = 1;
        int nx = x + dx, ny = y + dy;
        if (in_bounds(nx, ny) && (grid_[nx][ny].tile == Tile::SPACE || grid_[nx][ny].tile == Tile::DIRT)) { set_cell_internal(nx, ny, Tile::AMOEBA, 0, false, 0); mark_moved(nx, ny); return; }
    }
}

void ConsoleDash::process_magic_wall(int x, int y) { (void)x; (void)y; }
void ConsoleDash::process_rockford(int x, int y) { (void)x; (void)y; try_move_rockford(pending_dx_, pending_dy_); pending_dx_ = 0; pending_dy_ = 0; }

void ConsoleDash::explode_firefly(int x, int y) { explode_at(x, y, Tile::FIREFLY, Tile::SPACE); }
void ConsoleDash::explode_butterfly(int x, int y) { explode_at(x, y, Tile::BUTTERFLY, Tile::DIAMOND); }

void ConsoleDash::explode_at(int cx, int cy, Tile source, Tile fill) {
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            int x = cx + dx, y = cy + dy;
            if (!in_bounds(x, y)) continue;
            if (grid_[x][y].tile == Tile::TITANIUM_WALL) continue;
            if (grid_[x][y].tile == Tile::ROCKFORD) game_over_ = true;
            set_cell_internal(x, y, Tile::EXPLOSION, 0, false, 0);
            grid_[x][y].explosion_stage = 0;
            grid_[x][y].explosion_source = source;
            grid_[x][y].explosion_result = fill;
            mark_moved(x, y);
        }
}

void ConsoleDash::advance_explosions() {
    for (int x = 0; x < level_width_; ++x)
        for (int y = 0; y < level_height_; ++y) {
            if (grid_[x][y].tile == Tile::EXPLOSION) {
                if (grid_[x][y].explosion_stage < 2) {
                    grid_[x][y].explosion_stage++;
                } else {
                    set_cell_internal(x, y, grid_[x][y].explosion_result, 0, false, 0);
                }
            }
        }
}

void ConsoleDash::post_tick_amoeba() {
    amoeba_current_size_ = 0;
    bool has_growth_option = false;
    for (int x = 0; x < level_width_; ++x)
        for (int y = 0; y < level_height_; ++y)
            if (grid_[x][y].tile == Tile::AMOEBA) {
                amoeba_current_size_++;
                for (int d = 0; d < 4; ++d) {
                    int dx = (d == 0) ? -1 : (d == 1) ? 1 : 0;
                    int dy = (d == 2) ? -1 : (d == 3) ? 1 : 0;
                    if (d == 0) dx = -1, dy = 0;
                    if (d == 1) dx = 1, dy = 0;
                    if (d == 2) dx = 0, dy = -1;
                    if (d == 3) dx = 0, dy = 1;
                    int nx = x + dx, ny = y + dy;
                    if (in_bounds(nx, ny) && (grid_[nx][ny].tile == Tile::SPACE || grid_[nx][ny].tile == Tile::DIRT)) has_growth_option = true;
                }
            }
    if (amoeba_current_size_ >= amoeba_max_size_) {
        for (int x = 0; x < level_width_; ++x)
            for (int y = 0; y < level_height_; ++y)
                if (grid_[x][y].tile == Tile::AMOEBA) set_cell_internal(x, y, Tile::ROCK, 0, false, 0);
        return;
    }
    if (!has_growth_option && amoeba_current_size_ > 0)
        for (int x = 0; x < level_width_; ++x)
            for (int y = 0; y < level_height_; ++y)
                if (grid_[x][y].tile == Tile::AMOEBA) set_cell_internal(x, y, Tile::DIAMOND, 0, false, 0);
}

void ConsoleDash::try_move_rockford(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    int tx = rockford_x_ + dx, ty = rockford_y_ + dy;
    if (!in_bounds(tx, ty)) return;
    Tile target = grid_[tx][ty].tile;
    if (target == Tile::SPACE || target == Tile::DIRT) {
        int ox = rockford_x_, oy = rockford_y_;
        clear_cell(ox, oy);
        set_cell_internal(tx, ty, Tile::ROCKFORD, 0, false, 0);
        rockford_x_ = tx;
        rockford_y_ = ty;
        mark_moved(ox, oy);
        mark_moved(tx, ty);
        return;
    }
    if (target == Tile::DIAMOND) {
        if (grid_[tx][ty].was_falling) { game_over_ = true; return; }
        diamonds_collected_++;
        int ox = rockford_x_, oy = rockford_y_;
        clear_cell(ox, oy);
        set_cell_internal(tx, ty, Tile::ROCKFORD, 0, false, 0);
        rockford_x_ = tx;
        rockford_y_ = ty;
        mark_moved(ox, oy);
        mark_moved(tx, ty);
        return;
    }
    if (target == Tile::EXIT) { if (diamonds_collected_ >= diamonds_required_) player_wins_ = true; return; }
    if (target == Tile::ROCK) {
        if (dy == 0) {
            int bx = tx + dx, by = ty + dy;
            if (in_bounds(bx, by) && can_roll_into(bx, by)) {
                int ox = rockford_x_, oy = rockford_y_;
                set_cell_internal(bx, by, Tile::ROCK, 0, false, 0);
                clear_cell(ox, oy);
                clear_cell(tx, ty);
                set_cell_internal(tx, ty, Tile::ROCKFORD, 0, false, 0);
                rockford_x_ = tx;
                rockford_y_ = ty;
                mark_moved(ox, oy);
                mark_moved(tx, ty);
                mark_moved(bx, by);
            }
        }
        return;
    }
    if (target == Tile::FIREFLY || target == Tile::BUTTERFLY) { game_over_ = true; return; }
}

void ConsoleDash::try_reach_rockford(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    int tx = rockford_x_ + dx;
    int ty = rockford_y_ + dy;
    if (!in_bounds(tx, ty)) return;
    Tile target = grid_[tx][ty].tile;
    if (target == Tile::DIAMOND) { diamonds_collected_++; clear_cell(tx, ty); mark_moved(tx, ty); return; }
    if (target == Tile::ROCK) {
        if (dy == 0) {
            int bx = tx + dx;
            int by = ty + dy;
            if (in_bounds(bx, by) && can_roll_into(bx, by)) {
                set_cell_internal(bx, by, Tile::ROCK, 0, false, 0);
                clear_cell(tx, ty);
                mark_moved(tx, ty);
                mark_moved(bx, by);
            }
        }
        return;
    }
    if (target == Tile::DIRT) { clear_cell(tx, ty); return; }
}

} // namespace consoledash
