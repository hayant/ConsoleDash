#include "ConsoleDash.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <random>

namespace consoledash {

namespace {

int dx_for(Direction d) {
    if (d == Direction::LEFT) return -1;
    if (d == Direction::RIGHT) return 1;
    return 0;
}
int dy_for(Direction d) {
    if (d == Direction::UP) return -1;
    if (d == Direction::DOWN) return 1;
    return 0;
}

// CCW: Up -> Left -> Down -> Right. Next direction when turning left.
Direction turn_left(Direction d) {
    return static_cast<Direction>((static_cast<uint8_t>(d) + 1) % 4);
}
// Turning right = opposite of turn left (3 steps).
Direction turn_right(Direction d) {
    return static_cast<Direction>((static_cast<uint8_t>(d) + 3) % 4);
}
// CW: Up -> Right -> Down -> Left. "Right of facing" for butterfly.
Direction turn_right_cw(Direction d) {
    return static_cast<Direction>((static_cast<uint8_t>(d) + 3) % 4);
}
Direction turn_left_cw(Direction d) {
    return static_cast<Direction>((static_cast<uint8_t>(d) + 1) % 4);
}

} // namespace

ConsoleDash::ConsoleDash() {
    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y) {
            grid_[x][y] = Cell{};
            moved_[x][y] = false;
        }
}

bool ConsoleDash::in_bounds(int x, int y) const {
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
}

bool ConsoleDash::is_empty_or_dirt(int x, int y) const {
    if (!in_bounds(x, y)) return false;
    Tile t = grid_[x][y].tile;
    return t == Tile::SPACE || t == Tile::DIRT;
}

bool ConsoleDash::is_blocking(int x, int y) const {
    if (!in_bounds(x, y)) return true;
    Tile t = grid_[x][y].tile;
    return t == Tile::WALL || t == Tile::ROCK || t == Tile::DIAMOND ||
           t == Tile::MAGIC_WALL || t == Tile::EXIT;
}

bool ConsoleDash::can_roll_into(int x, int y) const {
    return in_bounds(x, y) && (grid_[x][y].tile == Tile::SPACE || grid_[x][y].tile == Tile::DIRT);
}

void ConsoleDash::mark_moved(int x, int y) {
    if (in_bounds(x, y)) moved_[x][y] = true;
}

bool ConsoleDash::was_moved(int x, int y) const {
    return in_bounds(x, y) && moved_[x][y];
}

void ConsoleDash::set_cell_internal(int x, int y, Tile t, uint8_t facing, bool falling, int magic_timer) {
    if (!in_bounds(x, y)) return;
    grid_[x][y].tile = t;
    grid_[x][y].facing = facing;
    grid_[x][y].was_falling = falling;
    grid_[x][y].magic_timer = magic_timer;
}

void ConsoleDash::clear_cell(int x, int y) {
    if (!in_bounds(x, y)) return;
    grid_[x][y].tile = Tile::SPACE;
    grid_[x][y].facing = 0;
    grid_[x][y].was_falling = false;
    grid_[x][y].magic_timer = 0;
}

void ConsoleDash::set_cell(int x, int y, Tile t, uint8_t facing) {
    set_cell_internal(x, y, t, facing, false, 0);
}

void ConsoleDash::set_rockford(int x, int y) {
    if (!in_bounds(x, y)) return;
    // Clear previous Rockford position if any
    for (int ix = 0; ix < WIDTH; ++ix)
        for (int iy = 0; iy < HEIGHT; ++iy)
            if (grid_[ix][iy].tile == Tile::ROCKFORD) {
                grid_[ix][iy].tile = Tile::SPACE;
                grid_[ix][iy].facing = 0;
                grid_[ix][iy].was_falling = false;
                grid_[ix][iy].magic_timer = 0;
                break;
            }
    rockford_x_ = x;
    rockford_y_ = y;
    set_cell_internal(x, y, Tile::ROCKFORD, 0, false, 0);
}

void ConsoleDash::set_exit(int x, int y) {
    set_cell(x, y, Tile::EXIT);
}

void ConsoleDash::set_input(int dx, int dy) {
    pending_dx_ = dx;
    pending_dy_ = dy;
}

void ConsoleDash::tick() {
    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y)
            moved_[x][y] = false;

    // Scan order: top-left to bottom-right
    for (int y = 0; y < HEIGHT && !game_over_ && !player_wins_; ++y)
        for (int x = 0; x < WIDTH; ++x)
            if (!was_moved(x, y))
                process_cell(x, y);

    post_tick_amoeba();

    // Decrement magic wall timers
    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y)
            if (grid_[x][y].tile == Tile::MAGIC_WALL && grid_[x][y].magic_timer > 0)
                grid_[x][y].magic_timer--;
}

