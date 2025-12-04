#ifndef SCENE_H
#define SCENE_H
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"

Model endermanModel (
    glm::vec3(5.0f, 19.5f, 5.0f), // Position (ajustez ces coordonnées X, Y, Z pour le placer où vous voulez)
    glm::vec3(0.06f, 0.06f, 0.06f), // Échelle réduite pour un Enderman
    0.0f, // angle de rotation
    glm::vec3(0.0f, 1.0f, 0.0f), // axe de rotation
    "../models/EnderMan.obj", // fichier de maillage OBJ
    "../models/enderman.png" // fichier de texture PNG
);

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    Model models[1] = {
        endermanModel
    };
};

#endif // SCENE_H