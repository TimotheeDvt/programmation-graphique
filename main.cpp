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
#include "./src/Mesh.h"
#include "./src/Scene.h"


class TextureManager {
private:
        std::map<std::string, Texture2D*> textureCache;

public:
        ~TextureManager() {
                // Nettoyer toutes les textures
                for (auto& pair : textureCache) {
                        delete pair.second;
                }
                textureCache.clear();
        }

        Texture2D* loadTexture(const std::string& filepath) {
                if (filepath.empty()) {
                        return nullptr;
                }

                // Vérifier si la texture est déjà chargée
                auto it = textureCache.find(filepath);
                if (it != textureCache.end()) {
                        return it->second;
                }

                // Charger la nouvelle texture
                Texture2D* newTexture = new Texture2D();
                if (newTexture->loadTexture(filepath, true)) {
                        textureCache[filepath] = newTexture;
                        std::cout << "Loaded texture: " << filepath << std::endl;
                        return newTexture;
                } else {
                        delete newTexture;
                        std::cerr << "Failed to load texture: " << filepath << std::endl;
                        return nullptr;
                }
        }

        void bindTexture(const std::string& filepath, GLuint textureUnit) {
                Texture2D* tex = loadTexture(filepath);
                if (tex) {
                        tex->bind(textureUnit);
                }
        }

        void unbindTexture(GLuint textureUnit) {
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                glBindTexture(GL_TEXTURE_2D, 0);
        }
};


const char* APP_TITLE = "Hello Shaders";
int gWindowWidth = 1024;
int gWindowHeight = 768;
GLFWwindow* gWindow = NULL;
bool gWireframe = false;
bool gFlashlightOn = true;

float cubeAngle = 0.0f;

