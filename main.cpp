#include <iostream>
#include <filesystem>
#include "src/Application.h"

const char* APP_TITLE = "Minecraft Clone - OpenGL Demo";
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

int main() {
    std::cout << "CWD: " << std::filesystem::current_path() << std::endl;

    try {
        Application app(APP_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}