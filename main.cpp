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

const char* APP_TITLE = "Hello Shaders";
int gWindowWidth = 1024;
int gWindowHeight = 768;
GLFWwindow* gWindow = NULL;
bool gWireframe = false;
const std::string texture1_file = "./img/crate.png";
const std::string texture2_file = "./img/grid.png";

float cubeAngle = 0.0f;

FPSCamera fpsCamera(glm::vec3(0.0f, 0.0f, 5.0f));
const double ZOOM_SENSITIVITY = -3.0;
const float MOVE_SPEED = 5.0; // units / sec
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

        GLfloat vertices[] = {
                // positions      // tex coords
                // front face
                -1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
                1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
                1.0f, -1.0f, 1.0f, 1.0f, 0.0f,

                // back face
                1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
                1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                -1.0f, -1.0f, -1.0f, 1.0f, 0.0f,

                // left face
                -1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                -1.0f, -1.0f, 1.0f, 1.0f, 0.0f,

                // right face
                1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
                1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
                1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
                1.0f, -1.0f, -1.0f, 1.0f, 0.0f,

                // top face
                -1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
                -1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
                1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                -1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
                1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

                // bottom face
                -1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
        };
        glm::vec3 cubePos = glm::vec3(0.0f, 0.0f, -5.0f);
        glm::vec3 floorPos = glm::vec3(0.0f, -1.0f, 0.0f);

        GLuint vbo, vao;

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), NULL);
        glEnableVertexAttribArray(0);

        // tex coord
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        ShaderProgram shaderProgram;
        shaderProgram.loadShaders("basic.vert", "basic.frag");

        Texture2D texture1;
        texture1.loadTexture(texture1_file, true);

        Texture2D texture2;
        texture2.loadTexture(texture2_file, true);

        double lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(gWindow)) {
                showFPS(gWindow);

                double currentTime = glfwGetTime();
                double deltaTime = currentTime - lastTime;

                glfwPollEvents();
                update(deltaTime);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                texture1.bind(0);

                glm::mat4 model(1.0), view(1.0), projection(1.0);

                model = glm::translate(model, cubePos);

                view = fpsCamera.getViewMatrix();

                const float fov = fpsCamera.getFOV();
                const float aspectRatio = (float)gWindowWidth/(float)gWindowHeight;
                projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);

                shaderProgram.use();

                shaderProgram.setUniform("model", model);
                shaderProgram.setUniform("view", view);
                shaderProgram.setUniform("projection", projection);

                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                texture2.bind(0);
                model = glm::mat4(1.0f);
                model = glm::translate(model, floorPos);
                model = glm::scale(model, glm::vec3(10.0f, 0.01f, 10.0f));


                shaderProgram.setUniform("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                glBindVertexArray(0);

                glfwSwapBuffers(gWindow);

                lastTime = currentTime;
        }

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);

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