#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <sstream>
#include <filesystem>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "./src/ShaderProgram.h"
#include "./src/Texture2D.h"
#include "./src/Camera.h"
#include "./src/Cube.h"

const char* APP_TITLE = "Minecraft Clone - OpenGL Demo";
int gWindowWidth = 1280;
int gWindowHeight = 720;
GLFWwindow* gWindow = NULL;
bool gWireframe = false;

FPSCamera fpsCamera(glm::vec3(0.0f, 10.0f, 20.0f));
const double ZOOM_SENSITIVITY = -3.0;
const float MOVE_SPEED = 8.0;
const float MOUSE_SENSITIVITY = 0.1f;

void glfw_onkey(GLFWwindow* window, int key, int scancode, int action, int mode);
void glfw_onFrameBufferSize(GLFWwindow* window, int width, int height);
void glfw_onMouseMove(GLFWwindow* window, double posX, double posY);
void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY);
void update(double elapsedTime);
void showFPS(GLFWwindow* window);
bool initOpenGL();

int main() {
        std::cout << "CWD: " << std::filesystem::current_path() << std::endl;

        if(!initOpenGL()) {
                std::cerr << "OpenGL initialization failed" << std::endl;
                return -1;
        }

        // Load shaders
        ShaderProgram minecraftShader;
        minecraftShader.loadShaders("./minecraft.vert", "./minecraft.frag");

        // Load block texture atlas
        Texture2D blockTexture;
        blockTexture.loadTexture("./textures/blocks.png", true);

        // Generate world
        World world;
        world.generate(4); // 4 chunk render distance

        std::cout << "World generated successfully!" << std::endl;

        // Get redstone light positions
        std::vector<glm::vec3> redstoneLights = world.getRedstoneLightPositions();
        std::cout << "Found " << redstoneLights.size() << " redstone blocks" << std::endl;

        double lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(gWindow)) {
                showFPS(gWindow);

                double currentTime = glfwGetTime();
                double deltaTime = currentTime - lastTime;

                glfwPollEvents();
                update(deltaTime);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Setup matrices
                glm::mat4 model = glm::mat4(1.0f);
                glm::mat4 view = fpsCamera.getViewMatrix();

                float fov = fpsCamera.getFOV();
                float aspectRatio = (float)gWindowWidth / (float)gWindowHeight;
                glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 200.0f);

                glm::vec3 viewPos = fpsCamera.getPosition();

                // Use shader
                minecraftShader.use();
                minecraftShader.setUniform("model", model);
                minecraftShader.setUniform("view", view);
                minecraftShader.setUniform("projection", projection);
                minecraftShader.setUniform("viewPos", viewPos);

                // Directional light (sun)
                glm::vec3 sunDirection(-0.3f, -0.8f, -0.5f);
                minecraftShader.setUniform("dirLight.direction", sunDirection);
                minecraftShader.setUniform("dirLight.ambient", glm::vec3(0.3f, 0.3f, 0.35f));
                minecraftShader.setUniform("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.7f));
                minecraftShader.setUniform("dirLight.specular", glm::vec3(0.3f, 0.3f, 0.3f));

                // Point lights (redstone) - limit to closest 50
                int numLights = std::min((int)redstoneLights.size(), 50);
                minecraftShader.setUniform("numPointLights", numLights);

                for (int i = 0; i < numLights; i++) {
                        std::string base = "pointLights[" + std::to_string(i) + "]";

                        minecraftShader.setUniform((base + ".position").c_str(), redstoneLights[i]);
                        minecraftShader.setUniform((base + ".ambient").c_str(), glm::vec3(0.1f, 0.0f, 0.0f));
                        minecraftShader.setUniform((base + ".diffuse").c_str(), glm::vec3(1.0f, 0.1f, 0.1f));
                        minecraftShader.setUniform((base + ".specular").c_str(), glm::vec3(1.0f, 0.2f, 0.2f));
                        minecraftShader.setUniform((base + ".constant").c_str(), 1.0f);
                        minecraftShader.setUniform((base + ".linear").c_str(), 0.14f);
                        minecraftShader.setUniform((base + ".exponant").c_str(), 0.07f);
                }

                // Material properties
                minecraftShader.setUniform("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));
                minecraftShader.setUniformSampler("material.diffuseMap", 0);
                minecraftShader.setUniform("material.specular", glm::vec3(0.1f, 0.1f, 0.1f));
                minecraftShader.setUniform("material.shininess", 8.0f);

                // Draw world
                blockTexture.bind(0);
                world.draw();
                blockTexture.unbind(0);

                glfwSwapBuffers(gWindow);
                lastTime = currentTime;
        }

        glfwTerminate();
        return 0;
}

