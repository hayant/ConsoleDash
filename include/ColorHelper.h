#ifndef COLORHELPER_H
#define COLORHELPER_H


namespace consoledash::Colors {

constexpr const char* RESET           = "\033[0m";
constexpr const char* C_WHITE         = "\033[37m";
constexpr const char* C_GRAY          = "\033[90m";
constexpr const char* C_BLUE          = "\033[34m";
constexpr const char* C_MAGENTA       = "\033[35m";
constexpr const char* C_BRIGHT_GREEN  = "\033[92m";
constexpr const char* C_CYAN          = "\033[36m";
constexpr const char* C_BRIGHT_CYAN   = "\033[96m";
constexpr const char* C_LIGHT_BLUE    = "\033[94m";
constexpr const char* C_BRIGHT_YELLOW = "\033[93m";
constexpr const char* C_DIM_YELLOW    = "\033[2;33m";

} // namespace consoledash::Colors


#endif // COLORHELPER_H
