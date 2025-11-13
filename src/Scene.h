#ifndef SCENE_H
#define SCENE_H
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "./src/Model.h"

Model floorModel(
        glm::vec3(0.0f, 0.0f, 0.0f), // position
        glm::vec3(10.0f, 0.1f, 10.0f), // scale
        0.0f, // rotation angle
        glm::vec3(0.0f, 1.0f, 0.0f), // rotation axis
        "./models/floor.obj", // mesh file
        "./img/tile_floor.jpg" // texture file
);

Model guitarModel(
        glm::vec3(2.5f, 1.0f, 0.0f), // position
        glm::vec3(1.0f, 1.0f, 1.0f), // scale
        0.0f, // rotation angle
        glm::vec3(0.0f, 1.0f, 0.0f), // rotation axis
        "./new_models/g/Guitar_01.obj", // mesh file
        "./new_models/g/Guitar_01_Textures_UnrealEngine4/guitar_01_BaseColor.png" // texture file
);

Model tableModel(
        glm::vec3(-2.5f, 1.0f, 0.0f), // position
        glm::vec3(1.0f, 1.0f, 1.0f), // scale
        270.0f, // rotation angle
        glm::vec3(1.0f, 0.0f, 0.0f), // rotation axis
        "./new_models/Wolf-Blender-2.82a.obj", // mesh file
        "./new_models/table_basecolor.png" // texture file
);

Model ukuleleModel (
        glm::vec3(-2.5f, 1.0f, 0.0f), // position
        glm::vec3(1.0f, 1.0f, 1.0f), // scale
        270.0f, // rotation angle
        glm::vec3(1.0f, 0.0f, 0.0f), // rotation axis
        "./blenderObj/ukulele.obj", // mesh file
        "./blenderObj/ukulele.jpg" // texture file
);

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    Model models[4] = {
        floorModel,
        guitarModel,
        tableModel,
        ukuleleModel
    };
};

#endif // SCENE_H