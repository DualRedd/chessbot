#include "core/game.hpp"
#include "interface/sfml_gui.hpp"
#include <iostream>

int main() {
    Game game;

    // Example boards
    std::cout << "\n" << game.get_board().to_string();
    game.make_move(0, 1, 0, 3); // white pawn a2 → a4
    std::cout << "\n" << game.get_board().to_string();

    // Example interface
    ChessGUI interface(game, 800, 800);
    interface.run();
}