void ConsoleDash::process_cell(int x, int y) {
    Tile t = grid_[x][y].tile;
    switch (t) {
        case Tile::SPACE:
        case Tile::DIRT:
        case Tile::WALL:
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

void ConsoleDash::process_rock_or_diamond(int x, int y) {
    Tile tile = grid_[x][y].tile;
    int nx = x, ny = y + 1;
    if (!in_bounds(nx, ny)) return;

    Cell& below = grid_[nx][ny];
    // Below is Rockford -> fall onto him and crush
    if (below.tile == Tile::ROCKFORD) {
        game_over_ = true;
        set_cell_internal(nx, ny, tile, 0, true, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(nx, ny);
        return;
    }

    // Gravity: below is empty or dirt -> fall
    if (is_empty_or_dirt(nx, ny)) {
        set_cell_internal(nx, ny, tile, 0, true, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(nx, ny);
        return;
    }

    // Below is firefly/butterfly -> explosion (only when falling onto them)
    if (below.tile == Tile::FIREFLY) {
        explode_firefly(nx, ny);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(nx, ny);
        return;
    }
    if (below.tile == Tile::BUTTERFLY) {
        explode_butterfly(nx, ny);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(nx, ny);
        return;
    }

    // Below is magic wall: activate and convert rock <-> diamond, place below wall
    if (below.tile == Tile::MAGIC_WALL) {
        below.magic_timer = MAGIC_WALL_DURATION;
        Tile converted = (tile == Tile::ROCK) ? Tile::DIAMOND : Tile::ROCK;
        int tx = nx, ty = ny + 1;
        if (in_bounds(tx, ty) && is_empty_or_dirt(tx, ty)) {
            set_cell_internal(tx, ty, converted, 0, true, 0);
            clear_cell(x, y);
            mark_moved(x, y);
            mark_moved(tx, ty);
        }
        return;
    }

    // Below is blocking (wall, rock, diamond) -> try roll
    if (!is_blocking(nx, ny)) return;
    bool downLeft = can_roll_into(x - 1, y + 1);
    bool downRight = can_roll_into(x + 1, y + 1);
    bool leftFree = !is_blocking(x - 1, y);
    bool rightFree = !is_blocking(x + 1, y);
    if (downLeft && leftFree) {
        set_cell_internal(x - 1, y + 1, tile, 0, true, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(x - 1, y + 1);
        return;
    }
    if (downRight && rightFree) {
        set_cell_internal(x + 1, y + 1, tile, 0, true, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(x + 1, y + 1);
        return;
    }
}

void ConsoleDash::process_firefly(int x, int y) {
    // Explode if Rockford touches any cardinal side
    for (int d = 0; d < 4; ++d) {
        int dx = dx_for(static_cast<Direction>(d));
        int dy = dy_for(static_cast<Direction>(d));
        if (in_bounds(x + dx, y + dy) && grid_[x + dx][y + dy].tile == Tile::ROCKFORD) {
            explode_firefly(x, y);
            game_over_ = true;
            return;
        }
    }

    Direction facing = static_cast<Direction>(grid_[x][y].facing);
    int lx = x + dx_for(turn_left(facing));
    int ly = y + dy_for(turn_left(facing));
    int fx = x + dx_for(facing);
    int fy = y + dy_for(facing);

    // 1. Left of facing empty -> turn left, move
    if (in_bounds(lx, ly) && is_empty_or_dirt(lx, ly)) {
        set_cell_internal(lx, ly, Tile::FIREFLY, static_cast<uint8_t>(turn_left(facing)), false, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(lx, ly);
        return;
    }
    // 2. Forward empty -> move forward
    if (in_bounds(fx, fy) && is_empty_or_dirt(fx, fy)) {
        set_cell_internal(fx, fy, Tile::FIREFLY, grid_[x][y].facing, false, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(fx, fy);
        return;
    }
    // 3. Else turn right (no move)
    grid_[x][y].facing = static_cast<uint8_t>(turn_right(facing));
}

void ConsoleDash::process_butterfly(int x, int y) {
    // Explode if Rockford touches
    for (int d = 0; d < 4; ++d) {
        int dx = dx_for(static_cast<Direction>(d));
        int dy = dy_for(static_cast<Direction>(d));
        if (in_bounds(x + dx, y + dy) && grid_[x + dx][y + dy].tile == Tile::ROCKFORD) {
            explode_butterfly(x, y);
            game_over_ = true;
            return;
        }
    }

    Direction facing = static_cast<Direction>(grid_[x][y].facing);
    Direction rightOf = turn_right_cw(facing);
    int rx = x + dx_for(rightOf);
    int ry = y + dy_for(rightOf);
    int fx = x + dx_for(facing);
    int fy = y + dy_for(facing);

    if (in_bounds(rx, ry) && is_empty_or_dirt(rx, ry)) {
        set_cell_internal(rx, ry, Tile::BUTTERFLY, static_cast<uint8_t>(rightOf), false, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(rx, ry);
        return;
    }
    if (in_bounds(fx, fy) && is_empty_or_dirt(fx, fy)) {
        set_cell_internal(fx, fy, Tile::BUTTERFLY, grid_[x][y].facing, false, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(fx, fy);
        return;
    }
    grid_[x][y].facing = static_cast<uint8_t>(turn_left_cw(facing));
}

void ConsoleDash::process_amoeba(int x, int y) {
    if (amoeba_growth_counter_ % AMOEBA_GROWTH_INTERVAL != 0) return;

    int order[4] = {0, 1, 2, 3};
    static std::mt19937 rng(42);
    std::shuffle(order, order + 4, rng);
    for (int i = 0; i < 4; ++i) {
        int dx = 0, dy = 0;
        if (order[i] == 0) dx = -1, dy = 0;
        if (order[i] == 1) dx = 1, dy = 0;
        if (order[i] == 2) dx = 0, dy = -1;
        if (order[i] == 3) dx = 0, dy = 1;
        int nx = x + dx, ny = y + dy;
        if (in_bounds(nx, ny) && (grid_[nx][ny].tile == Tile::SPACE || grid_[nx][ny].tile == Tile::DIRT)) {
            set_cell_internal(nx, ny, Tile::AMOEBA, 0, false, 0);
            mark_moved(nx, ny);
            return;
        }
    }
}

void ConsoleDash::process_magic_wall(int x, int y) {
    (void)x;
    (void)y;
    // Timer decrement done at end of tick
}

void ConsoleDash::process_rockford(int x, int y) {
    try_move_rockford(pending_dx_, pending_dy_);
    pending_dx_ = 0;
    pending_dy_ = 0;
}

void ConsoleDash::explode_firefly(int x, int y) {
    explode_at(x, y, Tile::SPACE);
}

void ConsoleDash::explode_butterfly(int x, int y) {
    explode_at(x, y, Tile::DIAMOND);
}

void ConsoleDash::explode_at(int cx, int cy, Tile fill) {
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            int x = cx + dx, y = cy + dy;
            if (!in_bounds(x, y)) continue;
            if (grid_[x][y].tile == Tile::WALL) continue;
            if (grid_[x][y].tile == Tile::ROCKFORD) game_over_ = true;
            set_cell_internal(x, y, fill, 0, false, 0);
            mark_moved(x, y);
        }
}

void ConsoleDash::post_tick_amoeba() {
    int count = 0;
    bool has_growth_option = false;
    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y) {
            if (grid_[x][y].tile != Tile::AMOEBA) continue;
            count++;
            for (int d = 0; d < 4; ++d) {
                int dx = (d == 0) ? -1 : (d == 1) ? 1 : 0;
                int dy = (d == 2) ? -1 : (d == 3) ? 1 : 0;
                if (d == 0) dx = -1, dy = 0;
                if (d == 1) dx = 1, dy = 0;
                if (d == 2) dx = 0, dy = -1;
                if (d == 3) dx = 0, dy = 1;
                int nx = x + dx, ny = y + dy;
                if (in_bounds(nx, ny) && (grid_[nx][ny].tile == Tile::SPACE || grid_[nx][ny].tile == Tile::DIRT))
                    has_growth_option = true;
            }
        }
    if (count >= AMOEBA_MAX_SIZE) {
        for (int x = 0; x < WIDTH; ++x)
            for (int y = 0; y < HEIGHT; ++y)
                if (grid_[x][y].tile == Tile::AMOEBA)
                    set_cell_internal(x, y, Tile::ROCK, 0, false, 0);
        return;
    }
    if (!has_growth_option && count > 0) {
        for (int x = 0; x < WIDTH; ++x)
            for (int y = 0; y < HEIGHT; ++y)
                if (grid_[x][y].tile == Tile::AMOEBA)
                    set_cell_internal(x, y, Tile::DIAMOND, 0, false, 0);
    }
    amoeba_growth_counter_++;
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
    if (target == Tile::EXIT) {
        if (diamonds_collected_ >= diamonds_required_) {
            player_wins_ = true;
        }
        return;
    }
    if (target == Tile::ROCK) {
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
        return;
    }
    if (target == Tile::FIREFLY || target == Tile::BUTTERFLY) {
        game_over_ = true;
        return;
    }
}

void ConsoleDash::render() const {
#if defined(_WIN32) || defined(_WIN64)
    std::system("cls");
#else
    std::system("clear");
#endif
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            switch (grid_[x][y].tile) {
                case Tile::SPACE:       std::cout << ' '; break;
                case Tile::DIRT:        std::cout << '.'; break;
                case Tile::WALL:        std::cout << '#'; break;
                case Tile::ROCK:        std::cout << 'O'; break;
                case Tile::DIAMOND:     std::cout << '*'; break;
                case Tile::FIREFLY:     std::cout << 'f'; break;
                case Tile::BUTTERFLY:   std::cout << 'b'; break;
                case Tile::AMOEBA:      std::cout << '~'; break;
                case Tile::MAGIC_WALL:  std::cout << 'M'; break;
                case Tile::ROCKFORD:    std::cout << '@'; break;
                case Tile::EXIT:        std::cout << 'X'; break;
            }
        }
        std::cout << '\n';
    }
    std::cout << "Diamonds: " << diamonds_collected_ << '/' << diamonds_required_
              << "  [WASD] Move  [Q] Quit\n";
}

} // namespace consoledash
