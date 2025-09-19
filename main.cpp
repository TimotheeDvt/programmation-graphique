#include <iostream>
#include <sstream>
#include <filesystem>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "./src/ShaderProgram.h"
#include "./src/Texture2D.h"

const char* APP_TITLE = "Hello Shaders";
const int gWindowWidth = 800;
const int gWindowHeight = 600;
GLFWwindow* gWindow = NULL;
bool gWireframe = false;

void glfw_onkey(GLFWwindow* window, int key, int scancode, int action, int mode);
void showFPS(GLFWwindow* window);
bool initOpenGL();
const std::string texture1_file = "./img/crate.png";
const std::string texture2_file = "./img/wall.png";

int main() {
        std::cout << "CWD: " << std::filesystem::current_path() << std::endl;
        if(!initOpenGL()) {
                std::cerr << "OpenGL initialization failed" << std::endl;
                return -1;
        };

        GLfloat vertices[] = {
                // Position       // Texture coords
               -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, // top left
                0.5f,  0.5f, 0.0f, 1.0f, 1.0f, // top right
                0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // bottom right
               -0.5f, -0.5f, 0.0f, 0.0f, 0.0f
        };

        GLuint indices[] = {
                0, 1, 2,
                0, 2, 3
        };

        GLuint vbo, ibo, vao;

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

        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        ShaderProgram shaderProgram;
        shaderProgram.loadShaders("basic.vert", "basic.frag");

        Texture2D texture1;
        texture1.loadTexture(texture1_file, true);

        Texture2D texture2;
        texture2.loadTexture(texture2_file, true);

        while (!glfwWindowShouldClose(gWindow)) {
                showFPS(gWindow);
                glfwPollEvents();

                glClear(GL_COLOR_BUFFER_BIT);

                texture1.bind(0);
                texture2.bind(1);

                shaderProgram.use();

                glUniform1i(glGetUniformLocation(shaderProgram.getProgram(), "myTexture1"), 0);
                glUniform1i(glGetUniformLocation(shaderProgram.getProgram(), "myTexture2"), 1);

                GLfloat time = (GLfloat)(glfwGetTime());
                // GLfloat blueColor = (GLfloat)((sin(time * 4) / 2) + 0.5);
                glm::vec2 pos;
                pos.x = sin(time) / 2;
                pos.y = cos(time) / 2;

                // shaderProgram.setUniform("vertColor", glm::vec4(0.0f, 0.0f, blueColor, 1.0f));
                shaderProgram.setUniform("posOffset", pos);

                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                glfwSwapBuffers(gWindow);
        }

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ibo);

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

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
                std::cerr << "GLEW initialization failed" << std::endl;
                glfwTerminate();
                return false;
        }

        glClearColor(0.23f, 0.38f, 0.47f, 1.0f);

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
        case GLFW_KEY_SPACE:
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