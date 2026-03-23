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
           t == Tile::DIRT || t == Tile::ROCKFORD ||
           t == Tile::AMOEBA;
}

bool ConsoleDash::can_roll_over(int x, int y) const {
    if (!in_bounds(x, y)) return true;
    Tile t = grid_[x][y].tile;
    return t == Tile::WALL || t == Tile::ROCK || t == Tile::DIAMOND;
}

bool ConsoleDash::can_roll_into(int x, int y) const {
    // Rolling/moving objects may only enter empty space (not dirt).
    return in_bounds(x, y) && grid_[x][y].tile == Tile::SPACE;
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
    if (t != Tile::EXPLOSION) {
        grid_[x][y].explosion_stage = 0;
        grid_[x][y].explosion_result = Tile::SPACE;
    }
}

void ConsoleDash::clear_cell(int x, int y) {
    if (!in_bounds(x, y)) return;
    grid_[x][y].tile = Tile::SPACE;
    grid_[x][y].facing = 0;
    grid_[x][y].was_falling = false;
    grid_[x][y].magic_timer = 0;
    grid_[x][y].explosion_stage = 0;
    grid_[x][y].explosion_result = Tile::SPACE;
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
    advance_explosions();

    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y)
            moved_[x][y] = false;

    // Apply player input at the beginning of the tick.
    // Rockford acts once per tick (move or reach), otherwise stays.
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

