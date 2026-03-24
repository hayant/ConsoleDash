#include "MainMenu.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

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
} // namespace

void MainMenu::clear_terminal() {
#if defined(_WIN32) || defined(_WIN64)
    std::system("cls");
#else
    std::system("clear");
#endif
}

void MainMenu::render_main_menu(Selection selection) {
    clear_terminal();
    const char* play_mark = (selection == Selection::Play) ? ">" : " ";
    const char* help_mark = (selection == Selection::Help) ? ">" : " ";
    const char* quit_mark = (selection == Selection::Quit) ? ">" : " ";

    std::cout
        << "########################################\n"
        << "#                                      #\n"
        << "#  #### #### #  # #### #### #    ####  #\n"
        << "#  #    #  # ## # #    #  # #    #     #\n"
        << "#  #    #  # # ## #### #  # #    ###   #\n"
        << "#  #    #  # #  #    # #  # #    #     #\n"
        << "#  #### #### #  # #### #### #### ####  #\n"
        << "#                                      #\n"
        << "#         ###  #### #### #  #          #\n"
        << "#         #  # #  # #    #  #          #\n"
        << "#         #  # #### #### ####          #\n"
        << "#         #  # #  #    # #  #          #\n"
        << "#         ###  #  # #### #  #          #\n"
        << "#                                      #\n"
        << "#                                      #\n"
        << "#                                      #\n"
        << "#               " << play_mark << " Play                 #\n"
        << "#                                      #\n"
        << "#               " << help_mark << " Help                 #\n"
        << "#                                      #\n"
        << "#               " << quit_mark << " Quit                 #\n"
        << "#                                      #\n"
        << "#      [W/S] Move  [Enter] Select      #\n"
        << "########################################\n";
}

void MainMenu::render_help_screen() {
    clear_terminal();
    std::cout
        << "########################################\n"
        << "#                 HELP                 #\n"
        << "#                                      #\n"
        << "#                                      #\n"
        << "#                                      #\n"
        << "#                                      #\n"
        << "#                                      #\n"
        << "#        Press [Enter] to return       #\n"
        << "########################################\n";
}

void MainMenu::render_level_select(const std::vector<LevelEntry>& levels, int selected_index) {
    clear_terminal();
    constexpr int width = 40;
    constexpr int height = 24;
    constexpr int inner_width = width - 2;

    std::vector<std::string> lines(height, std::string(inner_width, ' '));
    lines[0] = std::string(inner_width, '#');
    lines[height - 1] = std::string(inner_width, '#');

    auto set_line = [&](int row, const std::string& content) {
        if (row <= 0 || row >= height - 1) return;
        std::string line(inner_width, ' ');
        line[0] = '#';
        line[inner_width - 1] = '#';
        const int max_content = inner_width - 2;
        std::string clipped = content.substr(0, static_cast<size_t>(max_content));
        for (size_t i = 0; i < clipped.size(); ++i) line[i + 1] = clipped[i];
        lines[row] = line;
    };

    set_line(1, "            SELECT LEVEL");
    set_line(2, "");

    const int list_start = 3;
    const int list_end = 20;
    int row = list_start;
    for (size_t i = 0; i < levels.size() && row <= list_end; ++i, ++row) {
        const char mark = (static_cast<int>(i) == selected_index) ? '>' : ' ';
        set_line(row, std::string(1, mark) + " " + levels[i].display_label);
    }

    if (levels.empty()) {
        set_line(10, "No level files found in levels/");
    }
    set_line(22, "      [W/S] Move  [Enter] Select");

    for (const std::string& line : lines) {
        std::cout << line << '\n';
    }
}

MainMenu::Selection MainMenu::next_selection(Selection selection) {
    switch (selection) {
        case Selection::Play: return Selection::Help;
        case Selection::Help: return Selection::Quit;
        case Selection::Quit: return Selection::Play;
    }
    return Selection::Play;
}

MainMenu::Selection MainMenu::previous_selection(Selection selection) {
    switch (selection) {
        case Selection::Play: return Selection::Quit;
        case Selection::Help: return Selection::Play;
        case Selection::Quit: return Selection::Help;
    }
    return Selection::Play;
}

