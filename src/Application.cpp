#include "Application.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include "Light.h"
#include "Constants.h"

namespace {
    const double ZOOM_SENSITIVITY = -3.0;
    float MOVE_SPEED = 30.0f;
    const float MOUSE_SENSITIVITY = 0.1f;

    RaycastHit raycastWorld(const World& world, const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step = 0.05f) {
        RaycastHit hit;
        hit.hit = false;

        if (step <= 0.0f) step = 0.05f;

        float t = 0.0f;
        glm::ivec3 previousVoxel = glm::floor(origin + glm::vec3(0.5f));

        while (t <= maxDist) {
            t += step;
            glm::vec3 pos = origin + dir * t;
            glm::ivec3 voxel = glm::floor(pos + glm::vec3(0.5f));

            if (voxel != previousVoxel) {
                if (world.getBlockAt(glm::vec3(voxel)) != BlockType::AIR) {
                    hit.hit = true;
                    hit.blockPos = glm::vec3(voxel);
                    hit.hitPos = pos;

                    glm::ivec3 stepDir = voxel - previousVoxel;
                    glm::ivec3 normal(0);
                    if (stepDir.x != 0 || stepDir.y != 0 || stepDir.z != 0) {
                        normal = -stepDir;
                    } else {
                        glm::vec3 local = pos - glm::vec3(voxel);
                        float dx = std::min(local.x, 1.0f - local.x);
                        float dy = std::min(local.y, 1.0f - local.y);
                        float dz = std::min(local.z, 1.0f - local.z);
                        if (dx <= dy && dx <= dz) normal = glm::ivec3((local.x < 0.5f) ? -1 : 1, 0, 0);
                        else if (dy <= dx && dy <= dz) normal = glm::ivec3(0, (local.y < 0.5f) ? -1 : 1, 0);
                        else normal = glm::ivec3(0, 0, (local.z < 0.5f) ? -1 : 1);
                    }

                    hit.normal = glm::vec3(normal);
                    return hit;
                }
            }
            previousVoxel = voxel;
        }
        return hit;
    }
}

Application::Application(const char* title, int width, int height)
    : m_title(title), m_width(width), m_height(height),
      m_camera(glm::vec3(0.0f, 20.0f, 20.0f)) {
}

Application::~Application() {
    cleanup();
}

void Application::run() {
    init();
    mainLoop();
}

void Application::init() {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW initialization failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, m_title, NULL, NULL);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("GLFWwindow initialization failed");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSwapInterval(1);

    glfwSetKeyCallback(m_window, onKey);
    glfwSetFramebufferSizeCallback(m_window, onFramebufferSize);
    glfwSetCursorPosCallback(m_window, onMouseMove);
    glfwSetMouseButtonCallback(m_window, onMouseButton);
    glfwSetScrollCallback(m_window, onMouseScroll);

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(m_window, m_width / 2.0, m_height / 2.0);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("GLEW initialization failed");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f);

    m_renderer = std::make_unique<Renderer>();
    m_renderer->init();

    m_debugDrawer = std::make_unique<DebugDrawer>();
    m_debugDrawer->init();

    initResources();
}

void Application::initResources() {
    m_world.generate(4);

    for (const auto& modelData : m_scene.models) {
        if (m_meshCache.find(modelData.meshFile) == m_meshCache.end()) {
            auto mesh = std::make_unique<Mesh>();
            if (mesh->loadObj(modelData.meshFile)) {
                m_meshCache[modelData.meshFile] = std::move(mesh);
            }
        }
        if (!modelData.textureFile.empty() && m_modelTextureCache.find(modelData.textureFile) == m_modelTextureCache.end()) {
            auto texture = std::make_unique<Texture2D>();
            if (texture->loadTexture(modelData.textureFile, true)) {
                m_modelTextureCache[modelData.textureFile] = std::move(texture);
            }
        }
    }

    const auto& pathToIndex = Chunk::m_pathToTextureIndex;
    int numTexturesToBind = (int)pathToIndex.size();
    if (numTexturesToBind > MAX_BLOCK_TEXTURES) {
        throw std::runtime_error("Too many block textures!");
    }

    m_blockTextures = std::make_unique<Texture2D[]>(MAX_BLOCK_TEXTURES);
    for (const auto& pair : pathToIndex) {
        const std::string& path = pair.first;
        int index = pair.second;
        if (!m_blockTextures[index].loadTexture(path, true)) {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }
    }

    std::cout << "World generated successfully!" << std::endl;
}

void Application::mainLoop() {
    m_lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(m_window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - m_lastTime;

        glfwPollEvents();
        processInput(deltaTime);
        update(deltaTime);
        render();

        glfwSwapBuffers(m_window);
        m_lastTime = currentTime;
    }
}

