#include "MainMenu.h"
#include "ColorHelper.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <atomic>
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

void MainMenu::render_help_screen(int anim_counter) {
    clear_terminal();

    using namespace Colors;

    // '#' + content (padded/clipped to 38 visible chars) + '#'
    auto make_row = [](const std::string& content) {
        const int len = static_cast<int>(content.size());
        std::string row = "#";
        row += content.substr(0, std::min(len, 38));
        row += std::string(std::max(0, 38 - len), ' ');
        row += "#";
        return row;
    };

    // Three-column entry row. Visible positions within the 38 inner chars:
    //   1-2:  indent
    //   3:    colored game char  (aligns with "CHR" header at pos 3)
    //   4-7:  gap
    //   8:    level-editor char  (aligns with "EDIT" header at pos 8)
    //   9-13: gap
    //   14-38: description + trailing spaces (aligns with "DESCRIPTION" header at pos 14)
    auto make_entry = [&](const std::string& colored_game, char edit, const std::string& desc) {
        const int trailing = 25 - static_cast<int>(desc.size());
        std::string row = "#  ";
        row += colored_game;   // ANSI + char + RESET (1 visible char)
        row += "    ";         // pos 4-7
        row += edit;           // pos 8
        row += "     ";        // pos 9-13
        row += desc;
        if (trailing > 0) row += std::string(trailing, ' ');
        row += "#";
        return row;
    };

    const bool anim_even = (anim_counter % 2) == 0;
    const int anim_frame = anim_counter % 3;
    const char firefly_ch  = (anim_frame == 0) ? '|' : (anim_frame == 1) ? '<' : '>';
    const char butterfly_ch = (anim_frame == 0) ? '|' : (anim_frame == 1) ? '(' : ')';
    const char amoeba_ch   = anim_even ? '~' : '-';

    auto gc = [](const char* color, char ch) -> std::string {
        return std::string(color) + ch + RESET;
    };
    auto gs = [](const char* color, const std::string& s) -> std::string {
        return std::string(color) + s + RESET;
    };

    const std::string border(40, '#');
    std::cout
        << border                                                                   << "\n"
        << make_row("                 HELP")                                        << "\n"
        << make_row("")                                                             << "\n"
        << make_row("  CONTROLS")                                                   << "\n"
        << make_row("  W/A/S/D        Move Rockford")                              << "\n"
        << make_row("  Space+W/A/S/D  Reach in direction")                         << "\n"
        << make_row("  Q              Back to menu")                                << "\n"
        << make_row("")                                                             << "\n"
        << make_row("  LEVEL ELEMENTS")                                             << "\n"
        << make_row("  CHR  EDIT  DESCRIPTION")                                    << "\n"
        << make_entry(gc(C_BRIGHT_GREEN,  '@'), '@', "Rockford (player)")          << "\n"
        << make_entry(gc(C_WHITE,         '#'), '#', "Titanium wall")              << "\n"
        << make_entry(gc(C_BLUE,          '%'), 'W', "Destructible wall")          << "\n"
        << make_entry(gc(C_GRAY,          'O'), 'R', "Rock")                       << "\n"
        << make_entry(gc(anim_even ? C_BRIGHT_CYAN : C_CYAN,   '*'), 'D', "Diamond")   << "\n"
        << make_entry(gc(C_BRIGHT_YELLOW, firefly_ch),  'F', "Firefly")            << "\n"
        << make_entry(gc(C_MAGENTA,       butterfly_ch), 'B', "Butterfly")         << "\n"
        << make_entry(gc(C_BRIGHT_GREEN,  amoeba_ch),   'A', "Amoeba")             << "\n"
        << make_entry(gs(C_BLUE, anim_even ? "%" : "°"), 'M', "Magic wall")         << "\n"
        << make_entry(gc(C_WHITE, anim_even ? '#' : ' '), 'E', "Exit")                       << "\n"
        << make_entry(gs(C_DIM_YELLOW,    "·"), '.', "Dirt")                       << "\n"
        << make_entry(std::string(1, ' '),      ' ', "Empty space")                << "\n"
        << make_row("")                                                             << "\n"
        << make_row("      Press [Enter] to return")                               << "\n"
        << border                                                                   << "\n";
}

