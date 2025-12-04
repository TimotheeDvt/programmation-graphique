#include "DebugDrawer.h"
#include <glm/gtc/matrix_transform.hpp>

DebugDrawer::DebugDrawer() {}

DebugDrawer::~DebugDrawer() {
    if (m_lineVAO) glDeleteVertexArrays(1, &m_lineVAO);
    if (m_lineVBO) glDeleteBuffers(1, &m_lineVBO);
    if (m_pointVAO) glDeleteVertexArrays(1, &m_pointVAO);
    if (m_pointVBO) glDeleteBuffers(1, &m_pointVBO);
}

void DebugDrawer::init() {
    m_shader = std::make_unique<ShaderProgram>();
    m_shader->loadShaders("./debug_line.vert", "./debug_line.frag");

    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &m_pointVAO);
    glGenBuffers(1, &m_pointVBO);
    glBindVertexArray(m_pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_pointVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void DebugDrawer::drawRaycast(const FPSCamera& camera, const RaycastHit& hit, int windowWidth, int windowHeight) {
    glm::vec3 origin = camera.getPosition();
    glm::vec3 dir = glm::normalize(camera.getLook());

    glm::vec3 end;
    if (hit.hit) {
        end = hit.hitPos;
    } else {
        end = origin + dir * 8.0f;
    }

    float lineVerts[6] = { origin.x, origin.y, origin.z, end.x, end.y, end.z };

    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVerts), lineVerts);

    m_shader->use();
    m_shader->setUniform("model", glm::mat4(1.0f));
    m_shader->setUniform("view", camera.getViewMatrix());
    float aspectRatio = (float)windowWidth / (float)windowHeight;
    glm::mat4 projection = glm::perspective(glm::radians(camera.getFOV()), aspectRatio, 0.1f, 200.0f);
    m_shader->setUniform("projection", projection);

    glDisable(GL_DEPTH_TEST);

    // Line (red)
    m_shader->setUniform("uColor", glm::vec3(1.0f, 0.0f, 0.0f));
    glBindVertexArray(m_lineVAO);
    glLineWidth(4.0f);
    glDrawArrays(GL_LINES, 0, 2);

    if (hit.hit) {
        // Hit point (green)
        float hitPt[3] = { hit.hitPos.x, hit.hitPos.y, hit.hitPos.z };
        glBindBuffer(GL_ARRAY_BUFFER, m_pointVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(hitPt), hitPt);
        m_shader->setUniform("uColor", glm::vec3(0.0f, 1.0f, 0.0f));
        glBindVertexArray(m_pointVAO);
        glPointSize(12.0f);
        glDrawArrays(GL_POINTS, 0, 1);

        // Voxel center (cyan)
        glm::vec3 voxelCenter = hit.blockPos + glm::vec3(0.5f);
        float voxelPt[3] = { voxelCenter.x, voxelCenter.y, voxelCenter.z };
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(voxelPt), voxelPt);
        m_shader->setUniform("uColor", glm::vec3(0.0f, 1.0f, 1.0f));
        glPointSize(14.0f);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}