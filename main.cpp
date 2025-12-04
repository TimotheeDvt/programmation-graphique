#include <iostream>
#include <filesystem>
#include "src/Application.h"

const char* APP_TITLE = "Minecraft Clone - OpenGL Demo";
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

Model endermanModel (
    glm::vec3(5.0f, 19.5f, 5.0f), // Position
    glm::vec3(0.06f, 0.06f, 0.06f), // Échelle
    0.0f, // angle de rotation
    glm::vec3(0.0f, 1.0f, 0.0f), // axe de rotation
    "./models/EnderMan.obj", // mesh
    "./models/enderman.png" // texture
);

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