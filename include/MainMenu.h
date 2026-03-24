#ifndef MAINMENU_H
#define MAINMENU_H

#include "InputHelper.h"
#include <string>
#include <vector>

namespace consoledash {

enum class MainMenuResult : int { Play, Quit };

struct MainMenuSelectionResult {
    MainMenuResult result = MainMenuResult::Quit;
    std::string selected_level_path;
};

class MainMenu {
public:
    MainMenuSelectionResult show(InputHelper& input_helper) const;

private:
    enum class Selection : int { Play = 0, Help = 1, Quit = 2 };
    struct LevelEntry {
        std::string path;
        std::string display_label;
    };

    static void clear_terminal();
    static void render_main_menu(Selection selection);
    static void render_help_screen();
    static void render_level_select(const std::vector<LevelEntry>& levels, int selected_index);
    static Selection next_selection(Selection selection);
    static Selection previous_selection(Selection selection);
    static std::vector<LevelEntry> discover_levels();
    static std::string extract_level_name(const std::string& path);
    static bool show_level_select(InputHelper& input_helper, std::string& selected_level_path);
};

} // namespace consoledash

#endif // MAINMENU_H
