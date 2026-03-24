#include "MainMenu.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace consoledash {

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

MainMenuResult MainMenu::show(InputHelper& input_helper) const {
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
            return MainMenuResult::Quit;
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
                return MainMenuResult::Play;
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
            return MainMenuResult::Quit;
        }
    }
}

} // namespace consoledash
