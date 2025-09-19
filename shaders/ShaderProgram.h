#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>
using std::string;
#include <map>

class ShaderProgram {
    public:
        ShaderProgram(/* args */);
        ~ShaderProgram();

        enum ShaderType {
            VERTEX,
            FRAGMENT,
            PROGRAM
        };

        bool loadShaders(const char* vsFilename, const char* fsFilename);
        void use();

        void setUniform(const GLchar* name, const glm::vec2& v);
        void setUniform(const GLchar* name, const glm::vec3& v);
        void setUniform(const GLchar* name, const glm::vec4& v);

    private:
        string fileToString(const string& filename);
        void checkCompileErrors(GLuint shader, ShaderType type);
        GLint getUniformLocation(const GLchar* name);

        GLuint mHandle;

        std::map<string, GLint> mUniformLocations;
};

#endif