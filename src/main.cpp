#include "game.h"
#include <iostream>
#include <cstdlib>

int main() {
    vd::Game game;
    if (!game.init()) {
        std::cerr << "Failed to initialize Vector Drift\n";
        return 1;
    }
    std::cout << "Vector Drift — fly with inertia. W thrust, mouse look, LMB fire.\n";
    game.run();
    game.shutdown();
    return 0;
}