void Application::cleanup() {
    m_meshCache.clear();
    m_modelTextureCache.clear();
    m_blockTextures.reset();
    m_renderer.reset();
    m_debugDrawer.reset();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::processInput(double deltaTime) {
    double mouseX, mouseY;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);
    m_camera.rotate(
        (float)(mouseX - m_width / 2.0) * MOUSE_SENSITIVITY,
        (float)(m_height / 2.0 - mouseY) * MOUSE_SENSITIVITY
    );
    glfwSetCursorPos(m_window, m_width / 2.0, m_height / 2.0);

    float physicsMoveSpeed = 5.0f;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        MOVE_SPEED = 80.0f;
        physicsMoveSpeed = 10.0f;
    } else {
        MOVE_SPEED = 20.0f;
        physicsMoveSpeed = 5.0f;
    }

    if (m_isFlying) {
        m_camera.mVelocity = glm::vec3(0.0f);
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) m_camera.move(MOVE_SPEED * (float)deltaTime * m_camera.getLook());
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) m_camera.move(MOVE_SPEED * (float)deltaTime * -m_camera.getLook());
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) m_camera.move(MOVE_SPEED * (float)deltaTime * -m_camera.getRight());
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) m_camera.move(MOVE_SPEED * (float)deltaTime * m_camera.getRight());
        if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) m_camera.move(MOVE_SPEED * (float)deltaTime * glm::vec3(0, 1, 0));
        if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) m_camera.move(MOVE_SPEED * (float)deltaTime * glm::vec3(0, -1, 0));
    } else {
        glm::vec3 moveDirection(0.0f);
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) moveDirection += m_camera.getLook();
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) moveDirection -= m_camera.getLook();
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) moveDirection -= m_camera.getRight();
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) moveDirection += m_camera.getRight();
        if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) m_camera.jump();
        m_camera.mVelocity += glm::vec3(moveDirection.x, 0, moveDirection.z) * physicsMoveSpeed;
    }
}

void Application::update(double deltaTime) {
    if (!m_isFlying) {
        m_camera.applyPhysics(m_world, deltaTime);
    }

    if (m_leftMouseButtonPressed || m_rightMouseButtonPressed) {
        RaycastHit hit = raycastWorld(m_world, m_camera.getPosition(), glm::normalize(m_camera.getLook()), 16.0f);
        if (hit.hit) {
            if (m_leftMouseButtonPressed) {
                m_world.setBlockAt(hit.blockPos, BlockType::AIR);
            } else if (m_rightMouseButtonPressed) {
                glm::vec3 placePos = hit.blockPos + hit.normal;
                if (glm::length(placePos - m_camera.getPosition()) < 1.2f) {
                    std::cout << "Too close to player, can't place block" << std::endl;
                } else {
                    m_world.setBlockAt(placePos, m_selectedBlock);
                }
            }
        }
    }
    m_leftMouseButtonPressed = false;
    m_rightMouseButtonPressed = false;

    showFPS();
}

void Application::render() {
    m_renderer->render(m_camera, m_world, m_scene, m_meshCache, m_modelTextureCache, m_blockTextures.get(), m_width, m_height);

    if (m_debug) {
        RaycastHit hit = raycastWorld(m_world, m_camera.getPosition(), glm::normalize(m_camera.getLook()), 16.0f);
        m_debugDrawer->drawRaycast(m_camera, hit, m_width, m_height);
    }

    m_renderer->drawCrosshair(m_width, m_height);
}

void Application::onKey(GLFWwindow* window, int key, int scancode, int action, int mode) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
        case GLFW_KEY_F1:
            app->m_wireframe = !app->m_wireframe;
            glPolygonMode(GL_FRONT_AND_BACK, app->m_wireframe ? GL_LINE : GL_FILL);
            break;
        case GLFW_KEY_F2: app->m_debug = !app->m_debug; break;
        case GLFW_KEY_2: app->m_selectedBlock = BlockType::REDSTONE; break;
        case GLFW_KEY_3: app->m_selectedBlock = BlockType::DIRT; break;
        case GLFW_KEY_4: app->m_selectedBlock = BlockType::STONE; break;
        case GLFW_KEY_5: app->m_selectedBlock = BlockType::WOOD; break;
        case GLFW_KEY_6: app->m_selectedBlock = BlockType::LEAVES; break;
        case GLFW_KEY_7: app->m_selectedBlock = BlockType::TORCH; break;
        case GLFW_KEY_8: app->m_selectedBlock = BlockType::GLASS; break;
        case GLFW_KEY_F:
            app->m_isFlying = !app->m_isFlying;
            if (app->m_isFlying) app->m_camera.mVelocity.y = 0;
            break;
    }
}

void Application::onMouseButton(GLFWwindow* window, int button, int action, int mods) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) app->m_leftMouseButtonPressed = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) app->m_rightMouseButtonPressed = true;
    }
}

void Application::onMouseMove(GLFWwindow* window, double xpos, double ypos) {
    // Handled in processInput
}

void Application::onMouseScroll(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    double fov = app->m_camera.getFOV() + yoffset * ZOOM_SENSITIVITY;
    fov = glm::clamp(fov, 1.0, 120.0);
    app->m_camera.setFOV((float)fov);
}

void Application::onFramebufferSize(GLFWwindow* window, int width, int height) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->m_width = width;
    app->m_height = height;
    glViewport(0, 0, width, height);
}

void Application::showFPS() {
    static double previousSeconds = 0.0;
    static int frameCount = 0;
    double currentSeconds = glfwGetTime();
    double elapsedSeconds = currentSeconds - previousSeconds;

    if (elapsedSeconds > 0.25) {
        previousSeconds = currentSeconds;
        double fps = (double)frameCount / elapsedSeconds;
        double msPerFrame = 1000.0 / fps;

        std::ostringstream outs;
        outs.precision(3);
        outs << std::fixed << m_title << "    "
             << "FPS: " << fps << "    "
             << "Frame Time: " << msPerFrame << " (ms)";

        glfwSetWindowTitle(m_window, outs.str().c_str());
        frameCount = 0;
    }
    frameCount++;
}