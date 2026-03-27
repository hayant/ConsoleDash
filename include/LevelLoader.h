#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include "ConsoleDash.h"
#include <optional>
#include <string>

namespace consoledash {

struct LevelParameters {
    std::string NAME;
    std::optional<int> TIME;
    std::optional<int> DIAMONDS_REQUIRED;
    std::optional<int> AMOEBA_MAX_SIZE;
    std::optional<int> AMOEBA_GROWTH_FACTOR;
    std::optional<int> MAGIC_WALL_DURATION;
    std::optional<int> SLIME_PERMEABILITY_VALUE;
    std::optional<int> GAME_TICK_INTERVAL;
    std::optional<int> ANIMATION_TICK_INTERVAL;
};

class LevelLoader {
public:
    bool load_from_file(const std::string& path, ConsoleDash& game, LevelParameters& parameters) const;
    void build_test_level(ConsoleDash& game) const;
};

} // namespace consoledash

#endif // LEVELLOADER_H
