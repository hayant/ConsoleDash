#ifndef INPUTHELPER_H
#define INPUTHELPER_H

#include <chrono>

namespace consoledash {

class InputHelper {
public:
    void init_input();
    void restore_input();
    int poll_key_nonblock();
    void sample_input(int& dx, int& dy, bool& reach, bool& quit);

private:
    int get_key_nonblock();
    bool reach_armed_ = false;
    std::chrono::steady_clock::time_point reach_armed_at_ = std::chrono::steady_clock::now();
};

} // namespace consoledash

#endif // INPUTHELPER_H
