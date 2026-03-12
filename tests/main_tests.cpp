#include "ConsoleDash.h"
#include <iostream>

int main() {
    consoledash::ConsoleDash game;

    // Basic smoke test: we can tick and render without crashing.
    game.set_diamonds_required(0);
    game.tick();
    game.render();

    std::cout << "ConsoleDash basic test passed.\n";
    return 0;
}

