#ifndef CONSOLEDASH_H
#define CONSOLEDASH_H

#include <cstdint>
#include <cstddef>

namespace consoledash {

constexpr int WIDTH = 40;
constexpr int HEIGHT = 24;

enum class Tile : uint8_t {
    SPACE,
    DIRT,
    WALL,
    ROCK,
    DIAMOND,
    FIREFLY,
    BUTTERFLY,
    AMOEBA,
    MAGIC_WALL,
    ROCKFORD,
    EXIT,
};

// 0=Up, 1=Left, 2=Down, 3=Right (CCW order for firefly; butterfly uses CW)
enum class Direction : uint8_t { UP = 0, LEFT = 1, DOWN = 2, RIGHT = 3 };

struct Cell {
    Tile tile = Tile::SPACE;
    uint8_t facing = 0;
    bool was_falling = false;
    int magic_timer = 0;
};

class ConsoleDash {
public:
    ConsoleDash();

    void tick();
    void set_input(int dx, int dy, bool reach = false);
    void render() const;

    int rockford_x() const { return rockford_x_; }
    int rockford_y() const { return rockford_y_; }
    int diamonds_collected() const { return diamonds_collected_; }
    int diamonds_required() const { return diamonds_required_; }
    bool game_over() const { return game_over_; }
    bool player_wins() const { return player_wins_; }
    bool is_alive() const { return !game_over_ && !player_wins_; }

    void set_diamonds_required(int n) { diamonds_required_ = n; }
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
    bool game_over_ = false;
    bool player_wins_ = false;

    int amoeba_growth_counter_ = 0;
    static constexpr int AMOEBA_GROWTH_INTERVAL = 4;
    static constexpr int AMOEBA_MAX_SIZE = 150;
    static constexpr int MAGIC_WALL_DURATION = 200;

    bool in_bounds(int x, int y) const;
    bool is_space(int x, int y) const; // moving objects can only enter empty space
    bool is_empty_or_dirt(int x, int y) const;
    bool is_blocking(int x, int y) const;
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
    void explode_at(int cx, int cy, Tile fill);

    void post_tick_amoeba();
    void try_move_rockford(int dx, int dy);
    void try_reach_rockford(int dx, int dy);
};

} // namespace consoledash

#endif // CONSOLEDASH_H
