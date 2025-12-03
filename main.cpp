#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <sstream>
#include <filesystem>
#include <cmath>
#include <limits>
#include <glm/gtx/norm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "./src/ShaderProgram.h"
#include "./src/Texture2D.h"
#include "./src/Camera.h"
#include "./src/Cube.h"

#define MAX_BLOCK_TEXTURES 16
#define MAX_POINT_LIGHTS 256
Texture2D gBlockTextures[MAX_BLOCK_TEXTURES];

const char* APP_TITLE = "Minecraft Clone - OpenGL Demo";
int gWindowWidth = 1280;
int gWindowHeight = 720;
GLFWwindow* gWindow = NULL;
bool gWireframe = false;

FPSCamera fpsCamera(glm::vec3(0.0f, 20.0f, 20.0f));
bool gIsFlying = false;
const double ZOOM_SENSITIVITY = -3.0;
float MOVE_SPEED = 30.0;
const float MOUSE_SENSITIVITY = 0.1f;

bool gLeftMouseButtonPressed = false;
bool gRightMouseButtonPressed = false;

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
BlockType gSelectedBlock = BlockType::TORCH;

GLuint crosshairVAO = 0;
GLuint crosshairVBO = 0;
ShaderProgram* crosshairShader = nullptr;

bool blueDebug = true;
GLuint debugLineVAO = 0;
GLuint debugLineVBO = 0;
ShaderProgram* debugLineShader = nullptr;
GLuint debugPointVAO = 0;
GLuint debugPointVBO = 0;

