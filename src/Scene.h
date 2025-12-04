#ifndef SCENE_H
#define SCENE_H
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"
#include <vector>

extern Model endermanModel;

class Scene {
public:
    Scene() {
        models.push_back(endermanModel);
    }
    ~Scene() = default;

    std::vector<Model> models;
};

#endif // SCENE_H