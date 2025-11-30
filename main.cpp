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

FPSCamera fpsCamera(glm::vec3(0.0f, 20.0f, 20.0f));
const double ZOOM_SENSITIVITY = -3.0;
float MOVE_SPEED = 8.0;
const float MOUSE_SENSITIVITY = 0.1f;

void glfw_onkey(GLFWwindow* window, int key, int scancode, int action, int mode);
void glfw_onFrameBufferSize(GLFWwindow* window, int width, int height);
void glfw_onMouseMove(GLFWwindow* window, double posX, double posY);
void glfw_onMouseButton(GLFWwindow* window, int button, int action, int mods);
void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY);
void update(double elapsedTime);
void showFPS(GLFWwindow* window);
bool initOpenGL();
RaycastHit raycastWorld(const World& world, const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step = 0.05f);
void drawCrosshair();
void initCrosshair();
void cleanupCrosshair();

World world;
BlockType gSelectedBlock = BlockType::STONE;

GLuint crosshairVAO = 0;
GLuint crosshairVBO = 0;
ShaderProgram* crosshairShader = nullptr;

int main() {
        std::cout << "CWD: " << std::filesystem::current_path() << std::endl;

        if(!initOpenGL()) {
                std::cerr << "OpenGL initialization failed" << std::endl;
                return -1;
        }
        initCrosshair();

        // Load shaders
        ShaderProgram minecraftShader;
        minecraftShader.loadShaders("./minecraft.vert", "./minecraft.frag");

        // Load block texture atlas
        Texture2D blockTexture;
        blockTexture.loadTexture("./textures/blocks.png", true);

        // Generate world
        world.generate(1); // 4 chunk render distance

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
                minecraftShader.setUniform("dirLight.ambient", glm::vec3(0.3f, 0.3f, 0.35f)*0.5f);
                minecraftShader.setUniform("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.7f));
                minecraftShader.setUniform("dirLight.specular", glm::vec3(0.3f, 0.3f, 0.3f));

                // Point lights (redstone) - limit to closest 50
                int numLights = std::min((int)redstoneLights.size(), 50);
                minecraftShader.setUniform("numPointLights", numLights);

                for (int i = 0; i < numLights; i++) {
                        std::string base = "pointLights[" + std::to_string(i) + "]";

                        minecraftShader.setUniform((base + ".position").c_str(), redstoneLights[i]);
                        minecraftShader.setUniform((base + ".ambient").c_str(), glm::vec3(1.0f, 0.0f, 0.0f));
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

                drawCrosshair();

                glfwSwapBuffers(gWindow);
                lastTime = currentTime;
        }
        cleanupCrosshair();
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
        glfwSetMouseButtonCallback(gWindow, glfw_onMouseButton);
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
                case GLFW_KEY_2:         // select stone
                        gSelectedBlock = BlockType::STONE;
                        break;
                case GLFW_KEY_3:         // select dirt
                        gSelectedBlock = BlockType::DIRT;
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

void glfw_onMouseButton(GLFWwindow* window, int button, int action, int mods) {
        if (action != GLFW_PRESS) return;

        glm::vec3 origin = fpsCamera.getPosition();
        glm::vec3 dir    = glm::normalize(fpsCamera.getLook());

        RaycastHit hit = raycastWorld(world, origin, dir, 16.0f);

        if (!hit.hit) return;

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
                // Break block
                world.setBlockAt(hit.blockPos, BlockType::AIR);
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                // Place block on the face we hit: adjacent cell along normal
                glm::vec3 placePos = hit.blockPos + hit.normal;

                // Simple support check for torches: require solid below if placing torch
                if (gSelectedBlock == BlockType::TORCH) {
                        glm::vec3 below = placePos + glm::vec3(0,-1,0);
                        BlockType support = world.getBlockAt(below);
                        if (!world.setBlockAt(placePos, BlockType::TORCH))return;
                } else {
                        world.setBlockAt(placePos, gSelectedBlock);
                }
        }
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
        // if control is pressed, increase speed
        if (glfwGetKey(gWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                MOVE_SPEED = 20.0f;
        } else {
                MOVE_SPEED = 8.0f;
        }

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

RaycastHit raycastWorld(const World& world, const glm::vec3& origin, const glm::vec3& dir, float maxDist, float stepSize) {
        RaycastHit res{};
        res.hit = false;

        // Current voxel position
        glm::ivec3 voxel = glm::ivec3(glm::floor(origin));

        // Direction to step in each axis (-1, 0, or 1)
        glm::ivec3 stepDir = glm::ivec3(
                dir.x > 0 ? 1 : (dir.x < 0 ? -1 : 0),
                dir.y > 0 ? 1 : (dir.y < 0 ? -1 : 0),
                dir.z > 0 ? 1 : (dir.z < 0 ? -1 : 0)
        );

        // Distance to next voxel boundary along each axis
        glm::vec3 tDelta = glm::vec3(
                stepDir.x != 0 ? 1.0f / std::abs(dir.x) : FLT_MAX,
                stepDir.y != 0 ? 1.0f / std::abs(dir.y) : FLT_MAX,
                stepDir.z != 0 ? 1.0f / std::abs(dir.z) : FLT_MAX
        );

        // Distance along ray to next voxel boundary
        glm::vec3 tMax;

        // Calculate initial tMax for each axis
        if (stepDir.x > 0)
                tMax.x = (std::floor(origin.x) + 1.0f - origin.x) / std::abs(dir.x);
        else if (stepDir.x < 0)
                tMax.x = (origin.x - std::floor(origin.x)) / std::abs(dir.x);
        else
                tMax.x = FLT_MAX;

        if (stepDir.y > 0) {
                tMax.y = (std::floor(origin.y) + 1.0f - origin.y) / std::abs(dir.y);
        } else if (stepDir.y < 0) {
                tMax.y = (origin.y - std::floor(origin.y)) / -std::abs(dir.y);
        } else {
                tMax.y = FLT_MAX;
        }

        if (stepDir.z > 0) {
                tMax.z = (std::floor(origin.z) + 1.0f - origin.z) / std::abs(dir.z);
        } else if (stepDir.z < 0) {
                tMax.z = (origin.z - std::floor(origin.z)) / -std::abs(dir.z);
        } else {
                tMax.z = FLT_MAX;
        }

        float t = 0.0f;
        glm::ivec3 normal = glm::ivec3(0, 0, 0);

        // DDA traversal
        while (t < maxDist) {
                // Check current voxel (integer world coords)
                BlockType bt = world.getBlock(voxel.x, voxel.y, voxel.z);

                if (bt != BlockType::AIR) {
                        res.hit = true;
                        res.blockPos = glm::vec3(voxel);
                        res.hitPos   = origin + dir * t;
                        res.normal   = glm::vec3(normal);
                        return res;
                }

                // Step to next voxel boundary
                if (tMax.x < tMax.y) {
                        if (tMax.x < tMax.z) {
                                // X is closest
                                t = tMax.x;
                                tMax.x += tDelta.x;
                                voxel.x += stepDir.x;
                                normal = glm::ivec3(-stepDir.x, 0, 0);
                        } else {
                                // Z is closest
                                t = tMax.z;
                                tMax.z += tDelta.z;
                                voxel.z += stepDir.z;
                                normal = glm::ivec3(0, 0, -stepDir.z);
                        }
                } else {
                        if (tMax.y < tMax.z) {
                                // Y is closest
                                t = tMax.y;
                                tMax.y += tDelta.y;
                                voxel.y += stepDir.y;
                                normal = glm::ivec3(0, -stepDir.y, 0);
                        } else {
                                // Z is closest
                                t = tMax.z;
                                tMax.z += tDelta.z;
                                voxel.z += stepDir.z;
                                normal = glm::ivec3(0, 0, -stepDir.z);
                        }
                }
        }

        std::cout << "Raycast: no hit within max distance" << std::endl;
        return res;
}

void drawCrosshair() {
        if (crosshairShader == nullptr) return;

        float centerX = gWindowWidth / 2.0f;
        float centerY = gWindowHeight / 2.0f;
        float size = 10.0f; // crosshair arm length in pixels
        float gap = 3.0f;   // gap from center

        GLfloat verts[] = {
                // Horizontal line (left)
                centerX - size - gap, centerY, 0.0f,
                centerX - gap, centerY, 0.0f,
                // Horizontal line (right)
                centerX + gap, centerY, 0.0f,
                centerX + size + gap, centerY, 0.0f,
                // Vertical line (top)
                centerX, centerY + gap, 0.0f,
                centerX, centerY + size + gap, 0.0f,
                // Vertical line (bottom)
                centerX, centerY - gap, 0.0f,
                centerX, centerY - size - gap, 0.0f
        };

        glBindVertexArray(crosshairVAO);
        glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

        // Use orthographic projection for 2D overlay
        glm::mat4 ortho = glm::ortho(0.0f, (float)gWindowWidth, 0.0f, (float)gWindowHeight);

        crosshairShader->use();
        crosshairShader->setUniform("projection", ortho);

        glDisable(GL_DEPTH_TEST);
        glLineWidth(2.0f);
        glBindVertexArray(crosshairVAO);
        glDrawArrays(GL_LINES, 0, 8);
        glEnable(GL_DEPTH_TEST);
}

// Add this initialization function
void initCrosshair() {
        // Create shader
        crosshairShader = new ShaderProgram();
        crosshairShader->loadShaders("./crosshair.vert", "./crosshair.frag");

        // Setup VAO/VBO
        glGenVertexArrays(1, &crosshairVAO);
        glGenBuffers(1, &crosshairVBO);

        glBindVertexArray(crosshairVAO);
        glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);

        // Allocate buffer (we'll update it each frame)
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 24, nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
}

// Add cleanup function
void cleanupCrosshair() {
        if (crosshairVAO) glDeleteVertexArrays(1, &crosshairVAO);
        if (crosshairVBO) glDeleteBuffers(1, &crosshairVBO);
        if (crosshairShader) delete crosshairShader;
}