int main() {
        std::cout << "CWD: " << std::filesystem::current_path() << std::endl;

        if(!initOpenGL()) {
                std::cerr << "OpenGL initialization failed" << std::endl;
                return -1;
        }
        initCrosshair();

        if (blueDebug) {
                debugLineShader = new ShaderProgram();
                debugLineShader->loadShaders("./debug_line.vert", "./debug_line.frag");
                glGenVertexArrays(1, &debugLineVAO);
                glGenBuffers(1, &debugLineVBO);
                glBindVertexArray(debugLineVAO);
                glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, nullptr, GL_DYNAMIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                glBindVertexArray(0);
                glGenVertexArrays(1, &debugPointVAO);
                glGenBuffers(1, &debugPointVBO);
                glBindVertexArray(debugPointVAO);
                glBindBuffer(GL_ARRAY_BUFFER, debugPointVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3, nullptr, GL_DYNAMIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                glBindVertexArray(0);
        }

        ShaderProgram minecraftShader;
        minecraftShader.loadShaders("./minecraft.vert", "./minecraft.frag");

        world.generate(4);

        const auto& pathToIndex = Chunk::m_pathToTextureIndex;
        int numTexturesToBind = (int)pathToIndex.size();

        GLint textureUnits[MAX_BLOCK_TEXTURES];

        if (numTexturesToBind > MAX_BLOCK_TEXTURES) {
                std::cerr << "ERREUR: Trop de textures! Max est " << MAX_BLOCK_TEXTURES << std::endl;
                return -1;
        }

        for (const auto& pair : pathToIndex) {
                const std::string& path = pair.first;
                int index = pair.second;

                if (gBlockTextures[index].loadTexture(path, true)) {
                        textureUnits[index] = index;
                } else {
                        std::cerr << "Échec du chargement de la texture: " << path << std::endl;
                }
        }

        std::cout << "World generated successfully!" << std::endl;

        std::vector<glm::vec3> redstoneLights = world.getRedstoneLightPositions();

        std::vector<glm::vec3> frameLights;
        for (const auto& pos : redstoneLights) {
                if (frameLights.size() >= MAX_POINT_LIGHTS) break;
                frameLights.push_back(pos);
        }

        double lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(gWindow)) {
                redstoneLights = world.getRedstoneLightPositions();
                std::vector<glm::vec3> torchLights = world.getTorchLightPositions();

                frameLights.clear();
                for (const auto& pos : redstoneLights) {
                        if (frameLights.size() >= MAX_POINT_LIGHTS) break;
                        frameLights.push_back(pos);
                }
                int redstoneLightCount = frameLights.size();
                for (const auto& pos : torchLights) {
                        if (frameLights.size() >= MAX_POINT_LIGHTS) break;
                        frameLights.push_back(pos);
                }

                std::cout << "Redstone Lights: " << redstoneLightCount << ", Total Point Lights: " << frameLights.size() << std::endl;

                showFPS(gWindow);

                double currentTime = glfwGetTime();
                double deltaTime = currentTime - lastTime;

                glfwPollEvents();
                update(deltaTime);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glm::mat4 model = glm::mat4(1.0f);
                glm::mat4 view = fpsCamera.getViewMatrix();

                float fov = fpsCamera.getFOV();
                float aspectRatio = (float)gWindowWidth / (float)gWindowHeight;
                glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 200.0f);

                glm::vec3 viewPos = fpsCamera.getPosition();

                minecraftShader.use();

                minecraftShader.setUniform("model", model);
                minecraftShader.setUniform("view", view);
                minecraftShader.setUniform("projection", projection);
                minecraftShader.setUniform("viewPos", viewPos);

                // Directional light (sun)
                glm::vec3 sunDirection(-0.3f, -0.8f, -0.5f);
                minecraftShader.setUniform("dirLight.direction", sunDirection);
                minecraftShader.setUniform("dirLight.ambient", glm::vec3(0.3f, 0.3f, 0.35f)*0.2f);
                minecraftShader.setUniform("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.7f)*0.5f);
                minecraftShader.setUniform("dirLight.specular", glm::vec3(0.3f, 0.3f, 0.3f)*0.5f);

                // Point lights (redstone)
                minecraftShader.setUniform("numPointLights", (int)frameLights.size());
                for (size_t i = 0; i < frameLights.size(); i++) {
                        std::string base = "pointLights[" + std::to_string(i) + "]";
                        minecraftShader.setUniform((base + ".position").c_str(), frameLights[i]);
                        minecraftShader.setUniform((base + ".constant").c_str(), 1.0f);

                        if (i < redstoneLightCount) { // Redstone Light
                                minecraftShader.setUniform((base + ".ambient").c_str(), glm::vec3(0.01f, 0.0f, 0.0f));
                                minecraftShader.setUniform((base + ".diffuse").c_str(), glm::vec3(1.0f, 0.1f, 0.1f));
                                minecraftShader.setUniform((base + ".specular").c_str(), glm::vec3(0.5f, 0.1f, 0.1f));
                                minecraftShader.setUniform((base + ".linear").c_str(), 0.14f);
                                minecraftShader.setUniform((base + ".exponant").c_str(), 0.07f);
                        } else { // Torch Light
                                minecraftShader.setUniform((base + ".ambient").c_str(), glm::vec3(0.1f, 0.08f, 0.02f));
                                minecraftShader.setUniform((base + ".diffuse").c_str(), glm::vec3(1.0f, 0.8f, 0.2f));
                                minecraftShader.setUniform((base + ".specular").c_str(), glm::vec3(0.5f, 0.4f, 0.1f));
                                // Torches have a smaller light radius
                                minecraftShader.setUniform((base + ".linear").c_str(), 0.22f);
                                minecraftShader.setUniform((base + ".exponant").c_str(), 0.20f);
                        }
                }

                minecraftShader.setUniform("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));
                minecraftShader.setUniformSampler("material.diffuseMap", 0);
                minecraftShader.setUniform("material.specular", glm::vec3(0.1f, 0.1f, 0.1f));
                minecraftShader.setUniform("material.shininess", 8.0f);

                for (int i = 0; i < numTexturesToBind; i++) {
                        gBlockTextures[i].bind(i);
                        std::string samplerName = "material.diffuseMaps[" + std::to_string(i) + "]";
                        minecraftShader.setUniformSampler(samplerName.c_str(), i);
                }
                world.draw();
                for (int i = 0; i < numTexturesToBind; i++) {
                        gBlockTextures[i].unbind(i);
                }

                if (blueDebug) {
                        glm::vec3 origin = fpsCamera.getPosition();
                        glm::vec3 dir = glm::normalize(fpsCamera.getLook());

                        RaycastHit frameHit = raycastWorld(world, origin, dir, 16.0f);

                        glm::vec3 end;
                        if (frameHit.hit) {
                                end = frameHit.hitPos;
                        } else {
                                float len = 8.0f;
                                end = origin + dir * len;
                        }

                        float lineVerts[6] = { origin.x, origin.y, origin.z, end.x, end.y, end.z };

                        glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVerts), lineVerts);

                        debugLineShader->use();
                        debugLineShader->setUniform("model", glm::mat4(1.0f));
                        debugLineShader->setUniform("view", fpsCamera.getViewMatrix());
                        float fov = fpsCamera.getFOV();
                        float aspectRatio = (float)gWindowWidth / (float)gWindowHeight;
                        glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 200.0f);
                        debugLineShader->setUniform("projection", projection);

                        // Draw on top
                        glDisable(GL_DEPTH_TEST);

                        // Line (red)
                        debugLineShader->setUniform("uColor", glm::vec3(1.0f, 0.0f, 0.0f));
                        glBindVertexArray(debugLineVAO);
                        glLineWidth(4.0f);
                        glDrawArrays(GL_LINES, 0, 2);

                        if (frameHit.hit) {
                                float hitPt[3] = { frameHit.hitPos.x, frameHit.hitPos.y, frameHit.hitPos.z };
                                glBindBuffer(GL_ARRAY_BUFFER, debugPointVBO);
                                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(hitPt), hitPt);
                                debugLineShader->setUniform("uColor", glm::vec3(0.0f, 1.0f, 0.0f));
                                glBindVertexArray(debugPointVAO);
                                glPointSize(12.0f);
                                glDrawArrays(GL_POINTS, 0, 1);

                                glm::vec3 voxelCenter = frameHit.blockPos + glm::vec3(0.5f);
                                float voxelPt[3] = { voxelCenter.x, voxelCenter.y, voxelCenter.z };
                                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(voxelPt), voxelPt);
                                debugLineShader->setUniform("uColor", glm::vec3(0.0f, 1.0f, 1.0f));
                                glPointSize(14.0f);
                                glDrawArrays(GL_POINTS, 0, 1);
                        }

                        glBindVertexArray(0);
                        glEnable(GL_DEPTH_TEST);
                }

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
                case GLFW_KEY_F1:
                        gWireframe = !gWireframe;
                        if (gWireframe) {
                                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        } else {
                                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        }
                        break;
                case GLFW_KEY_F2:
                        blueDebug = !blueDebug;
                        break;
                case GLFW_KEY_2:
                        gSelectedBlock = BlockType::REDSTONE;
                        break;
                case GLFW_KEY_3:
                        gSelectedBlock = BlockType::DIRT;
                        break;
                case GLFW_KEY_4:
                        gSelectedBlock = BlockType::STONE;
                        break;
                case GLFW_KEY_5:
                        gSelectedBlock = BlockType::WOOD;
                        break;
                case GLFW_KEY_6:
                        gSelectedBlock = BlockType::LEAVES;
                        break;
                case GLFW_KEY_7:
                        gSelectedBlock = BlockType::TORCH;
                        break;
                case GLFW_KEY_8:
                        gSelectedBlock = BlockType::GLASS;
                        break;
                case GLFW_KEY_F:         // Toggle fly mode
                        gIsFlying = !gIsFlying;
                        if (gIsFlying) {
                                fpsCamera.mVelocity.y = 0;
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
        // This is handled in the update loop
}

