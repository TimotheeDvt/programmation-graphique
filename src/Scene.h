#ifndef SCENE_H
#define SCENE_H
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"

extern Model endermanModel;

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    Model models[1] = {
        endermanModel
    };
};

#endif // SCENE_H