#pragma once
#include <glm/glm.hpp>

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float exponant;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float exponant;
    float cosInnerCone;
    float cosOuterCone;
};

inline PointLight CreateRedstoneLight(const glm::vec3& position) {
    return { position, glm::vec3(0.5f), glm::vec3(1.0f, 0.1f, 0.1f), glm::vec3(0.5f, 0.1f, 0.1f), 1.0f, 0.14f, 0.07f };
}

inline PointLight CreateTorchLight(const glm::vec3& position) {
    return { position, glm::vec3(0.0f), glm::vec3(1.0f, 0.8f, 0.2f), glm::vec3(0.5f, 0.4f, 0.1f), 1.0f, 0.22f, 0.20f };
}

inline SpotLight CreateEndermanLight(const glm::vec3& position, const glm::vec3& direction) {
    return {
        position, direction,
        glm::vec3(0.0f), glm::vec3(0.8f, 0.0f, 0.8f) * 8.0f, glm::vec3(0.6f, 0.0f, 0.6f) * 8.0f,
        1.0f, 0.09f, 0.032f,
        glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(25.0f))
    };
}