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
#include <glm/gtc/matrix_inverse.hpp>
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

Scene scene;
std::map<std::string, Mesh*> meshCache;
std::map<std::string, Texture2D*> modelTextureCache;

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

// Directional Shadow Map
const unsigned int DIR_SHADOW_WIDTH = 2048; // Réduit pour la perf (anciennement 4096)
const unsigned int DIR_SHADOW_HEIGHT = 2048; // Réduit pour la perf
GLuint gDirShadowMapFBO;
GLuint gDirShadowMap;

// Point Light Shadow Map (Cube Map)
const unsigned int POINT_SHADOW_WIDTH = 1024;
const unsigned int POINT_SHADOW_HEIGHT = 1024;
const float POINT_NEAR_PLANE = 0.1f;
const float POINT_FAR_PLANE = 20.0f;
GLuint gPointShadowMapFBO;
GLuint gPointShadowMap;
ShaderProgram* pointDepthShader = nullptr; // Shader pour la passe de profondeur des points

// Spot Light Shadow Maps (2D Maps Array)
const unsigned int SPOT_SHADOW_WIDTH = 1024;
const unsigned int SPOT_SHADOW_HEIGHT = 1024;
const float SPOT_NEAR_PLANE = 0.1f;
const float SPOT_FAR_PLANE = 30.0f;
// On utilise les textures des points de lumières existantes pour les spots
GLuint gSpotShadowMapFBOs[MAX_SPOT_LIGHTS];
GLuint gSpotShadowMaps[MAX_SPOT_LIGHTS];

// Shader de profondeur générique (réutilise shadow_dir.vert/frag)
ShaderProgram* depthShader = nullptr;

