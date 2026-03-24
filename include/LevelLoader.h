#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include "ConsoleDash.h"
#include <optional>
#include <string>

namespace consoledash {

struct LevelParameters {
    std::string NAME;
    std::optional<int> TIME;
    std::optional<int> AMOEBA_MAX_SIZE;
    std::optional<int> MAGIC_WALL_DURATION;
    std::optional<int> GAME_TICK_INTERVAL;
    std::optional<int> ANIMATION_TICK_INTERVAL;
};

class LevelLoader {
public:
    bool load_from_file(const std::string& path, ConsoleDash& game, LevelParameters& parameters) const;
};

} // namespace consoledash

#endif // LEVELLOADER_H
