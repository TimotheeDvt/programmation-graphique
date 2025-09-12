#include <iostream>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// cmake --build build --config Release ; .\build\Release\main.exe

const char* APP_TITLE = "Introduction to Modern OpenGL : Hello window !";
const int gWindowWidth = 800;
const int gWindowHeight = 600;
bool gFullScreen = false;

void glfw_onkey(GLFWwindow* window, int key, int scancode, int action, int mode);
void showFPS(GLFWwindow* window);

int main() {
        if (!glfwInit()) {
                std::cerr << "GLFW initialization failed" << std::endl;
                return -1;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* pWindow = NULL;
        if (gFullScreen) {
                GLFWmonitor* pMonitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* pVMode = glfwGetVideoMode(pMonitor);
                if (pVMode != NULL) {
                        pWindow = glfwCreateWindow(pVMode->width, pVMode->height, APP_TITLE, pMonitor, NULL);
                }
        } else {
                pWindow = glfwCreateWindow(gWindowWidth, gWindowHeight, APP_TITLE, NULL, NULL);
        }

        if (pWindow == NULL) {
                std::cerr << "GLFWwindow initialization failed" << std::endl;
                glfwTerminate();
                return -1;
        }

        glfwMakeContextCurrent(pWindow);

        glfwSetKeyCallback(pWindow, glfw_onkey);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
                std::cerr << "GLEW initialization failed" << std::endl;
                glfwTerminate();
                return -1;
        }

        // Main loop
        while (!glfwWindowShouldClose(pWindow)) {
                showFPS(pWindow);
                glfwPollEvents();

                glClearColor(0.23f, 0.38f, 0.47f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                glfwSwapBuffers(pWindow);
        }

        glfwTerminate();
        return 0;
}


void glfw_onkey(GLFWwindow* window, int key, int scancode, int action, int mode) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GL_TRUE);
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