bool initShadows() {
        // --- 1. Directional Shadow Map ---
        glGenFramebuffers(1, &gDirShadowMapFBO);
        glGenTextures(1, &gDirShadowMap);
        glBindTexture(GL_TEXTURE_2D, gDirShadowMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                DIR_SHADOW_WIDTH, DIR_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindFramebuffer(GL_FRAMEBUFFER, gDirShadowMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDirShadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Dir Shadow Framebuffer not complete!" << std::endl;
                return false;
        }

        // --- 2. Point Light Shadow Map (Cube Map) ---
        glGenFramebuffers(1, &gPointShadowMapFBO);
        glGenTextures(1, &gPointShadowMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, gPointShadowMap);
        for (unsigned int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                        POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL
                );
        }
        // Utiliser GL_LINEAR pour un meilleur lissage des ombres
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, gPointShadowMapFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gPointShadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Point Shadow Framebuffer not complete!" << std::endl;
                return false;
        }

        // --- 3. Spot Light Shadow Maps (Array of 2D Maps) ---
        for (int i = 0; i < MAX_SPOT_LIGHTS; ++i) {
                glGenFramebuffers(1, &gSpotShadowMapFBOs[i]);
                glGenTextures(1, &gSpotShadowMaps[i]);
                glBindTexture(GL_TEXTURE_2D, gSpotShadowMaps[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                        SPOT_SHADOW_WIDTH, SPOT_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL
                );
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
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

        if(!initShadows()) {
                std::cerr << "Shadow initialization failed" << std::endl;
                return -1;
        }

        // NOUVEAU: Chargement des shaders de profondeur
        depthShader = new ShaderProgram();
        // Utilise le shader de profondeur simple pour Dir/Spot (écrit Z/W)
        depthShader->loadShaders("./shadow_dir.vert", "./shadow_dir.frag");

        pointDepthShader = new ShaderProgram();
        // Shader pour point light (écrit la distance linéaire normalisée)
        // Réutilise shadow_dir.vert car il est simple (juste la position)
        pointDepthShader->loadShaders("./shadow_dir.vert", "./depth_point.frag");


        if (blueDebug) {
                // ... (debug shader init) ...
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

        // ... (model loading, texture loading) ...
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

        glm::mat4 lightSpaceMatrix; // pour la lumière directionnelle
        glm::mat4 spotLightSpaceMatrices[MAX_SPOT_LIGHTS];

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
                        glm::vec3 lookDirection = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
                        glm::vec3 eyeLightPos = modelData.position + glm::vec3(0.1f, 2.75f, 0.4f);
                        spotLights.push_back({eyeLightPos, lookDirection});
                        eyeLightPos = modelData.position + glm::vec3(-0.1f, 2.75f, 0.45f);
                        spotLights.push_back({eyeLightPos, lookDirection});
                }
                int spotLightCount = spotLights.size();

                showFPS(gWindow);

                double currentTime = glfwGetTime();
                double deltaTime = currentTime - lastTime;

                glfwPollEvents();
                update(deltaTime);

                // --- PASS 1A: Rendu de la carte de profondeur Directionnelle ---
                glm::vec3 sunDirection(-0.3f, -0.8f, -0.5f);
                float dir_near_plane = 1.0f, dir_far_plane = 70.0f;
                glm::mat4 dirLightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, dir_near_plane, dir_far_plane);
                glm::vec3 centerPos = fpsCamera.getPosition();
                glm::mat4 dirLightView = glm::lookAt(centerPos - sunDirection * 20.0f, centerPos, glm::vec3(0.0f, 1.0f, 0.0f));
                lightSpaceMatrix = dirLightProjection * dirLightView;

                glViewport(0, 0, DIR_SHADOW_WIDTH, DIR_SHADOW_HEIGHT);
                glBindFramebuffer(GL_FRAMEBUFFER, gDirShadowMapFBO);
                glClear(GL_DEPTH_BUFFER_BIT);
                glCullFace(GL_FRONT);

                depthShader->use(); // Utilise le shader simple pour la profondeur (z/w)
                depthShader->setUniform("lightSpaceMatrix", lightSpaceMatrix);

                glm::mat4 model = glm::mat4(1.0f);
                depthShader->setUniform("model", model);
                world.draw();

                for (const auto& modelData : scene.models) {
                        Mesh* mesh = meshCache.count(modelData.meshFile) ? meshCache.at(modelData.meshFile) : nullptr;
                        if (mesh) {
                                glm::mat4 modelMatrix = glm::mat4(1.0f);
                                modelMatrix = glm::translate(modelMatrix, modelData.position);
                                modelMatrix = glm::rotate(modelMatrix, glm::radians(modelData.rotation.angle), modelData.rotation.axis);
                                modelMatrix = glm::scale(modelMatrix, modelData.scale);
                                depthShader->setUniform("model", modelMatrix);
                                mesh->draw();
                        }
                }
                glCullFace(GL_BACK);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);


                // --- PASS 1B: Rendu de la carte de profondeur Cube Map (Point Lights) ---
                glm::mat4 pointShadowProj = glm::perspective(glm::radians(90.0f), (float)POINT_SHADOW_WIDTH / (float)POINT_SHADOW_HEIGHT, POINT_NEAR_PLANE, POINT_FAR_PLANE);
                std::vector<glm::mat4> pointShadowTransforms;

                glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);
                glBindFramebuffer(GL_FRAMEBUFFER, gPointShadowMapFBO);

                pointDepthShader->use(); // Utilise le shader qui écrit la distance linéaire normalisée
                pointDepthShader->setUniform("farPlane", POINT_FAR_PLANE);

                for (int i = 0; i < worldLightCount; i++) {
                        glm::vec3 lightPos = frameLights[i];
                        pointDepthShader->setUniform("lightPos", lightPos);

                        pointShadowTransforms.clear();
                        // Les 6 directions (faces)
                        pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
                        pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
                        pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
                        pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
                        pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
                        pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));

                        // Rendu de la scène
                        for (int j = 0; j < 6; ++j) {
                                // Attacher la j-ième face du Cube Map
                                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, gPointShadowMap, 0);
                                glClear(GL_DEPTH_BUFFER_BIT);

                                pointDepthShader->setUniform("lightSpaceMatrix", pointShadowTransforms[j]);

                                pointDepthShader->setUniform("model", model);
                                world.draw();

                                for (const auto& modelData : scene.models) {
                                        Mesh* mesh = meshCache.count(modelData.meshFile) ? meshCache.at(modelData.meshFile) : nullptr;
                                        if (mesh) {
                                                glm::mat4 modelMatrix = glm::mat4(1.0f);
                                                modelMatrix = glm::translate(modelMatrix, modelData.position);
                                                modelMatrix = glm::rotate(modelMatrix, glm::radians(modelData.rotation.angle), modelData.rotation.axis);
                                                modelMatrix = glm::scale(modelMatrix, modelData.scale);
                                                pointDepthShader->setUniform("model", modelMatrix);
                                                mesh->draw();
                                        }
                                }
                        }
                }
                glBindFramebuffer(GL_FRAMEBUFFER, 0);


                // --- PASS 1C: Rendu des Shadow Maps 2D (Spot Lights) ---
                glViewport(0, 0, SPOT_SHADOW_WIDTH, SPOT_SHADOW_HEIGHT);
                depthShader->use();

                for (int i = 0; i < spotLightCount; ++i) {
                        glm::vec3 lightPos = spotLights[i].first;
                        glm::vec3 lightDir = spotLights[i].second;

                        // FOV basé sur le cosOuterCone du Enderman (25.0f * 2)
                        float outerCone = 25.0f;
                        float fov = glm::radians(outerCone * 2.0f);

                        glm::mat4 spotProjection = glm::perspective(fov, 1.0f, SPOT_NEAR_PLANE, SPOT_FAR_PLANE);
                        glm::mat4 spotView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
                        spotLightSpaceMatrices[i] = spotProjection * spotView;

                        // Attacher le FBO spécifique au spot light i
                        glBindFramebuffer(GL_FRAMEBUFFER, gSpotShadowMapFBOs[i]);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gSpotShadowMaps[i], 0);
                        glClear(GL_DEPTH_BUFFER_BIT);

                        depthShader->setUniform("lightSpaceMatrix", spotLightSpaceMatrices[i]);

                        depthShader->setUniform("model", model);
                        world.draw();

                        for (const auto& modelData : scene.models) {
                                Mesh* mesh = meshCache.count(modelData.meshFile) ? meshCache.at(modelData.meshFile) : nullptr;
                                if (mesh) {
                                        glm::mat4 modelMatrix = glm::mat4(1.0f);
                                        modelMatrix = glm::translate(modelMatrix, modelData.position);
                                        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelData.rotation.angle), modelData.rotation.axis);
                                        modelMatrix = glm::scale(modelMatrix, modelData.scale);
                                        depthShader->setUniform("model", modelMatrix);
                                        mesh->draw();
                                }
                        }
                }

                glBindFramebuffer(GL_FRAMEBUFFER, 0);


                // --- PASS 2: Rendu de la scène principale avec ombres ---
                glViewport(0, 0, gWindowWidth, gWindowHeight);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glm::mat4 view = fpsCamera.getViewMatrix();
                float fov = fpsCamera.getFOV();
                float aspectRatio = (float)gWindowWidth / (float)gWindowHeight / 1.0f;
                glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 200.0f);
                glm::vec3 viewPos = fpsCamera.getPosition();

                ShaderProgram& minecraftShaderRef = minecraftShader; // Utiliser une référence

                minecraftShaderRef.use();

                model = glm::mat4(1.0f);
                minecraftShaderRef.setUniform("model", model);
                minecraftShaderRef.setUniform("view", view);
                minecraftShaderRef.setUniform("projection", projection);
                minecraftShaderRef.setUniform("viewPos", viewPos);

                // Directional Shadow Map
                minecraftShaderRef.setUniform("lightSpaceMatrix", lightSpaceMatrix);
                glActiveTexture(GL_TEXTURE0 + MAX_BLOCK_TEXTURES);
                glBindTexture(GL_TEXTURE_2D, gDirShadowMap);
                minecraftShaderRef.setUniformSampler("dirShadowMap", MAX_BLOCK_TEXTURES);

                // Point Light Shadow Map
                glActiveTexture(GL_TEXTURE0 + MAX_BLOCK_TEXTURES + 1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, gPointShadowMap);
                minecraftShaderRef.setUniformSampler("pointShadowMap", MAX_BLOCK_TEXTURES + 1);
                minecraftShaderRef.setUniform("pointFarPlane", POINT_FAR_PLANE);

                // Spot Light Shadow Maps
                for (int i = 0; i < spotLightCount; ++i) {
                        std::string matrixBase = "spotLightSpaceMatrices[" + std::to_string(i) + "]";
                        minecraftShaderRef.setUniform(matrixBase.c_str(), spotLightSpaceMatrices[i]);

                        glActiveTexture(GL_TEXTURE0 + MAX_BLOCK_TEXTURES + 2 + i);
                        glBindTexture(GL_TEXTURE_2D, gSpotShadowMaps[i]);
                        std::string samplerBase = "spotShadowMaps[" + std::to_string(i) + "]";
                        minecraftShaderRef.setUniformSampler(samplerBase.c_str(), MAX_BLOCK_TEXTURES + 2 + i);
                }

                // Directional light
                minecraftShaderRef.setUniform("dirLight.direction", sunDirection);
                minecraftShaderRef.setUniform("dirLight.ambient", glm::vec3(0.3f, 0.3f, 0.35f)*0.2f);
                minecraftShaderRef.setUniform("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.7f)*0.5f);
                minecraftShaderRef.setUniform("dirLight.specular", glm::vec3(0.3f, 0.3f, 0.3f)*0.5f);

                // Point lights
                minecraftShaderRef.setUniform("numPointLights", worldLightCount);

                for (int i = 0; i < worldLightCount; i++) {
                        std::string base = "pointLights[" + std::to_string(i) + "]";
                        minecraftShaderRef.setUniform((base + ".position").c_str(), frameLights[i]);
                        minecraftShaderRef.setUniform((base + ".constant").c_str(), 1.0f);
                        minecraftShaderRef.setUniform((base + ".ambient").c_str(), glm::vec3(0.0f));

                        if (i < redstoneLightCount) {
                                minecraftShaderRef.setUniform((base + ".diffuse").c_str(), glm::vec3(1.0f, 0.1f, 0.1f));
                                minecraftShaderRef.setUniform((base + ".specular").c_str(), glm::vec3(0.5f, 0.1f, 0.1f));
                                minecraftShaderRef.setUniform((base + ".linear").c_str(), 0.14f);
                                minecraftShaderRef.setUniform((base + ".exponant").c_str(), 0.07f);
                        }
                        else {
                                minecraftShaderRef.setUniform((base + ".diffuse").c_str(), glm::vec3(1.0f, 0.8f, 0.2f));
                                minecraftShaderRef.setUniform((base + ".specular").c_str(), glm::vec3(0.5f, 0.4f, 0.1f));
                                minecraftShaderRef.setUniform((base + ".linear").c_str(), 0.22f);
                                minecraftShaderRef.setUniform((base + ".exponant").c_str(), 0.20f);
                        }
                }

                // Spotlights
                minecraftShaderRef.setUniform("numSpotLights", spotLightCount);

                for (int i = 0; i < spotLightCount; i++) {
                        std::string base = "spotLights[" + std::to_string(i) + "]";
                        minecraftShaderRef.setUniform((base + ".position").c_str(), spotLights[i].first);
                        minecraftShaderRef.setUniform((base + ".direction").c_str(), spotLights[i].second);
                        minecraftShaderRef.setUniform((base + ".ambient").c_str(), glm::vec3(0.0f));
                        minecraftShaderRef.setUniform((base + ".diffuse").c_str(), glm::vec3(0.8f, 0.0f, 0.8f) * 8.0f);
                        minecraftShaderRef.setUniform((base + ".specular").c_str(), glm::vec3(0.6f, 0.0f, 0.6f) * 8.0f);
                        minecraftShaderRef.setUniform((base + ".constant").c_str(), 1.0f);
                        minecraftShaderRef.setUniform((base + ".linear").c_str(), 0.09f);
                        minecraftShaderRef.setUniform((base + ".exponant").c_str(), 0.032f);
                        minecraftShaderRef.setUniform((base + ".cosInnerCone").c_str(), glm::cos(glm::radians(12.5f)));
                        minecraftShaderRef.setUniform((base + ".cosOuterCone").c_str(), glm::cos(glm::radians(25.0f)));
                }

                // Materials
                minecraftShaderRef.setUniform("material.ambient", glm::vec3(1.0f, 1.0f, 1.0f));
                minecraftShaderRef.setUniformSampler("material.diffuseMap", 0);
                minecraftShaderRef.setUniform("material.specular", glm::vec3(0.1f, 0.1f, 0.1f));
                minecraftShaderRef.setUniform("material.shininess", 8.0f);

                for (int i = 0; i < numTexturesToBind; i++) {
                        gBlockTextures[i].bind(i);
                        std::string samplerName = "material.diffuseMaps[" + std::to_string(i) + "]";
                        minecraftShaderRef.setUniformSampler(samplerName.c_str(), i);
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

                                minecraftShaderRef.setUniform("model", modelMatrix);

                                if (texture) {
                                        texture->bind(0);
                                        minecraftShaderRef.setUniformSampler("material.diffuseMap", 0);
                                }

                                mesh->draw();

                                if (texture) {
                                        texture->unbind(0);
                                }
                        }
                }

                // Unbind textures and shadow maps
                for (int i = 0; i < numTexturesToBind; i++) {
                        gBlockTextures[i].unbind(i);
                }
                glActiveTexture(GL_TEXTURE0 + MAX_BLOCK_TEXTURES);
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE0 + MAX_BLOCK_TEXTURES + 1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                for (int i = 0; i < spotLightCount; ++i) {
                        glActiveTexture(GL_TEXTURE0 + MAX_BLOCK_TEXTURES + 2 + i);
                        glBindTexture(GL_TEXTURE_2D, 0);
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

        for (auto pair : meshCache) {
                delete pair.second;
        }
        for (auto pair : modelTextureCache) {
                delete pair.second;
        }

        glDeleteFramebuffers(1, &gDirShadowMapFBO);
        glDeleteTextures(1, &gDirShadowMap);
        glDeleteFramebuffers(1, &gPointShadowMapFBO);
        glDeleteTextures(1, &gPointShadowMap);
        for (int i = 0; i < MAX_SPOT_LIGHTS; ++i) {
                glDeleteFramebuffers(1, &gSpotShadowMapFBOs[i]);
                glDeleteTextures(1, &gSpotShadowMaps[i]);
        }

        delete depthShader;
        delete pointDepthShader;
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

        float physicsMoveSpeed = 5.0f; // Vitesse d'accélération au sol
        if (glfwGetKey(gWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                MOVE_SPEED = 80.0f;
                physicsMoveSpeed = 10.0f;
        } else {
                MOVE_SPEED = 20.0f;
                physicsMoveSpeed = 5.0f;
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