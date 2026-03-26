#ifndef COLORHELPER_H
#define COLORHELPER_H

namespace consoledash {

struct ColorHelper {
    static constexpr const char* RESET          = "\033[0m";
    static constexpr const char* C_WHITE        = "\033[37m";
    static constexpr const char* C_GRAY         = "\033[90m";
    static constexpr const char* C_BLUE         = "\033[34m";
    static constexpr const char* C_MAGENTA      = "\033[35m";
    static constexpr const char* C_BRIGHT_GREEN = "\033[92m";
    static constexpr const char* C_BRIGHT_CYAN  = "\033[96m";
    static constexpr const char* C_BRIGHT_YELLOW = "\033[93m";
    static constexpr const char* C_DIM_YELLOW   = "\033[2;33m";
};

} // namespace consoledash

#endif // COLORHELPER_H
