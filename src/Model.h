#ifndef MODEL_H
#define MODEL_H
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

struct Rotation {
        float angle;
        glm::vec3 axis;
};

class Model {
public:
        Model(glm::vec3 pos, glm::vec3 scl, float angle, glm::vec3 axis, std::string meshF, std::string textureF = "");
        Model(glm::vec3 pos, glm::vec3 scl, Rotation rot, std::string meshF, std::string textureF = "");
        ~Model() = default;

        glm::vec3 position;
        glm::vec3 scale;
        Rotation rotation;
        std::string meshFile;
        std::string textureFile;
};

Model::Model(glm::vec3 pos, glm::vec3 scl, float angle, glm::vec3 axis, std::string meshF, std::string textureF)
    : position(pos), scale(scl), meshFile(meshF), textureFile(textureF) {
        rotation.angle = angle;
        rotation.axis = axis;
        std::cout << "Model created at position (" << position.x << ", " << position.y << ", " << position.z << ") with scale (" << scale.x << ", " << scale.y << ", " << scale.z << ") and rotation angle " << rotation.angle << " around axis (" << rotation.axis.x << ", " << rotation.axis.y << ", " << rotation.axis.z << ")" << std::endl;
}

Model::Model(glm::vec3 pos, glm::vec3 scl, Rotation rot, std::string meshF, std::string textureF)
    : position(pos), scale(scl), rotation(rot), meshFile(meshF), textureFile(textureF)
{ }

#endif // MODEL_H