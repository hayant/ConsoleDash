#include "LevelLoader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <vector>

namespace consoledash {

namespace {

std::string trim_copy(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

std::string uppercase_copy(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::optional<int> parse_positive_int(const std::string& s) {
    try {
        size_t parsed = 0;
        int value = std::stoi(s, &parsed);
        if (parsed != s.size() || value <= 0) return std::nullopt;
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

void parse_level_parameters(const std::vector<std::string>& parameter_lines, LevelParameters& parameters) {
    for (const std::string& raw_line : parameter_lines) {
        const std::string line = trim_copy(raw_line);
        if (line.empty()) continue;

        const size_t sep = line.find(':');
        if (sep == std::string::npos) continue;

        const std::string key = uppercase_copy(trim_copy(line.substr(0, sep)));
        std::string value = trim_copy(line.substr(sep + 1));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        if (key == "NAME") {
            parameters.NAME = value;
            continue;
        }

        const std::optional<int> parsed_value = parse_positive_int(value);
        if (!parsed_value.has_value()) continue;

        if (key == "TIME") parameters.TIME = *parsed_value;
        else if (key == "AMOEBA_MAX_SIZE") parameters.AMOEBA_MAX_SIZE = *parsed_value;
        else if (key == "MAGIC_WALL_DURATION") parameters.MAGIC_WALL_DURATION = *parsed_value;
        else if (key == "GAME_TICK_INTERVAL") parameters.GAME_TICK_INTERVAL = *parsed_value;
        else if (key == "ANIMATION_TICK_INTERVAL") parameters.ANIMATION_TICK_INTERVAL = *parsed_value;
    }
}

} // namespace

bool LevelLoader::load_from_file(const std::string& path, ConsoleDash& game, LevelParameters& parameters) const {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    int parameters_start = static_cast<int>(lines.size());
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        if (trim_copy(lines[i]).empty()) {
            parameters_start = i + 1;
            break;
        }
    }

    std::vector<std::string> map_lines(lines.begin(), lines.begin() + std::min(parameters_start, static_cast<int>(lines.size())));
    std::vector<std::string> parameter_lines;
    if (parameters_start < static_cast<int>(lines.size())) {
        parameter_lines.assign(lines.begin() + parameters_start, lines.end());
    }
    parse_level_parameters(parameter_lines, parameters);

    int level_height = static_cast<int>(map_lines.size());
    int level_width = 0;
    for (const std::string& ln : map_lines) {
        int w = static_cast<int>(ln.size());
        if (w > level_width) level_width = w;
    }

    if (level_width <= 0 || level_height <= 0) {
        return false;
    }
    if (level_width > WIDTH || level_height > HEIGHT) {
        std::cerr << "Error: Level size " << level_width << "x" << level_height
                  << " exceeds maximum " << WIDTH << "x" << HEIGHT << '\n';
        return false;
    }
    if (!game.set_level_size(level_width, level_height)) {
        return false;
    }

    bool has_rockford = false;
    for (int y = 0; y < level_height; ++y) {
        for (int x = 0; x < level_width; ++x) {
            char ch = ' ';
            if (y < static_cast<int>(map_lines.size()) && x < static_cast<int>(map_lines[y].size())) {
                ch = map_lines[y][x];
            }

            switch (ch) {
                case '@':
                    game.set_cell(x, y, Tile::SPACE);
                    game.set_rockford(x, y);
                    has_rockford = true;
                    break;
                case '#':
                    game.set_cell(x, y, Tile::TITANIUM_WALL);
                    break;
                case 'W':
                    game.set_cell(x, y, Tile::WALL);
                    break;
                case 'R':
                    game.set_cell(x, y, Tile::ROCK);
                    break;
                case 'F':
                    game.set_cell(x, y, Tile::FIREFLY);
                    break;
                case 'B':
                    game.set_cell(x, y, Tile::BUTTERFLY);
                    break;
                case 'A':
                    game.set_cell(x, y, Tile::AMOEBA);
                    break;
                case 'M':
                    game.set_cell(x, y, Tile::MAGIC_WALL);
                    break;
                case 'E':
                    game.set_cell(x, y, Tile::EXIT);
                    break;
                case ' ':
                    game.set_cell(x, y, Tile::SPACE);
                    break;
                case '.':
                    game.set_cell(x, y, Tile::DIRT);
                    break;
                case 'D':
                    game.set_cell(x, y, Tile::DIAMOND);
                    break;
                default:
                    game.set_cell(x, y, Tile::DIRT);
                    break;
            }
        }
    }

    if (!has_rockford) {
        game.set_rockford(1, 1);
    }
    game.set_diamonds_required(3);
    if (parameters.TIME.has_value()) game.set_time_limit(*parameters.TIME);
    if (parameters.AMOEBA_MAX_SIZE.has_value()) game.set_amoeba_max_size(*parameters.AMOEBA_MAX_SIZE);
    if (parameters.MAGIC_WALL_DURATION.has_value()) game.set_magic_wall_duration(*parameters.MAGIC_WALL_DURATION);
    return true;
}

} // namespace consoledash
