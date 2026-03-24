#ifndef MAINMENU_H
#define MAINMENU_H

#include "InputHelper.h"

namespace consoledash {

enum class MainMenuResult : int { Play, Quit };

class MainMenu {
public:
    MainMenuResult show(InputHelper& input_helper) const;

private:
    enum class Selection : int { Play = 0, Help = 1, Quit = 2 };

    static void clear_terminal();
    static void render_main_menu(Selection selection);
    static void render_help_screen();
    static Selection next_selection(Selection selection);
    static Selection previous_selection(Selection selection);
};

} // namespace consoledash

#endif // MAINMENU_H
