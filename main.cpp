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

const char* APP_TITLE = "Hello Shaders";
int gWindowWidth = 1024;
int gWindowHeight = 768;
GLFWwindow* gWindow = NULL;
bool gWireframe = false;
bool gFlashlightOn = true;

float cubeAngle = 0.0f;

FPSCamera fpsCamera(glm::vec3(0.0f, 2.0f, 10.0f));
const double ZOOM_SENSITIVITY = -3.0;
const float MOVE_SPEED = 10.0; // units / sec
const float MOUSE_SENSITIVITY = 0.1f;

void glfw_onkey(GLFWwindow* window, int key, int scancode, int action, int mode);
void glfw_onFrameBufferSize(GLFWwindow* window, int width, int height);
void glfw_onMouseMove(GLFWwindow* window, double posX, double posY);
void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY);
void update(double elapsedTime);
void showFPS(GLFWwindow* window);
bool initOpenGL();

struct Rotation {
        float angle;
        glm::vec3 axis;
};

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

        glm::vec3 modelPos[] = {
                glm::vec3( 0.0f, 0.0f,  0.0f), // floor
                glm::vec3( 2.5f, 1.0f,  0.0f), // guitar
                glm::vec3( -2.5f, 1.0f, 0.0f), // table
                glm::vec3( 0.0f, 0.0f,  0.0f)  // floor
        };

        glm::vec3 modelScale[] = {
                glm::vec3(10.0f, 0.1f, 10.0f), // floor
                glm::vec3( 1.0f, 1.0f,  1.0f), // guitar
                glm::vec3(1.0f, 1.0f, 1.0f), // table
                glm::vec3(10.0f, 0.1f, 10.0f)  // floor
        };

        const int numModels = 3;
        Mesh mesh[numModels];
        Texture2D texture[numModels];

        Rotation modelRot[numModels];
        for (int i = 0; i < numModels; ++i) {
                Rotation tempRot;
                tempRot.angle = 0.0f;
                tempRot.axis = glm::vec3(0.0f, 1.0f, 0.0f);
                modelRot[i] = tempRot;
        }
        Rotation wolf_rot;
        wolf_rot.angle = 270.0f;
        wolf_rot.axis = glm::vec3(1.0f, 0.0f, 0.0f);

        modelRot[2] = wolf_rot;

        mesh[0].loadObj("./models/floor.obj");
        mesh[1].loadObj("./new_models/g/Guitar_01.obj");
        mesh[2].loadObj("./new_models/Wolf-Blender-2.82a.obj");

        texture[0].loadTexture("./img/tile_floor.jpg", true);
        texture[1].loadTexture("./new_models/g/Guitar_01_Textures_UnrealEngine4/guitar_01_BaseColor.png", true);
        texture[1].loadTexture("./new_models/table_basecolor.png", true);

        double lastTime = glfwGetTime();
        float angle = 0;

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

                glm::vec3 viewPos;
                viewPos.x = fpsCamera.getPosition().x;
                viewPos.y = fpsCamera.getPosition().y;
                viewPos.z = fpsCamera.getPosition().z;

                glm::vec3 lightPos(0.0f, 1.0f, 10.0f);
                glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
                glm::vec3 lightDirection(0.0f, -0.9f, -0.17f);

                lightingShader.use();
                lightingShader.setUniform("view", view);
                lightingShader.setUniform("viewPos", viewPos);
                lightingShader.setUniform("projection", projection);

                lightingShader.setUniform("light.direction", lightDirection);
                lightingShader.setUniform("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
                lightingShader.setUniform("light.diffuse", lightColor);
                lightingShader.setUniform("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));

                for (int i = 0; i < numModels; i++) {
                        model = glm::translate(glm::mat4(1.0f), modelPos[i])
                                * glm::scale(glm::mat4(1.0f), modelScale[i])
                                * glm::rotate(glm::mat4(1.0f), glm::radians(modelRot[i].angle), modelRot[i].axis);

                        lightingShader.setUniform("model", model);

                        // Vérifier si le mesh a des submeshes avec matériaux
                        const auto& subMeshes = mesh[i].getSubMeshes();

                        if (subMeshes.empty()) {
                                // Pas de matériaux, utiliser la texture par défaut
                                lightingShader.setUniform("material.ambient", glm::vec3(0.1f, 0.1f, 0.1f));
                                lightingShader.setUniformSampler("material.diffuseMap", 0);
                                lightingShader.setUniform("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
                                lightingShader.setUniform("material.shininess", 32.0f);

                                texture[i].bind(0);
                                mesh[i].draw();
                                texture[i].unbind(0);
                        } else {
                                // Dessiner chaque submesh avec son matériau
                                for (const auto& submeshPair : subMeshes) {
                                const std::string& materialName = submeshPair.first;
                                const SubMesh& submesh = submeshPair.second;

                                // Récupérer les propriétés du matériau
                                const Material* mat = mesh[i].getMaterial(materialName);

                                if (mat) {
                                        lightingShader.setUniform("material.ambient", mat->ambient);
                                        lightingShader.setUniform("material.specular", mat->specular);
                                        lightingShader.setUniform("material.shininess", mat->shininess);

                                        // Si le matériau a une texture, la charger
                                        // (vous devrez créer un système de cache de textures)
                                        // Pour l'instant, utiliser la texture par défaut
                                        lightingShader.setUniformSampler("material.diffuseMap", 0);
                                        texture[i].bind(0);
                                } else {
                                        // Matériau non trouvé, utiliser les valeurs par défaut
                                        lightingShader.setUniform("material.ambient", glm::vec3(0.1f, 0.1f, 0.1f));
                                        lightingShader.setUniformSampler("material.diffuseMap", 0);
                                        lightingShader.setUniform("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
                                        lightingShader.setUniform("material.shininess", 32.0f);
                                        texture[i].bind(0);
                                }

                                // Dessiner ce submesh spécifique
                                mesh[i].drawSubMesh(materialName);
                                texture[i].unbind(0);
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