std::string MainMenu::extract_level_name(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return "";

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);

    int parameters_start = static_cast<int>(lines.size());
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        if (trim_copy(lines[i]).empty()) {
            parameters_start = i + 1;
            break;
        }
    }
    if (parameters_start >= static_cast<int>(lines.size())) return "";

    for (int i = parameters_start; i < static_cast<int>(lines.size()); ++i) {
        const std::string entry = trim_copy(lines[i]);
        if (entry.empty()) continue;
        const size_t sep = entry.find(':');
        if (sep == std::string::npos) continue;
        const std::string key = uppercase_copy(trim_copy(entry.substr(0, sep)));
        if (key != "NAME") continue;
        std::string value = trim_copy(entry.substr(sep + 1));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        return value;
    }
    return "";
}

std::vector<MainMenu::LevelEntry> MainMenu::discover_levels() {
    std::vector<LevelEntry> levels;
    const std::filesystem::path levels_dir("../levels");
    if (!std::filesystem::exists(levels_dir) || !std::filesystem::is_directory(levels_dir)) {
        return levels;
    }

    for (const auto& entry : std::filesystem::directory_iterator(levels_dir)) {
        if (!entry.is_regular_file()) continue;
        const std::filesystem::path p = entry.path();
        if (p.extension() != ".txt") continue;
        const std::string stem = p.stem().string();
        const std::string name = extract_level_name(p.string());
        LevelEntry level;
        level.path = p.string();
        level.display_label = stem + ": " + (name.empty() ? "-" : name);
        levels.push_back(level);
    }

    std::sort(levels.begin(), levels.end(), [](const LevelEntry& a, const LevelEntry& b) {
        return a.path < b.path;
    });
    return levels;
}

bool MainMenu::show_level_select(InputHelper& input_helper, std::string& selected_level_path) {
    std::vector<LevelEntry> levels = discover_levels();
    int selected_index = 0;
    render_level_select(levels, selected_index);
    while (true) {
        const int key = input_helper.poll_key_nonblock();
        if (key == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        if (key == 'q' || key == 'Q') {
            return false;
        }
        if (!levels.empty() && (key == 'w' || key == 'W')) {
            selected_index = (selected_index - 1 + static_cast<int>(levels.size())) % static_cast<int>(levels.size());
            render_level_select(levels, selected_index);
            continue;
        }
        if (!levels.empty() && (key == 's' || key == 'S')) {
            selected_index = (selected_index + 1) % static_cast<int>(levels.size());
            render_level_select(levels, selected_index);
            continue;
        }
        if (key == '\n' || key == '\r') {
            if (levels.empty()) return false;
            selected_level_path = levels[selected_index].path;
            return true;
        }
    }
}

MainMenuSelectionResult MainMenu::show(InputHelper& input_helper) const {
    Selection selection = Selection::Play;
    render_main_menu(selection);
    while (true) {
        const int key = input_helper.poll_key_nonblock();
        if (key == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        if (key == 'q' || key == 'Q') {
            clear_terminal();
            return {MainMenuResult::Quit, ""};
        }
        if (key == 'w' || key == 'W') {
            selection = previous_selection(selection);
            render_main_menu(selection);
            continue;
        }
        if (key == 's' || key == 'S') {
            selection = next_selection(selection);
            render_main_menu(selection);
            continue;
        }
        if (key == '\n' || key == '\r') {
            if (selection == Selection::Play) {
                std::string selected_level_path;
                if (show_level_select(input_helper, selected_level_path)) {
                    return {MainMenuResult::Play, selected_level_path};
                }
                render_main_menu(selection);
                continue;
            }
            if (selection == Selection::Help) {
                render_help_screen();
                while (true) {
                    const int help_key = input_helper.poll_key_nonblock();
                    if (help_key == '\n' || help_key == '\r') {
                        render_main_menu(selection);
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                }
                continue;
            }
            clear_terminal();
            return {MainMenuResult::Quit, ""};
        }
    }
}

} // namespace consoledash