bool initOpenGL() {
        if (!glfwInit()) {
                std::cerr << "GLFW initialization failed" << std::endl;
                return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        gWindow = glfwCreateWindow(gWindowWidth, gWindowHeight, APP_TITLE, NULL, NULL);

        if (gWindow == NULL) {
                std::cerr << "GLFWwindow initialization failed" << std::endl;
                glfwTerminate();
                return false;
        }

        glfwMakeContextCurrent(gWindow);
        glfwSwapInterval(1);

        glfwSetKeyCallback(gWindow, glfw_onkey);
        glfwSetFramebufferSizeCallback(gWindow, glfw_onFrameBufferSize);
        glfwSetCursorPosCallback(gWindow, glfw_onMouseMove);
        glfwSetScrollCallback(gWindow, glfw_onMouseScroll);

        glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPos(gWindow, gWindowWidth / 2.0, gWindowHeight / 2.0);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
                std::cerr << "GLEW initialization failed" << std::endl;
                glfwTerminate();
                return false;
        }

        glClearColor(0.53f, 0.81f, 0.98f, 1.0f); // Sky blue
        glViewport(0, 0, gWindowWidth, gWindowHeight);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        return true;
}

void glfw_onkey(GLFWwindow* window, int key, int scancode, int action, int mode) {
        if (action != GLFW_PRESS) {
                return;
        }

        switch (key){
                case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
                case GLFW_KEY_1:
                gWireframe = !gWireframe;
                if (gWireframe) {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                } else {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
                break;
                default:
                break;
        }
}

void glfw_onFrameBufferSize(GLFWwindow* window, int width, int height) {
        gWindowWidth = width;
        gWindowHeight = height;
        glViewport(0, 0, gWindowWidth, gWindowHeight);
}

void glfw_onMouseMove(GLFWwindow* window, double posX, double posY) {
        // Mouse movement handled in update()
}

void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY) {
        double fov = fpsCamera.getFOV() + deltaY * ZOOM_SENSITIVITY;
        fov = glm::clamp(fov, 1.0, 120.0);
        fpsCamera.setFOV((float)fov);
}

void update(double elapsedTime) {
        double mouseX, mouseY;
        glfwGetCursorPos(gWindow, &mouseX, &mouseY);

        fpsCamera.rotate(
                (float)(gWindowWidth/2.0 - mouseX) * MOUSE_SENSITIVITY,
                (float)(gWindowHeight/2.0 - mouseY) * MOUSE_SENSITIVITY
        );

        glfwSetCursorPos(gWindow, gWindowWidth/2.0, gWindowHeight/2.0);

        // Movement
        if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getLook());
        }
        if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getLook());
        }
        if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getRight());
        }
        if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getRight());
        }
        if (glfwGetKey(gWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * glm::vec3(0, 1, 0));
        }
        if (glfwGetKey(gWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * glm::vec3(0, -1, 0));
        }
}

void showFPS(GLFWwindow* window) {
        static double previousSeconds = 0.0;
        static int frameCount = 0;
        double elapsedSeconds;
        double currentSeconds = glfwGetTime();

        elapsedSeconds = currentSeconds - previousSeconds;

        if (elapsedSeconds > 0.25) {
                previousSeconds = currentSeconds;
                double fps = (double)frameCount / elapsedSeconds;
                double msPerFrame = 1000.0 / fps;

                std::ostringstream outs;
                outs.precision(3);
                outs << std::fixed
                << APP_TITLE << "    "
                << "FPS: " << fps << "    "
                << "Frame Time: " << msPerFrame  << " (ms)";

                glfwSetWindowTitle(window, outs.str().c_str());
                frameCount = 0;
        }
        frameCount++;
}