void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY) {
        double fov = fpsCamera.getFOV() + deltaY * ZOOM_SENSITIVITY;
        fov = glm::clamp(fov, 1.0, 120.0);
        fpsCamera.setFOV((float)fov);
}

void glfw_onMouseButton(GLFWwindow* window, int button, int action, int mods) {
        if (action == GLFW_PRESS) {
                if (button == GLFW_MOUSE_BUTTON_LEFT) gLeftMouseButtonPressed = true;
                if (button == GLFW_MOUSE_BUTTON_RIGHT) gRightMouseButtonPressed = true;
        }
}


void update(double elapsedTime) {
        double mouseX, mouseY;
        glfwGetCursorPos(gWindow, &mouseX, &mouseY);

        fpsCamera.rotate(
                (float)(mouseX - gWindowWidth/2.0) * MOUSE_SENSITIVITY,
                (float)(gWindowHeight/2.0 - mouseY) * MOUSE_SENSITIVITY
        );

        glfwSetCursorPos(gWindow, gWindowWidth/2.0, gWindowHeight/2.0);

        float physicsMoveSpeed = 1.0f; // Vitesse d'accélération au sol
        if (glfwGetKey(gWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                MOVE_SPEED = 80.0f;
                physicsMoveSpeed = 2.5f;
        } else {
                MOVE_SPEED = 20.0f;
                physicsMoveSpeed = 1.0f;
        }

        if (gIsFlying) {
                fpsCamera.mVelocity = glm::vec3(0.0f);

                if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS)
                        fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getLook());
                if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS)
                        fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getLook());
                if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS)
                        fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getRight());
                if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS)
                        fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getRight());
                if (glfwGetKey(gWindow, GLFW_KEY_SPACE) == GLFW_PRESS)
                        fpsCamera.move(MOVE_SPEED * (float)elapsedTime * glm::vec3(0, 1, 0));
                if (glfwGetKey(gWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                        fpsCamera.move(MOVE_SPEED * (float)elapsedTime * glm::vec3(0, -1, 0));

        } else {
                glm::vec3 moveDirection(0.0f);

                if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS) moveDirection += fpsCamera.getLook();
                if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS) moveDirection -= fpsCamera.getLook();
                if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS) moveDirection -= fpsCamera.getRight();
                if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS) moveDirection += fpsCamera.getRight();
                if (glfwGetKey(gWindow, GLFW_KEY_SPACE) == GLFW_PRESS) fpsCamera.jump();

                fpsCamera.mVelocity += glm::vec3(moveDirection.x, 0, moveDirection.z) * physicsMoveSpeed;
                fpsCamera.applyPhysics(world, elapsedTime);
        }

        if (gLeftMouseButtonPressed || gRightMouseButtonPressed) {
                glm::vec3 origin = fpsCamera.getPosition();
                glm::vec3 dir = glm::normalize(fpsCamera.getLook());
                RaycastHit hit = raycastWorld(world, origin, dir, 16.0f);

                if (hit.hit) {
                        if (gLeftMouseButtonPressed) {
                                world.setBlockAt(hit.blockPos, BlockType::AIR);
                        } else if (gRightMouseButtonPressed) {
                                glm::vec3 placePos = hit.blockPos + hit.normal;

                                glm::vec3 playerPos = fpsCamera.getPosition();
                                if (glm::length(placePos - playerPos) < 1.2f) {
                                        std::cout << "Too close to player, can't place block" << std::endl;
                                } else {
                                        if (gSelectedBlock == BlockType::TORCH) {
                                                glm::vec3 below = placePos + glm::vec3(0, -1, 0);
                                                BlockType support = world.getBlockAt(below);
                                                if (support == BlockType::AIR) {
                                                        std::cout << "Torch needs solid block below" << std::endl;
                                                } else {
                                                        world.setBlockAt(placePos, gSelectedBlock);
                                                }
                                        } else {
                                                bool placed = world.setBlockAt(placePos, gSelectedBlock);
                                        }
                                }
                        }
                } else {

                }
        }

        gLeftMouseButtonPressed = false;
        gRightMouseButtonPressed = false;
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
        RaycastHit hit;
        hit.hit = false;

        if (stepSize <= 0.0f) stepSize = 0.05f;

        float t = 0.0f;
        glm::ivec3 previousVoxel = glm::floor(origin + glm::vec3(0.5f));

        // march along the ray
        while (t <= maxDist) {
                t += stepSize;
                glm::vec3 pos = origin + dir * t;
                glm::ivec3 voxel = glm::floor(pos + glm::vec3(0.5f));

                // If voxel changed since last step, check for block
                if (voxel.x != previousVoxel.x || voxel.y != previousVoxel.y || voxel.z != previousVoxel.z) {
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

void drawCrosshair() {
        if (crosshairShader == nullptr) return;

        float centerX = gWindowWidth / 2.0f;
        float centerY = gWindowHeight / 2.0f;
        float size = 10.0f;
        float gap = 3.0f;

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

void initCrosshair() {
        crosshairShader = new ShaderProgram();
        crosshairShader->loadShaders("./crosshair.vert", "./crosshair.frag");

        glGenVertexArrays(1, &crosshairVAO);
        glGenBuffers(1, &crosshairVBO);

        glBindVertexArray(crosshairVAO);
        glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 24, nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
}

void cleanupCrosshair() {
        if (crosshairVAO) glDeleteVertexArrays(1, &crosshairVAO);
        if (crosshairVBO) glDeleteBuffers(1, &crosshairVBO);
        if (crosshairShader) delete crosshairShader;
        if (!blueDebug) return;
        if (debugLineVAO) glDeleteVertexArrays(1, &debugLineVAO);
        if (debugLineVBO) glDeleteBuffers(1, &debugLineVBO);
        if (debugLineShader) delete debugLineShader;
        if (debugPointVAO) glDeleteVertexArrays(1, &debugPointVAO);
        if (debugPointVBO) glDeleteBuffers(1, &debugPointVBO);
}