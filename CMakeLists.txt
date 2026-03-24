cmake_minimum_required(VERSION 4.1)
project(ConsoleDash)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core game/engine library (Pitchfork: src/ + include/)
add_library(ConsoleDashLib
    src/ConsoleDash.cpp
    src/ConsoleDashLogic.cpp
    src/ConsoleDashRender.cpp
    src/LevelLoader.cpp
)

target_include_directories(ConsoleDashLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Main executable links against the library
add_executable(ConsoleDash
    src/main.cpp
)

target_link_libraries(ConsoleDash
    PRIVATE
        ConsoleDashLib
)

enable_testing()

add_executable(ConsoleDashTests
    tests/main_tests.cpp
)

target_link_libraries(ConsoleDashTests
    PRIVATE
        ConsoleDashLib
)

add_test(NAME ConsoleDashBasicTests COMMAND ConsoleDashTests)
