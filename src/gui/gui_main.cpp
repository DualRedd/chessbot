#include "gui/chess_gui.hpp"
#include "gui/assets.hpp"

int main() {
    fs::path theme_file = get_executable_dir() / "assets" / "tgui_theme_black.txt";
    tgui::Theme::setDefault(theme_file.string());

    ChessGUI gui(1200, 800);
    gui.run();
}
