#pragma once

#include <memory>
#include <vector>
#include <map>
#include <string>
#include <glm/glm.hpp>

#include "ShaderProgram.h"
#include "Camera.h"
#include "Cube.h"
#include "Scene.h"
#include "Mesh.h"
#include "Texture2D.h"
#include "Light.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    void init();
    void render(const FPSCamera& camera, const World& world, const Scene& scene,
                const std::map<std::string, std::unique_ptr<Mesh>>& meshCache,
                const std::map<std::string, std::unique_ptr<Texture2D>>& modelTextureCache,
                const Texture2D* blockTextures,
                int windowWidth, int windowHeight);

    void drawCrosshair(int windowWidth, int windowHeight);

private:
    void initShaders();
    void initShadows();
    void initCrosshair();

    void renderScene(ShaderProgram& shader, const World& world, const Scene& scene,
                     const std::map<std::string, std::unique_ptr<Mesh>>& meshCache);

    void dirShadowPass(const FPSCamera& camera, const World& world, const Scene& scene,
                       const std::map<std::string, std::unique_ptr<Mesh>>& meshCache);
    void pointShadowPass(const std::vector<PointLight>& pointLights, const World& world, const Scene& scene,
                         const std::map<std::string, std::unique_ptr<Mesh>>& meshCache);
    void spotShadowPass(const std::vector<SpotLight>& spotLights, const World& world, const Scene& scene,
                        const std::map<std::string, std::unique_ptr<Mesh>>& meshCache);
    void mainRenderPass(const FPSCamera& camera, const World& world, const Scene& scene,
                        const std::map<std::string, std::unique_ptr<Mesh>>& meshCache,
                        const std::map<std::string, std::unique_ptr<Texture2D>>& modelTextureCache,
                        const Texture2D* blockTextures,
                        const std::vector<PointLight>& pointLights, const std::vector<SpotLight>& spotLights,
                        int windowWidth, int windowHeight);

    // Shaders
    std::unique_ptr<ShaderProgram> m_minecraftShader;
    std::unique_ptr<ShaderProgram> m_depthShader;
    std::unique_ptr<ShaderProgram> m_pointDepthShader;
    std::unique_ptr<ShaderProgram> m_crosshairShader;

    // Shadow Maps
    GLuint m_dirShadowMapFBO = 0, m_dirShadowMap = 0;
    GLuint m_pointShadowMapFBO = 0, m_pointShadowMap = 0;
    std::vector<GLuint> m_spotShadowMapFBOs, m_spotShadowMaps;

    // Shadow matrices
    glm::mat4 m_dirLightSpaceMatrix;
    std::vector<glm::mat4> m_spotLightSpaceMatrices;

    // Crosshair
    GLuint m_crosshairVAO = 0, m_crosshairVBO = 0;

    // Directional Light
    DirectionalLight m_dirLight;
};