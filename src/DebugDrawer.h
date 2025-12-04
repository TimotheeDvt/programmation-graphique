#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "ShaderProgram.h"
#include "Camera.h"
#include "Cube.h"

class DebugDrawer {
public:
    DebugDrawer();
    ~DebugDrawer();

    void init();
    void drawRaycast(const FPSCamera& camera, const RaycastHit& hit, int windowWidth, int windowHeight);

private:
    std::unique_ptr<ShaderProgram> m_shader;
    GLuint m_lineVAO = 0, m_lineVBO = 0;
    GLuint m_pointVAO = 0, m_pointVBO = 0;
};