void ConsoleDash::process_rock_or_diamond(int x, int y) {
    Tile tile = grid_[x][y].tile;
    bool was_falling = grid_[x][y].was_falling;
    int nx = x, ny = y + 1;
    if (!in_bounds(nx, ny)) {
        grid_[x][y].was_falling = false;
        return;
    }

    Cell& below = grid_[nx][ny];
    // Below is Rockford: stationary rock is safe; a rock/diamond that was falling
    // (moved down in the previous tick) crushes on this tick.
    if (below.tile == Tile::ROCKFORD) {
        if (was_falling) {
            game_over_ = true;
        }
        grid_[x][y].was_falling = false; // consume falling marker (only one tick)
        return;
    }

    // Gravity: below must be empty space -> fall
    if (is_space(nx, ny)) {
        set_cell_internal(nx, ny, tile, 0, true, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(nx, ny);
        return;
    }

    // Below is firefly/butterfly -> explosion (only when falling onto them)
    if (below.tile == Tile::FIREFLY) {
        if (was_falling) {
            explode_firefly(nx, ny);
            clear_cell(x, y);
            mark_moved(x, y);
            mark_moved(nx, ny);
        } else {
            grid_[x][y].was_falling = false; // consume marker
        }
        return;
    }
    if (below.tile == Tile::BUTTERFLY) {
        if (was_falling) {
            explode_butterfly(nx, ny);
            clear_cell(x, y);
            mark_moved(x, y);
            mark_moved(nx, ny);
        } else {
            grid_[x][y].was_falling = false; // consume marker
        }
        return;
    }

    // Below is magic wall: activate and convert rock <-> diamond, place below wall
    if (below.tile == Tile::MAGIC_WALL) {
        below.magic_timer = MAGIC_WALL_DURATION;
        Tile converted = (tile == Tile::ROCK) ? Tile::DIAMOND : Tile::ROCK;
        int tx = nx, ty = ny + 1;
        if (in_bounds(tx, ty) && is_space(tx, ty)) {
            set_cell_internal(tx, ty, converted, 0, true, 0);
            clear_cell(x, y);
            mark_moved(x, y);
            mark_moved(tx, ty);
        }
        // Even if conversion couldn't happen, a falling marker only lasts one tick.
        grid_[x][y].was_falling = false;
        return;
    }

    // Below is blocking (wall, rock, diamond) -> try roll
    if (!can_roll_over(nx, ny)) {
        // No downward movement this tick -> falling marker is consumed.
        grid_[x][y].was_falling = false;
        return;
    }
    bool downLeft = can_roll_into(x - 1, y + 1);
    bool downRight = can_roll_into(x + 1, y + 1);
    bool leftFree = !is_blocking(x - 1, y);
    bool rightFree = !is_blocking(x + 1, y);
    if (downLeft && leftFree) {
        set_cell_internal(x - 1, y, tile, 0, true, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(x - 1, y);
        return;
    }
    if (downRight && rightFree) {
        set_cell_internal(x + 1, y, tile, 0, true, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(x + 1, y);
        return;
    }

    // If we didn't move down this tick, we consumed the falling marker.
    grid_[x][y].was_falling = false;
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

    // Firefly explodes when it touches amoeba (cardinal adjacency).
    for (int d = 0; d < 4; ++d) {
        int dx = dx_for(static_cast<Direction>(d));
        int dy = dy_for(static_cast<Direction>(d));
        if (in_bounds(x + dx, y + dy) && grid_[x + dx][y + dy].tile == Tile::AMOEBA) {
            explode_firefly(x, y);
            return;
        }
    }

    Direction facing = static_cast<Direction>(grid_[x][y].facing);
    int lx = x + dx_for(turn_left(facing));
    int ly = y + dy_for(turn_left(facing));
    int fx = x + dx_for(facing);
    int fy = y + dy_for(facing);

    // 1. Left of facing empty -> turn left, move
    if (in_bounds(lx, ly) && is_space(lx, ly)) {
        set_cell_internal(lx, ly, Tile::FIREFLY, static_cast<uint8_t>(turn_left(facing)), false, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(lx, ly);
        return;
    }
    // 2. Forward empty -> move forward
    if (in_bounds(fx, fy) && is_space(fx, fy)) {
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

    // Butterfly explodes when it touches amoeba (cardinal adjacency).
    for (int d = 0; d < 4; ++d) {
        int dx = dx_for(static_cast<Direction>(d));
        int dy = dy_for(static_cast<Direction>(d));
        if (in_bounds(x + dx, y + dy) && grid_[x + dx][y + dy].tile == Tile::AMOEBA) {
            explode_butterfly(x, y);
            return;
        }
    }

    Direction facing = static_cast<Direction>(grid_[x][y].facing);
    Direction rightOf = turn_right_cw(facing);
    int rx = x + dx_for(rightOf);
    int ry = y + dy_for(rightOf);
    int fx = x + dx_for(facing);
    int fy = y + dy_for(facing);

    if (in_bounds(rx, ry) && is_space(rx, ry)) {
        set_cell_internal(rx, ry, Tile::BUTTERFLY, static_cast<uint8_t>(rightOf), false, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(rx, ry);
        return;
    }
    if (in_bounds(fx, fy) && is_space(fx, fy)) {
        set_cell_internal(fx, fy, Tile::BUTTERFLY, grid_[x][y].facing, false, 0);
        clear_cell(x, y);
        mark_moved(x, y);
        mark_moved(fx, fy);
        return;
    }
    grid_[x][y].facing = static_cast<uint8_t>(turn_left_cw(facing));
}

void ConsoleDash::process_amoeba(int x, int y) {
    static std::mt19937 growth_rng(std::random_device{}());
    int rand_number = std::uniform_int_distribution<int>(0, AMOEBA_MAX_SIZE / 2 - amoeba_current_size_ / 2)(growth_rng);
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
            // Titanium wall is indestructible. Regular wall is consumable by explosion fill.
            if (grid_[x][y].tile == Tile::TITANIUM_WALL) continue;
            if (grid_[x][y].tile == Tile::ROCKFORD) game_over_ = true;
            set_cell_internal(x, y, Tile::EXPLOSION, 0, false, 0);
            grid_[x][y].explosion_stage = 0;
            grid_[x][y].explosion_result = fill;
            mark_moved(x, y);
        }
}

void ConsoleDash::advance_explosions() {
    for (int x = 0; x < WIDTH; ++x) {
        for (int y = 0; y < HEIGHT; ++y) {
            if (grid_[x][y].tile != Tile::EXPLOSION) continue;
            if (grid_[x][y].explosion_stage < 2) {
                grid_[x][y].explosion_stage++;
            } else {
                Tile result = grid_[x][y].explosion_result;
                set_cell_internal(x, y, result, 0, false, 0);
            }
        }
    }
}

void ConsoleDash::post_tick_amoeba() {
    amoeba_current_size_ = 0;
    bool has_growth_option = false;
    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y) {
            if (grid_[x][y].tile != Tile::AMOEBA) continue;
            amoeba_current_size_++;
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
    if (amoeba_current_size_ >= AMOEBA_MAX_SIZE) {
        for (int x = 0; x < WIDTH; ++x)
            for (int y = 0; y < HEIGHT; ++y)
                if (grid_[x][y].tile == Tile::AMOEBA)
                    set_cell_internal(x, y, Tile::ROCK, 0, false, 0);
        return;
    }
    if (!has_growth_option && amoeba_current_size_ > 0) {
        for (int x = 0; x < WIDTH; ++x)
            for (int y = 0; y < HEIGHT; ++y)
                if (grid_[x][y].tile == Tile::AMOEBA)
                    set_cell_internal(x, y, Tile::DIAMOND, 0, false, 0);
    }
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
        // Collecting a diamond while it is falling counts as being crushed.
        if (grid_[tx][ty].was_falling) {
            game_over_ = true;
            return;
        }
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

void ConsoleDash::try_reach_rockford(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    int tx = rockford_x_ + dx;
    int ty = rockford_y_ + dy;
    if (!in_bounds(tx, ty)) return;

    Tile target = grid_[tx][ty].tile;

    // Reach collect: collect adjacent diamond without moving.
    if (target == Tile::DIAMOND) {
        diamonds_collected_++;
        clear_cell(tx, ty);
        mark_moved(tx, ty);
        return;
    }

    // Reach push: same push rule as walk-into-rock, but Rockford stays put.
    if (target == Tile::ROCK) {
        int bx = tx + dx;
        int by = ty + dy;
        if (in_bounds(bx, by) && can_roll_into(bx, by)) {
            set_cell_internal(bx, by, Tile::ROCK, 0, false, 0);
            clear_cell(tx, ty);
            mark_moved(tx, ty);
            mark_moved(bx, by);
        }
        return;
    }

    // Dirt is digged with reach.
    if (target == Tile::DIRT) {
        clear_cell(tx, ty);
        return;
    }
}

const char firefly_mark(int anim_frame_per_three) {
    switch (anim_frame_per_three) {
        case 0: return '|';
        case 1: return '<';
        case 2: return '>';
        default: return '|';
    }
}

const char butterfly_mark(int anim_frame_per_three) {
    switch (anim_frame_per_three) {
        case 0: return '|';
        case 1: return '(';
        case 2: return ')';
        default: return '|';
    }
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
    constexpr const char* C_RED = "\033[31m";
    constexpr const char* C_BRIGHT_RED = "\033[91m";

    auto add_colored = [&](const char* color, const std::string& glyph) {
        frame += color;
        frame += glyph;
        frame += RESET;
    };
    auto add_colored_char = [&](const char* color, char glyph) {
        frame += color;
        frame += glyph;
        frame += RESET;
    };

#if defined(_WIN32) || defined(_WIN64)
    std::system("cls");
#else
    std::system("clear");
#endif
    frame.reserve((WIDTH + 1) * HEIGHT * 2); // *2 for occasional multibyte chars
    frame += "\033[H";

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            switch (grid_[x][y].tile) {
                case Tile::SPACE:
                    frame += ' ';
                    break;
                case Tile::DIRT:
                    add_colored(C_DIM_YELLOW, "\u00B7");
                    break;
                case Tile::TITANIUM_WALL:
                    add_colored_char(C_WHITE, '#');
                    break;
                case Tile::WALL:
                    add_colored_char(C_BLUE, '%');
                    break;
                case Tile::EXPLOSION: {
                    char mark = 'a';
                    if (grid_[x][y].explosion_stage == 1) mark = 'b';
                    else if (grid_[x][y].explosion_stage >= 2) mark = 'c';
                    add_colored_char(C_BRIGHT_RED, mark);
                    break;
                }
                case Tile::ROCK:
                    add_colored_char(C_GRAY, 'O');
                    break;
                case Tile::DIAMOND:
                    add_colored_char(C_BRIGHT_CYAN, '*');
                    break;
                case Tile::FIREFLY:
                    add_colored_char(C_BRIGHT_YELLOW, firefly_mark(anim_frame_per_three));
                    break;
                case Tile::BUTTERFLY:
                    add_colored_char(C_MAGENTA, butterfly_mark(anim_frame_per_three));
                    break;
                case Tile::AMOEBA:
                    add_colored_char(C_BRIGHT_GREEN, (anim_even ? '~' : '-'));
                    break;
                case Tile::MAGIC_WALL:
                    add_colored_char(C_BLUE, 'M');
                    break;
                case Tile::ROCKFORD:
                    add_colored_char(C_BRIGHT_GREEN, '@');
                    break;
                case Tile::EXIT:
                    add_colored_char(C_WHITE, (diamonds_collected_ >= diamonds_required_
                                             ? (anim_even ? ' ' : '#') : '#'));
                    break;
            }
        }
        frame += '\n';
    }

    // One syscall for the entire frame
    fwrite(frame.data(), 1, frame.size(), stdout);
    fflush(stdout);

    std::cout << "Diamonds: " << diamonds_collected_ << '/' << diamonds_required_
              << "  [WASD] Move  [Q] Quit\n";
}

} // namespace consoledash