FPSCamera fpsCamera(glm::vec3(0.0f, 2.0f, 10.0f));
const double ZOOM_SENSITIVITY = -3.0;
const float MOVE_SPEED = 2.0; // units / sec
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
        };

        ShaderProgram lightShader;
        lightShader.loadShaders("basic.vert", "basic.frag");

        ShaderProgram lightingShader;
        lightingShader.loadShaders("lighting_dir.vert", "lighting_dir.frag");

        ShaderProgram pbrShader;
        pbrShader.loadShaders("lighting_pbr.vert", "lighting_pbr.frag");

        Scene scene;
        const int numModels = 1;
        Mesh mesh[numModels];

        // Créer le gestionnaire de textures
        TextureManager texManager;

        // Charger les meshes (les textures seront chargées à la demande)
        for (int i = 0; i < numModels; ++i) {
                mesh[i].loadObj(scene.models[i].meshFile);
        }

        glm::vec3 lightPos = fpsCamera.getPosition();
        lightPos.y -= 0.5f;
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        glm::vec3 lightDirection(0.0f, -0.9f, -0.17f);
        double lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(gWindow)) {
                showFPS(gWindow);

                double currentTime = glfwGetTime();
                double deltaTime = currentTime - lastTime;

                glfwPollEvents();
                update(deltaTime);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glm::mat4 model(1.0), view(1.0), projection(1.0);

                view = fpsCamera.getViewMatrix();

                const float fov = fpsCamera.getFOV();
                const float aspectRatio = (float)gWindowWidth/(float)gWindowHeight;
                projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);

                glm::vec3 viewPos = fpsCamera.getPosition();
                glm::vec3 lightDirection(0.0f, -0.9f, -0.17f);
                glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

                pbrShader.use();
                pbrShader.setUniform("view", view);
                pbrShader.setUniform("viewPos", viewPos);
                pbrShader.setUniform("projection", projection);

                lightingShader.use();
                lightingShader.setUniform("view", view);
                lightingShader.setUniform("viewPos", viewPos);
                lightingShader.setUniform("projection", projection);

                lightingShader.setUniform("light.direction", lightDirection);
                lightingShader.setUniform("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
                lightingShader.setUniform("light.diffuse", lightColor);
                lightingShader.setUniform("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));

                for (int i = 0; i < numModels; i++) {
                        model = glm::translate(glm::mat4(1.0f), scene.models[i].position)
                                * glm::scale(glm::mat4(1.0f), scene.models[i].scale)
                                * glm::rotate(glm::mat4(1.0f), glm::radians(scene.models[i].rotation.angle), scene.models[i].rotation.axis);

                        pbrShader.setUniform("model", model);

                        // Extraire le chemin de base du fichier OBJ
                        std::string objPath = scene.models[i].meshFile;
                        size_t lastSlash = objPath.find_last_of("/\\");
                        std::string basePath = (lastSlash != std::string::npos) ? objPath.substr(0, lastSlash + 1) : "";

                        const auto& subMeshes = mesh[i].getSubMeshes();

                        if (subMeshes.empty()) {
                                // Pas de submeshes, rendu simple
                                lightingShader.setUniform("material.ambient", glm::vec3(0.1f, 0.1f, 0.1f));
                                lightingShader.setUniform("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
                                lightingShader.setUniform("material.shininess", 32.0f);
                                lightingShader.setUniform("material.hasNormalMap", false);
                                lightingShader.setUniform("material.hasRoughnessMap", false);
                                lightingShader.setUniform("material.hasMetallicMap", false);
                                lightingShader.setUniform("material.hasEmissiveMap", false);

                                // Utiliser la texture par défaut du modèle
                                if (!scene.models[i].textureFile.empty()) {
                                        texManager.bindTexture(scene.models[i].textureFile, 0);
                                }
                                lightingShader.setUniformSampler("material.diffuseMap", 0);

                                mesh[i].draw();
                                texManager.unbindTexture(0);
                        } else {
                                // Dessiner chaque submesh avec son matériau complet
                                for (const auto& submeshPair : subMeshes) {
                                        const std::string& materialName = submeshPair.first;
                                        const Material* mat = mesh[i].getMaterial(materialName);

                                        if (mat) {
                                                // Propriétés de base
                                                lightingShader.setUniform("material.ambient", mat->ambient);
                                                lightingShader.setUniform("material.specular", mat->specular);
                                                lightingShader.setUniform("material.shininess", mat->shininess);
                                                lightingShader.setUniform("material.bumpMultiplier", mat->bumpMultiplier);

                                                // Texture diffuse (slot 0)
                                                if (!mat->diffuseMap.empty()) {
                                                        texManager.bindTexture(basePath + mat->diffuseMap, 0);
                                                }
                                                lightingShader.setUniformSampler("material.diffuseMap", 0);

                                                // Normal map (slot 1)
                                                bool hasNormal = !mat->normalMap.empty();
                                                pbrShader.setUniform("material.hasNormalMap", hasNormal);
                                                if (hasNormal) {
                                                        texManager.bindTexture(basePath + mat->normalMap, 1);
                                                        pbrShader.setUniformSampler("material.normalMap", 1);
                                                }

                                                // Roughness map (slot 2)
                                                bool hasRoughness = !mat->roughnessMap.empty();
                                                pbrShader.setUniform("material.hasRoughnessMap", hasRoughness);
                                                if (hasRoughness) {
                                                        texManager.bindTexture(basePath + mat->roughnessMap, 2);
                                                        pbrShader.setUniformSampler("material.roughnessMap", 2);
                                                }

                                                // Metallic map (slot 3)
                                                bool hasMetallic = !mat->metallicMap.empty();
                                                pbrShader.setUniform("material.hasMetallicMap", hasMetallic);
                                                if (hasMetallic) {
                                                        texManager.bindTexture(basePath + mat->metallicMap, 3);
                                                        pbrShader.setUniformSampler("material.metallicMap", 3);
                                                }

                                                // Emissive map (slot 4)
                                                bool hasEmissive = !mat->emissiveMap.empty();
                                                pbrShader.setUniform("material.hasEmissiveMap", hasEmissive);
                                                if (hasEmissive) {
                                                        texManager.bindTexture(basePath + mat->emissiveMap, 4);
                                                        pbrShader.setUniformSampler("material.emissiveMap", 4);
                                                }

                                                // Dessiner le submesh
                                                mesh[i].drawSubMesh(materialName);

                                                // Unbind toutes les textures
                                                for (int slot = 0; slot < 5; ++slot) {
                                                        texManager.unbindTexture(slot);
                                                }
                                        } else {
                                                // Matériau non trouvé, utiliser les valeurs par défaut
                                                lightingShader.setUniform("material.ambient", glm::vec3(0.1f));
                                                lightingShader.setUniform("material.specular", glm::vec3(0.5f));
                                                lightingShader.setUniform("material.shininess", 32.0f);
                                                pbrShader.setUniform("material.hasNormalMap", false);
                                                pbrShader.setUniform("material.hasRoughnessMap", false);
                                                pbrShader.setUniform("material.hasMetallicMap", false);
                                                pbrShader.setUniform("material.hasEmissiveMap", false);

                                                mesh[i].drawSubMesh(materialName);
                                        }
                                }
                        }
                }

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
        glfwSwapInterval(0);  // disable vsync for better fps

        glfwSetKeyCallback(gWindow, glfw_onkey);
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

        glClearColor(0.23f, 0.38f, 0.47f, 1.0f);
        glViewport(0, 0, gWindowWidth, gWindowHeight);
        glEnable(GL_DEPTH_TEST);
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
                case GLFW_KEY_F:
                        gFlashlightOn = !gFlashlightOn;
                        std::cout << "Flashlight " << (gFlashlightOn ? "ON" : "OFF") << std::endl;
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
        // static glm::vec2 lastMousePos = glm::vec2(0, 0);

        // if (glfwGetMouseButton(gWindow, GLFW_MOUSE_BUTTON_LEFT) == 1) {
        //         gYaw -= ((float)posX - lastMousePos.x) * MOUSE_SENSITIVITY;
        //         gPitch += ((float)posY - lastMousePos.y) * MOUSE_SENSITIVITY;
        // }

        // if (glfwGetMouseButton(gWindow, GLFW_MOUSE_BUTTON_RIGHT) == 1) {
        //         float dx = 0.01f + ((float)posX - lastMousePos.x);
        //         float dy = 0.01f + ((float)posY - lastMousePos.y);
        //         gRadius += dx - dy;
        // }

        // lastMousePos.x = (float)posX;
        // lastMousePos.y = (float)posY;
}

void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY) {
        double fov = fpsCamera.getFOV() + deltaY * ZOOM_SENSITIVITY;

        fov = glm::clamp(fov, 1.0, 120.0);

        fpsCamera.setFOV((float)fov);
}

void update(double elapsedTime) {
        double mouseX, mouseY;

        glfwGetCursorPos(gWindow, &mouseX, &mouseY);

        fpsCamera.rotate((float)(gWindowWidth/2.0 - mouseX) * MOUSE_SENSITIVITY, (float)(gWindowHeight/2.0 - mouseY) * MOUSE_SENSITIVITY);

        glfwSetCursorPos(gWindow, gWindowWidth/2.0, gWindowHeight/2.0);

        // Mouvement avant/arrière
        if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getLook());
        } else if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getLook());
        }

        // Mouvement gauche/droite
        if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getRight());
        } else if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getRight());
        }

        // Mouvement haut/bas (inchangé)
        if (glfwGetKey(gWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getUp());
        } else if (glfwGetKey(gWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getUp());
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