#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "Scene.h"
#include "Cube.h"
#include "Renderer.h"
#include "DebugDrawer.h"
#include "Texture2D.h"
#include "Mesh.h"

class Application {
public:
    Application(const char* title, int width, int height);
    ~Application();

    void run();

private:
    void init();
    void initResources();
    void mainLoop();
    void cleanup();

    void processInput(double deltaTime);
    void update(double deltaTime);
    void render();

    // Window
    const char* m_title;
    int m_width;
    int m_height;
    GLFWwindow* m_window = nullptr;

    // State
    bool m_wireframe = false;
    bool m_debug = true;
    bool m_isFlying = false;
    BlockType m_selectedBlock = BlockType::TORCH;

    // Timing
    double m_lastTime = 0.0;

    // Scene
    FPSCamera m_camera;
    World m_world;
    Scene m_scene;

    // Resources
    std::map<std::string, std::unique_ptr<Mesh>> m_meshCache;
    std::map<std::string, std::unique_ptr<Texture2D>> m_modelTextureCache;
    std::unique_ptr<Texture2D[]> m_blockTextures;

    // Systems
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<DebugDrawer> m_debugDrawer;

    // Input
    bool m_leftMouseButtonPressed = false;
    bool m_rightMouseButtonPressed = false;

    // Static callbacks
    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mode);
    static void onMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void onMouseMove(GLFWwindow* window, double xpos, double ypos);
    static void onMouseScroll(GLFWwindow* window, double xoffset, double yoffset);
    static void onFramebufferSize(GLFWwindow* window, int width, int height);

    void showFPS();
};