void MainMenu::render_level_select(
    const std::vector<LevelEntry>& levels,
    const int selected_index,
    const std::string& levels_path,
    const int scroll_offset) {

    clear_terminal();
    constexpr int inner_width = 38;

    auto make_border = []() { return std::string(inner_width, '#'); };
    auto make_row = [](const std::string& content) {
        std::string line(inner_width, ' ');
        line[0] = '#';
        line[inner_width - 1] = '#';
        const std::string clipped = content.substr(0, inner_width - 2);
        for (size_t i = 0; i < clipped.size(); ++i) line[i + 1] = clipped[i];
        return line;
    };

    std::vector<std::string> lines;
    lines.push_back(make_border());
    lines.push_back(make_row("            SELECT LEVEL"));
    lines.push_back(make_row(""));
    const char path_mark = (selected_index == 0) ? '>' : ' ';
    lines.push_back(make_row("  " + std::string(1, path_mark) + " Path: " + levels_path));
    lines.push_back(make_row(""));

    if (levels.empty()) {
        lines.push_back(make_row("  No level files found."));
    } else {
        constexpr int max_visible = 20;
        const int total = static_cast<int>(levels.size());
        const int visible_count = std::min(max_visible, total - scroll_offset);
        lines.push_back(make_row(scroll_offset > 0 ? "  ^ ..." : ""));
        // if (scroll_offset > 0)
        //     lines.push_back(make_row("  ^ ..."));
        for (int i = 0; i < visible_count; ++i) {
            const int level_idx = scroll_offset + i;
            const char mark = (level_idx + 1 == selected_index) ? '>' : ' ';
            lines.push_back(make_row("  " + std::string(1, mark) + " " + levels[level_idx].display_label));
        }
        lines.push_back(make_row(scroll_offset + visible_count < total ? "  v ..." : ""));
        // if (scroll_offset + visible_count < total)
        //     lines.push_back(make_row("  v ..."));
    }

    lines.push_back(make_row(""));
    lines.push_back(make_row("      [W/S] Move  [Enter] Select"));
    lines.push_back(make_border());

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

std::vector<MainMenu::LevelEntry> MainMenu::discover_levels(const std::string& levels_path) {
    std::vector<LevelEntry> levels;
    const std::filesystem::path levels_dir(levels_path);
    if (!std::filesystem::exists(levels_dir) || !std::filesystem::is_directory(levels_dir)) {
        return levels;
    }

    for (const auto& entry : std::filesystem::directory_iterator(levels_dir)) {
        if (!entry.is_regular_file()) continue;
        const std::filesystem::path& p = entry.path();
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

std::string MainMenu::prompt_levels_path(InputHelper& input_helper, const std::string& current_path) {
    std::string path = current_path;
    while (true) {
        clear_terminal();
        std::cout
            << "########################################\n"
            << "#            SELECT LEVEL              #\n"
            << "#                                      #\n"
            << "# Enter levels path and press [Enter]  #\n"
            << "# Press [Q] to cancel                  #\n"
            << "#                                      #\n";

        std::string shown = "Path: " + path;
        if (shown.size() > 38) {
            shown = shown.substr(shown.size() - 38);
        }
        std::cout << "#" << shown << std::string(38 - shown.size(), ' ') << "#\n";
        std::cout << "#                                      #\n"
                  << "########################################\n";

        const int key = input_helper.poll_key_nonblock();
        if (key == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        if (key == 'q' || key == 'Q') {
            return current_path;
        }
        if (key == '\n' || key == '\r') {
            return path.empty() ? current_path : path;
        }
        if (key == 127 || key == 8) {
            if (!path.empty()) path.pop_back();
            continue;
        }
        if (key >= 32 && key <= 126) {
            path.push_back(static_cast<char>(key));
        }
    }
}

bool MainMenu::show_level_select(InputHelper& input_helper, std::string& selected_level_path) {
    constexpr int max_visible = 20;
    std::string levels_path = "../levels";
    std::vector<LevelEntry> levels = discover_levels(levels_path);
    int selected_index = 0; // 0 = path line, 1..N = level rows
    int scroll_offset = 0;

    auto update_scroll = [&]() {
        if (selected_index == 0) return;
        const int level_idx = selected_index - 1;
        if (level_idx < scroll_offset)
            scroll_offset = level_idx;
        else if (level_idx >= scroll_offset + max_visible)
            scroll_offset = level_idx - max_visible + 1;
    };

    render_level_select(levels, selected_index, levels_path, scroll_offset);
    while (true) {
        const int key = input_helper.poll_key_nonblock();
        if (key == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        if (key == 'q' || key == 'Q') {
            return false;
        }
        const int item_count = static_cast<int>(levels.size()) + 1;
        if (key == 'w' || key == 'W') {
            selected_index = (selected_index - 1 + item_count) % item_count;
            update_scroll();
            render_level_select(levels, selected_index, levels_path, scroll_offset);
            continue;
        }
        if (key == 's' || key == 'S') {
            selected_index = (selected_index + 1) % item_count;
            update_scroll();
            render_level_select(levels, selected_index, levels_path, scroll_offset);
            continue;
        }
        if (key == '\n' || key == '\r') {
            if (selected_index == 0) {
                levels_path = prompt_levels_path(input_helper, levels_path);
                levels = discover_levels(levels_path);
                selected_index = 0;
                scroll_offset = 0;
                render_level_select(levels, selected_index, levels_path, scroll_offset);
                continue;
            }
            if (levels.empty()) {
                continue;
            }
            selected_level_path = levels[static_cast<size_t>(selected_index - 1)].path;
            return true;
        }
    }
}

MainMenuSelectionResult MainMenu::show(InputHelper& input_helper) {
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
                std::atomic<bool> anim_running{true};
                int anim_counter = 0;
                constexpr auto anim_interval = std::chrono::milliseconds(200);
                std::thread anim_thread([&] {
                    auto next_tick = std::chrono::steady_clock::now();
                    while (anim_running.load(std::memory_order_relaxed)) {
                        render_help_screen(anim_counter++);
                        next_tick += anim_interval;
                        std::this_thread::sleep_until(next_tick);
                    }
                });
                while (true) {
                    const int help_key = input_helper.poll_key_nonblock();
                    if (help_key == '\n' || help_key == '\r') {
                        anim_running.store(false, std::memory_order_relaxed);
                        anim_thread.join();
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
