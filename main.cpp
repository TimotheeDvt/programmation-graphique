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

#include "./src/Mesh.h"
#include "./src/Model.h"
#include "./src/Scene.h"

#define MAX_BLOCK_TEXTURES 16
#define MAX_POINT_LIGHTS 32
#define MAX_SPOT_LIGHTS 8
Texture2D gBlockTextures[MAX_BLOCK_TEXTURES];

Scene scene; // Déclaration de l'instance de la scène
std::map<std::string, Mesh*> meshCache; // Cache pour les données des maillages (OBJ)
std::map<std::string, Texture2D*> modelTextureCache; // Cache pour les textures des modèles

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

const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;
GLuint gShadowMapFBO;
GLuint gShadowMap;
ShaderProgram* shadowShader = nullptr;

bool initShadows() {
    // 1. Créer le FBO
    glGenFramebuffers(1, &gShadowMapFBO);

    // 2. Créer la texture de profondeur (Shadow Map)
    glGenTextures(1, &gShadowMap);
    glBindTexture(GL_TEXTURE_2D, gShadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    // Paramètres pour la gestion des bords et du filtrage
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Couleur blanche pour les zones hors champ
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 3. Attacher la texture de profondeur au FBO
    glBindFramebuffer(GL_FRAMEBUFFER, gShadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gShadowMap, 0);

    // 4. Configurer le FBO pour ne pas avoir de sortie couleur
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!" << std::endl;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

int main() {
        std::cout << "CWD: " << std::filesystem::current_path() << std::endl;

        if(!initOpenGL()) {
                std::cerr << "OpenGL initialization failed" << std::endl;
                return -1;
        }
        initCrosshair();

        if(!initShadows()) { // AJOUTÉ: Initialisation des ombres
                std::cerr << "Shadow initialization failed" << std::endl;
                return -1;
        }

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

        shadowShader = new ShaderProgram();
        shadowShader->loadShaders("./shadow_dir.vert", "./shadow_dir.frag"); // AJOUTÉ

        ShaderProgram minecraftShader;
        minecraftShader.loadShaders("./minecraft.vert", "./minecraft.frag");

        world.generate(4);

        // NOUVEAU: Préchargement des maillages et textures
        for (const auto& modelData : scene.models) {
                // 1. Charger le maillage (Enderman.obj, ukulele.obj)
                if (meshCache.find(modelData.meshFile) == meshCache.end()) {
                        Mesh* mesh = new Mesh();
                        if (mesh->loadObj(modelData.meshFile)) {
                                meshCache[modelData.meshFile] = mesh;
                        } else {
                                delete mesh;
                        }
                }
                // 2. Charger la texture (enderman.png, ukulele.jpg)
                if (!modelData.textureFile.empty() && modelTextureCache.find(modelData.textureFile) == modelTextureCache.end()) {
                        Texture2D* texture = new Texture2D();
                        if (texture->loadTexture(modelData.textureFile, true)) {
                                modelTextureCache[modelData.textureFile] = texture;
                        } else {
                                delete texture;
                        }
                }
        }

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

        std::vector<glm::vec3> frameLights;

        double lastTime = glfwGetTime();

        glm::mat4 lightSpaceMatrix;

        while (!glfwWindowShouldClose(gWindow)) {
                std::vector<glm::vec3> redstoneLights = world.getRedstoneLightPositions();
                std::vector<glm::vec3> torchLights = world.getTorchLightPositions();

                // === ÉTAPE 1: Préparer TOUTES les lumières AVANT le rendu ===
                frameLights.clear();
                std::vector<std::pair<glm::vec3, glm::vec3>> spotLights; // position, direction

                // Ajouter les lumières redstone
                for (const auto& pos : redstoneLights) {
                        if (frameLights.size() >= MAX_POINT_LIGHTS) break;
                                frameLights.push_back(pos);
                }
                int redstoneLightCount = frameLights.size();

                // Ajouter les lumières torches
                for (const auto& pos : torchLights) {
                        if (frameLights.size() >= MAX_POINT_LIGHTS) break;
                                frameLights.push_back(pos);
                }
                int worldLightCount = frameLights.size();

                // Ajouter les spotlights des modèles (Enderman)
                for (const auto& modelData : scene.models) {
                        if (spotLights.size() >= MAX_SPOT_LIGHTS) break;
                        // Direction du regard (vers l'avant du modèle)
                        glm::vec3 lookDirection = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
                        // Position des yeux
                        glm::vec3 eyeLightPos = modelData.position + glm::vec3(0.1f, 2.75f, 0.4f);
                        spotLights.push_back({eyeLightPos, lookDirection});
                        eyeLightPos = modelData.position + glm::vec3(-0.1f, 2.75f, 0.45f);
                        spotLights.push_back({eyeLightPos, lookDirection});
                }

                showFPS(gWindow);

                double currentTime = glfwGetTime();
                double deltaTime = currentTime - lastTime;

                glfwPollEvents();
                update(deltaTime);

                // --- PASS 1: Rendu de la carte de profondeur (Shadow Map) ---
                glm::vec3 sunDirection(-0.3f, -0.8f, -0.5f);
                float near_plane = 1.0f, far_plane = 70.0f;
                // Crée une matrice de projection orthographique pour la lumière
                glm::mat4 lightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, near_plane, far_plane);

                // Crée une matrice de vue de la lumière, centrée sur la caméra du joueur
                glm::vec3 centerPos = fpsCamera.getPosition();
                glm::mat4 lightView = glm::lookAt(centerPos - sunDirection * 20.0f, // Position de la caméra de lumière
                                                  centerPos,                         // Cible (centre du champ de vision)
                                                  glm::vec3(0.0f, 1.0f, 0.0f));       // Up vector

                lightSpaceMatrix = lightProjection * lightView;

                glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
                glBindFramebuffer(GL_FRAMEBUFFER, gShadowMapFBO);
                glClear(GL_DEPTH_BUFFER_BIT);
                glCullFace(GL_FRONT); // Culling de face avant pour éviter le shadow acne

                shadowShader->use();
                shadowShader->setUniform("lightSpaceMatrix", lightSpaceMatrix);

                glm::mat4 model = glm::mat4(1.0f);
                // Rendu du monde
                shadowShader->setUniform("model", model);
                world.draw();

                // Rendu des modèles dans le Shadow Map
                for (const auto& modelData : scene.models) {
                        Mesh* mesh = meshCache.count(modelData.meshFile) ? meshCache.at(modelData.meshFile) : nullptr;
                        if (mesh) {
                                glm::mat4 modelMatrix = glm::mat4(1.0f);
                                modelMatrix = glm::translate(modelMatrix, modelData.position);
                                modelMatrix = glm::rotate(modelMatrix, glm::radians(modelData.rotation.angle), modelData.rotation.axis);
                                modelMatrix = glm::scale(modelMatrix, modelData.scale);

                                shadowShader->setUniform("model", modelMatrix);
                                mesh->draw();
                        }
                }

                glCullFace(GL_BACK); // Retour au culling de face arrière par défaut
                glBindFramebuffer(GL_FRAMEBUFFER, 0);


                // --- PASS 2: Rendu de la scène principale avec ombres ---
                glViewport(0, 0, gWindowWidth, gWindowHeight);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glm::mat4 view = fpsCamera.getViewMatrix();

                float fov = fpsCamera.getFOV();
                float aspectRatio = (float)gWindowWidth / (float)gWindowHeight;
                glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 200.0f);

                glm::vec3 viewPos = fpsCamera.getPosition();

                minecraftShader.use();

                model = glm::mat4(1.0f); // Re-initialisation du model matrix pour le monde
                minecraftShader.setUniform("model", model);
                minecraftShader.setUniform("view", view);
                minecraftShader.setUniform("projection", projection);
                minecraftShader.setUniform("viewPos", viewPos);

                // Passer la matrice d'espace lumière et le Shadow Map (unité de texture 16)
                minecraftShader.setUniform("lightSpaceMatrix", lightSpaceMatrix);
                glActiveTexture(GL_TEXTURE0 + MAX_BLOCK_TEXTURES);
                glBindTexture(GL_TEXTURE_2D, gShadowMap);
                minecraftShader.setUniformSampler("shadowMap", MAX_BLOCK_TEXTURES);

                // Directional light (sun) - Ambient est géré dans le shader principal
                minecraftShader.setUniform("dirLight.direction", sunDirection);
                minecraftShader.setUniform("dirLight.ambient", glm::vec3(0.3f, 0.3f, 0.35f)*0.2f);
                minecraftShader.setUniform("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.7f)*0.5f);
                minecraftShader.setUniform("dirLight.specular", glm::vec3(0.3f, 0.3f, 0.3f)*0.5f);

                // === ÉTAPE 2: Configurer les point lights ===
                minecraftShader.setUniform("numPointLights", worldLightCount);

                for (int i = 0; i < worldLightCount; i++) {
                        std::string base = "pointLights[" + std::to_string(i) + "]";
                        minecraftShader.setUniform((base + ".position").c_str(), frameLights[i]);
                        minecraftShader.setUniform((base + ".constant").c_str(), 1.0f);

                        // Ambient doit être 0 dans le C++ pour correspondre au shader sans occlusion
                        minecraftShader.setUniform((base + ".ambient").c_str(), glm::vec3(0.0f));

                        if (i < redstoneLightCount) {
                                // Redstone Light
                                minecraftShader.setUniform((base + ".diffuse").c_str(), glm::vec3(1.0f, 0.1f, 0.1f));
                                minecraftShader.setUniform((base + ".specular").c_str(), glm::vec3(0.5f, 0.1f, 0.1f));
                                minecraftShader.setUniform((base + ".linear").c_str(), 0.14f);
                                minecraftShader.setUniform((base + ".exponant").c_str(), 0.07f);
                        }
                        else {
                                // Torch Light
                                minecraftShader.setUniform((base + ".diffuse").c_str(), glm::vec3(1.0f, 0.8f, 0.2f));
                                minecraftShader.setUniform((base + ".specular").c_str(), glm::vec3(0.5f, 0.4f, 0.1f));
                                minecraftShader.setUniform((base + ".linear").c_str(), 0.22f);
                                minecraftShader.setUniform((base + ".exponant").c_str(), 0.20f);
                        }
                }

                // === ÉTAPE 3: Configurer les spotlights (Enderman eyes) ===
                minecraftShader.setUniform("numSpotLights", (int)spotLights.size());

                for (int i = 0; i < spotLights.size(); i++) {
                        std::string base = "spotLights[" + std::to_string(i) + "]";
                        minecraftShader.setUniform((base + ".position").c_str(), spotLights[i].first);
                        minecraftShader.setUniform((base + ".direction").c_str(), spotLights[i].second);

                        // Spotlight properties
                        // Ambient doit être 0 dans le C++ pour correspondre au shader sans occlusion
                        minecraftShader.setUniform((base + ".ambient").c_str(), glm::vec3(0.0f)); 
                        minecraftShader.setUniform((base + ".diffuse").c_str(), glm::vec3(0.8f, 0.0f, 0.8f) * 8.0f);
                        minecraftShader.setUniform((base + ".specular").c_str(), glm::vec3(0.6f, 0.0f, 0.6f) * 8.0f);

                        // Attenuation
                        minecraftShader.setUniform((base + ".constant").c_str(), 1.0f);
                        minecraftShader.setUniform((base + ".linear").c_str(), 0.09f);
                        minecraftShader.setUniform((base + ".exponant").c_str(), 0.032f);

                        // Spotlight cone angles
                        minecraftShader.setUniform((base + ".cosInnerCone").c_str(), glm::cos(glm::radians(12.5f)));
                        minecraftShader.setUniform((base + ".cosOuterCone").c_str(), glm::cos(glm::radians(25.0f)));
                }

                // === ÉTAPE 4: Configurer les matériaux ===
                minecraftShader.setUniform("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));
                minecraftShader.setUniformSampler("material.diffuseMap", 0);
                minecraftShader.setUniform("material.specular", glm::vec3(0.1f, 0.1f, 0.1f));
                minecraftShader.setUniform("material.shininess", 8.0f);

                for (int i = 0; i < numTexturesToBind; i++) {
                        gBlockTextures[i].bind(i);
                        std::string samplerName = "material.diffuseMaps[" + std::to_string(i) + "]";
                        minecraftShader.setUniformSampler(samplerName.c_str(), i);
                }

                // === ÉTAPE 5: Dessiner le monde ===
                world.draw();

                // === ÉTAPE 6: Dessiner les modèles ===
                for (const auto& modelData : scene.models) {
                        Mesh* mesh = meshCache.count(modelData.meshFile) ? meshCache.at(modelData.meshFile) : nullptr;
                        Texture2D* texture = modelTextureCache.count(modelData.textureFile) ? modelTextureCache.at(modelData.textureFile) : nullptr;

                        if (mesh) {
                                glm::mat4 modelMatrix = glm::mat4(1.0f);
                                modelMatrix = glm::translate(modelMatrix, modelData.position);
                                modelMatrix = glm::rotate(modelMatrix, glm::radians(modelData.rotation.angle), modelData.rotation.axis);
                                modelMatrix = glm::scale(modelMatrix, modelData.scale);

                                minecraftShader.setUniform("model", modelMatrix);

                                if (texture) {
                                        texture->bind(0);
                                        minecraftShader.setUniformSampler("material.diffuseMap", 0);
                                }

                                mesh->draw();

                                if (texture) {
                                        texture->unbind(0);
                                }
                        }
                }

                // Unbind textures and shadow map
                for (int i = 0; i < numTexturesToBind; i++) {
                        gBlockTextures[i].unbind(i);
                }
                glActiveTexture(GL_TEXTURE0 + MAX_BLOCK_TEXTURES);
                glBindTexture(GL_TEXTURE_2D, 0);


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

        for (auto pair : meshCache) {
                delete pair.second;
        }
        for (auto pair : modelTextureCache) {
                delete pair.second;
        }

        glDeleteFramebuffers(1, &gShadowMapFBO);
        glDeleteTextures(1, &gShadowMap);
        delete shadowShader; // AJOUTÉ
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