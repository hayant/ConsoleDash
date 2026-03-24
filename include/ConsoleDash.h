#ifndef CONSOLEDASH_H
#define CONSOLEDASH_H

#include <cstdint>
#include <cstddef>
#include <atomic>
#include <chrono>
#include <mutex>

namespace consoledash {

constexpr int WIDTH = 100;   // maximum supported level width
constexpr int HEIGHT = 50;   // maximum supported level height
constexpr int DEFAULT_WIDTH = 40;
constexpr int DEFAULT_HEIGHT = 24;

enum class Tile : uint8_t {
    SPACE,
    DIRT,
    WALL,
    EXPLOSION,
    ROCK,
    DIAMOND,
    FIREFLY,
    BUTTERFLY,
    AMOEBA,
    MAGIC_WALL,
    ROCKFORD,
    EXIT,
    TITANIUM_WALL,
};

// 0=Up, 1=Left, 2=Down, 3=Right (CCW order for firefly; butterfly uses CW)
enum class Direction : uint8_t { UP = 0, LEFT = 1, DOWN = 2, RIGHT = 3 };

struct Cell {
    Tile tile = Tile::SPACE;
    uint8_t facing = 0;
    bool was_falling = false;
    int magic_timer = 0;
    uint8_t explosion_stage = 0; // 0..2 while in explosion state
    Tile explosion_source = Tile::SPACE; // source creature for explosion styling
    Tile explosion_result = Tile::SPACE; // final tile when explosion ends
};

class ConsoleDash {
public:
    ConsoleDash();

    void tick();
    void set_input(int dx, int dy, bool reach = false);
    void advance_animation();
    void render() const;
    bool set_level_size(int width, int height);
    int level_width() const { return level_width_; }
    int level_height() const { return level_height_; }

    int rockford_x() const { return rockford_x_; }
    int rockford_y() const { return rockford_y_; }
    int diamonds_collected() const { return diamonds_collected_; }
    int diamonds_required() const { return diamonds_required_; }
    int time_remaining() const { return time_remaining_; }
    int time_limit() const { return time_limit_; }
    bool game_over() const { return game_over_; }
    bool player_wins() const { return player_wins_; }
    bool is_alive() const { return !game_over_ && !player_wins_; }

    void set_diamonds_required(int n) { diamonds_required_ = n; }
    void set_time_limit(int seconds);
    void set_amoeba_max_size(int max_size);
    void set_magic_wall_duration(int duration_ticks);
    void set_cell(int x, int y, Tile t, uint8_t facing = 0);
    void set_rockford(int x, int y);
    void set_exit(int x, int y);

private:
    Cell grid_[WIDTH][HEIGHT];
    bool moved_[WIDTH][HEIGHT];
    int rockford_x_ = 0;
    int rockford_y_ = 0;
    int pending_dx_ = 0;
    int pending_dy_ = 0;
    bool pending_reach_ = false;
    int diamonds_collected_ = 0;
    int diamonds_required_ = 0;
    int time_limit_ = 255;
    int time_remaining_ = 255;
    std::chrono::steady_clock::time_point last_time_update_ = std::chrono::steady_clock::now();
    bool game_over_ = false;
    bool player_wins_ = false;
    int level_width_ = DEFAULT_WIDTH;
    int level_height_ = DEFAULT_HEIGHT;
    std::atomic<std::uint64_t> animation_counter_{0};
    mutable std::mutex state_mutex_;

    int amoeba_current_size_ = 0;
    int amoeba_max_size_ = 150;
    int magic_wall_duration_ = 200;

    bool in_bounds(int x, int y) const;
    bool is_space(int x, int y) const; // moving objects can only enter empty space
    bool is_empty_or_dirt(int x, int y) const;
    bool is_blocking(int x, int y) const;
    bool can_roll_over(int x, int y) const;
    bool can_roll_into(int x, int y) const;
    void mark_moved(int x, int y);
    bool was_moved(int x, int y) const;
    void set_cell_internal(int x, int y, Tile t, uint8_t facing = 0, bool falling = false, int magic_timer = 0);
    void clear_cell(int x, int y);

    void process_cell(int x, int y);
    void process_rock_or_diamond(int x, int y);
    void process_firefly(int x, int y);
    void process_butterfly(int x, int y);
    void process_amoeba(int x, int y);
    void process_magic_wall(int x, int y);
    void process_rockford(int x, int y);

    void explode_firefly(int x, int y);
    void explode_butterfly(int x, int y);
    void explode_at(int cx, int cy, Tile source, Tile fill);
    void advance_explosions();

    void post_tick_amoeba();
    void try_move_rockford(int dx, int dy);
    void try_reach_rockford(int dx, int dy);
};

} // namespace consoledash

#endif // CONSOLEDASH_H
