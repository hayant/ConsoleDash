#include "InputHelper.h"

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#endif

namespace consoledash {

#if !defined(_WIN32) && !defined(_WIN64)
namespace {
struct termios s_saved;
bool s_raw = false;
} // namespace
#endif

void InputHelper::init_input() {
#if defined(_WIN32) || defined(_WIN64)
    // No terminal mode setup required on Windows.
#else
    if (!isatty(STDIN_FILENO)) return;
    tcgetattr(STDIN_FILENO, &s_saved);
    struct termios raw = s_saved;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    s_raw = true;
#endif
}

void InputHelper::restore_input() {
#if defined(_WIN32) || defined(_WIN64)
    // Nothing to restore on Windows.
#else
    if (s_raw && isatty(STDIN_FILENO))
        tcsetattr(STDIN_FILENO, TCSANOW, &s_saved);
    s_raw = false;
#endif
}

int InputHelper::get_key_nonblock() {
#if defined(_WIN32) || defined(_WIN64)
    if (!_kbhit()) return 0;
    return _getch();
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = { 0, 0 };
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0)
        return 0;
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return 0;
    return c;
#endif
}

int InputHelper::poll_key_nonblock() {
    return get_key_nonblock();
}

void InputHelper::sample_input(int& dx, int& dy, bool& reach, bool& quit) {
    dx = 0;
    dy = 0;
    reach = false;
    quit = false;

    constexpr auto reach_arm_timeout = std::chrono::milliseconds(350);
    const auto now = std::chrono::steady_clock::now();
    if (now - reach_armed_at_ > reach_arm_timeout) {
        reach_armed_ = false;
    }

    int key = 0;
    while ((key = get_key_nonblock()) != 0) {
        if (key == 'q' || key == 'Q') {
            quit = true;
            return;
        }
        if (key == ' ') {
            reach_armed_ = true;
            reach_armed_at_ = std::chrono::steady_clock::now();
            continue;
        }

        int ndx = 0, ndy = 0;
        if (key == 'w' || key == 'W') ndy = -1;
        else if (key == 's' || key == 'S') ndy = 1;
        else if (key == 'a' || key == 'A') ndx = -1;
        else if (key == 'd' || key == 'D') ndx = 1;
        else continue;

        dx = ndx;
        dy = ndy;
        reach = reach_armed_;
        reach_armed_ = false;
    }
}

} // namespace consoledash
