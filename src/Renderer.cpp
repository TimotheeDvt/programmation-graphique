#include "Renderer.h"
#include "Constants.h"
#include "Chunk.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>

Renderer::Renderer() {
    m_dirLight = {
        glm::vec3(0.0f, -1.0f, 0.1f), // Initial sun position (midday)
        glm::vec3(0.3f, 0.3f, 0.35f) * 0.2f,
        glm::vec3(0.8f, 0.8f, 0.7f) * 0.5f,
        glm::vec3(0.3f, 0.3f, 0.3f) * 0.5f
    };

    // Initialize m_whiteTexture for solid color GUI elements
    unsigned char whitePixel[] = { 255, 255, 255, 255 }; // RGBA
    m_whiteTexture = std::make_unique<Texture2D>();
    m_whiteTexture->loadFromMemory(1, 1, whitePixel);
}

Renderer::~Renderer() {
    glDeleteFramebuffers(1, &m_dirShadowMapFBO);
    glDeleteTextures(1, &m_dirShadowMap);
    glDeleteFramebuffers(1, &m_pointShadowMapFBO);
    glDeleteTextures(1, &m_pointShadowMap);
    if (!m_spotShadowMapFBOs.empty()) {
        glDeleteFramebuffers(m_spotShadowMapFBOs.size(), m_spotShadowMapFBOs.data());
        glDeleteTextures(m_spotShadowMaps.size(), m_spotShadowMaps.data());
    }

    if (m_crosshairVAO) glDeleteVertexArrays(1, &m_crosshairVAO);
    if (m_crosshairVBO) glDeleteBuffers(1, &m_crosshairVBO);
    if (m_guiVAO) glDeleteVertexArrays(1, &m_guiVAO);
    if (m_guiVBO) glDeleteBuffers(1, &m_guiVBO);
}

void Renderer::init() {
    initShaders();
    initShadows();
    initCrosshair();
    initGUIMesh();
}

void Renderer::initShaders() {
    m_minecraftShader = std::make_unique<ShaderProgram>();
    m_minecraftShader->loadShaders("./minecraft.vert", "./minecraft.frag");

    m_depthShader = std::make_unique<ShaderProgram>();
    m_depthShader->loadShaders("./shadow_dir.vert", "./shadow_dir.frag");

    m_pointDepthShader = std::make_unique<ShaderProgram>();
    m_pointDepthShader->loadShaders("./shadow_dir.vert", "./depth_point.frag");

    m_crosshairShader = std::make_unique<ShaderProgram>();
    m_crosshairShader->loadShaders("./crosshair.vert", "./crosshair.frag");

    m_guiShader = std::make_unique<ShaderProgram>();
    m_guiShader->loadShaders("./gui.vert", "./gui.frag");
}

void Renderer::initShadows() {
    // Directional Shadow Map
    glGenFramebuffers(1, &m_dirShadowMapFBO);
    glGenTextures(1, &m_dirShadowMap);
    glBindTexture(GL_TEXTURE_2D, m_dirShadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, DIR_SHADOW_WIDTH, DIR_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, m_dirShadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_dirShadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Dir Shadow Framebuffer not complete!");
    }

    // Point Light Shadow Map (Cube Map)
    glGenFramebuffers(1, &m_pointShadowMapFBO);
    glGenTextures(1, &m_pointShadowMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointShadowMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pointShadowMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_pointShadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Point Shadow Framebuffer not complete!");
    }

    // Spot Light Shadow Maps
    m_spotShadowMapFBOs.resize(MAX_SPOT_LIGHTS);
    m_spotShadowMaps.resize(MAX_SPOT_LIGHTS);
    glGenFramebuffers(MAX_SPOT_LIGHTS, m_spotShadowMapFBOs.data());
    glGenTextures(MAX_SPOT_LIGHTS, m_spotShadowMaps.data());
    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i) {
        glBindTexture(GL_TEXTURE_2D, m_spotShadowMaps[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SPOT_SHADOW_WIDTH, SPOT_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::initCrosshair() {
    glGenVertexArrays(1, &m_crosshairVAO);
    glGenBuffers(1, &m_crosshairVBO);
    glBindVertexArray(m_crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 24, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Renderer::render(const FPSCamera& camera, const World& world, const Scene& scene,
                      const std::map<std::string, std::unique_ptr<Mesh>>& meshCache,
                      const std::map<std::string, std::unique_ptr<Texture2D>>& modelTextureCache,
                      const Texture2D* blockTextures,
                      int windowWidth, int windowHeight) {

    // 1. Collect all lights for the frame
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;

    for (const auto& pos : world.getRedstoneLightPositions()) {
        if (pointLights.size() >= MAX_POINT_LIGHTS) break;
        pointLights.push_back(CreateRedstoneLight(pos));
    }
    for (const auto& pos : world.getTorchLightPositions()) {
        if (pointLights.size() >= MAX_POINT_LIGHTS) break;
        pointLights.push_back(CreateTorchLight(pos));
    }
    for (const auto& modelData : scene.models) { // No change needed here, range-based for loop works on both
        if (spotLights.size() >= MAX_SPOT_LIGHTS) break;

        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(modelData.rotation.angle), modelData.rotation.axis);
        glm::vec3 lookDirection = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

        glm::vec3 eyeOffset1 = glm::vec3(rotationMatrix * glm::vec4(0.1f, 2.75f, 0.4f, 1.0f));
        glm::vec3 eyeLightPos1 = modelData.position + eyeOffset1;
        spotLights.push_back(CreateEndermanLight(eyeLightPos1, lookDirection));
        if (spotLights.size() >= MAX_SPOT_LIGHTS) break;
        glm::vec3 eyeOffset2 = glm::vec3(rotationMatrix * glm::vec4(-0.1f, 2.75f, 0.45f, 1.0f));
        glm::vec3 eyeLightPos2 = modelData.position + eyeOffset2;
        spotLights.push_back(CreateEndermanLight(eyeLightPos2, lookDirection));
    }

    // 2. Render Shadow Maps
    dirShadowPass(camera, world, scene, meshCache, blockTextures);
    pointShadowPass(pointLights, world, scene, meshCache);
    spotShadowPass(spotLights, world, scene, meshCache);

    // 3. Main Render Pass
    mainRenderPass(camera, world, scene, meshCache, modelTextureCache, blockTextures, pointLights, spotLights, windowWidth, windowHeight);
}

void Renderer::updateSun(float deltaTime) {
    // Vitesse de rotation du soleil (complète un cycle en 120 secondes)
    const float cycleDuration = 60.0f;
    const float rotationSpeed = 2.0f * glm::pi<float>() / cycleDuration;

    m_sunAngle += rotationSpeed * deltaTime;
    if (m_sunAngle > 2.0f * glm::pi<float>()) {
        m_sunAngle -= 2.0f * glm::pi<float>();
    }

    // Rotation autour de l'axe X (lever/coucher sur l'axe Z)
    m_dirLight.direction = glm::vec3(0.0f, -sin(m_sunAngle), -cos(m_sunAngle));
    m_dirLight.direction = glm::normalize(m_dirLight.direction);
}

void Renderer::renderScene(ShaderProgram& shader, const World& world, const Scene& scene,
                           const std::map<std::string, std::unique_ptr<Mesh>>& meshCache) {
    glm::mat4 model(1.0f);
    shader.setUniform("model", model);
    world.draw();

    for (const auto& modelData : scene.models) { // No change needed here
        if (meshCache.count(modelData.meshFile)) {
            Mesh* mesh = meshCache.at(modelData.meshFile).get();
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelData.position);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelData.rotation.angle), modelData.rotation.axis);
            modelMatrix = glm::scale(modelMatrix, modelData.scale);
            shader.setUniform("model", modelMatrix);
            mesh->draw();
        }
    }
}

void Renderer::dirShadowPass(const FPSCamera& camera, const World& world, const Scene& scene,
                             const std::map<std::string, std::unique_ptr<Mesh>>& meshCache, const Texture2D* blockTextures) {
float dir_near_plane = 1.0f, dir_far_plane = 70.0f;
    // Orthographic bounds are [-40, 40], so total frustum size is 80 units.
    glm::mat4 dirLightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, dir_near_plane, dir_far_plane);
    glm::vec3 centerPos = camera.getPosition();

    // 1. Initial Light View matrix
    glm::vec3 lightTarget = centerPos;
    glm::vec3 lightPos = lightTarget - m_dirLight.direction * 20.0f;
    glm::mat4 dirLightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    // 2. Snapping Logic for Voxel Alignment (Fixes diagonal/misaligned shadows)
    glm::mat4 lightSpace = dirLightProjection * dirLightView;
    glm::vec4 shadowOrigin = lightSpace * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    // Scale factor: Frustum size (80.0f) / Resolution (2048) -> inverted for texel size
    float worldToTexel = (float)DIR_SHADOW_WIDTH / 80.0f;
    shadowOrigin *= worldToTexel;

    // Snap the light-view coordinates to the nearest pixel center (floor)
    shadowOrigin.x = floor(shadowOrigin.x);
    shadowOrigin.y = floor(shadowOrigin.y);

    // Transform back to world space
    glm::mat4 inverseLightSpace = glm::inverse(lightSpace);
    glm::vec4 snappedOrigin = inverseLightSpace * (shadowOrigin / worldToTexel);

    // Calculate the translation correction and apply it to the light's position/target
    glm::vec3 translation = glm::vec3(snappedOrigin) - lightTarget;
    lightTarget += translation;
    lightPos += translation;

    // 3. Recalculate Light View and final Light Space Matrix with snapped position
    dirLightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    m_dirLightSpaceMatrix = dirLightProjection * dirLightView;

    glViewport(0, 0, DIR_SHADOW_WIDTH, DIR_SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, m_dirShadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    // glCullFace(GL_FRONT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(4.0f, 100.0f);

    m_depthShader->use();
    m_depthShader->setUniform("lightSpaceMatrix", m_dirLightSpaceMatrix);

    const auto& pathToIndex = Chunk::m_pathToTextureIndex;
    for (size_t i = 0; i < pathToIndex.size(); i++) {
        blockTextures[i].bind(i);
        m_depthShader->setUniformSampler(("diffuseMaps[" + std::to_string(i) + "]").c_str(), i);
    }

    renderScene(*m_depthShader, world, scene, meshCache);

    for (size_t i = 0; i < pathToIndex.size(); i++) {
        blockTextures[i].unbind(i);
    }

    // glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::pointShadowPass(const std::vector<PointLight>& pointLights, const World& world, const Scene& scene,
                               const std::map<std::string, std::unique_ptr<Mesh>>& meshCache) {
    if (pointLights.empty()) return;

    glm::mat4 pointShadowProj = glm::perspective(glm::radians(90.0f), (float)POINT_SHADOW_WIDTH / (float)POINT_SHADOW_HEIGHT, POINT_NEAR_PLANE, POINT_FAR_PLANE);

    glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pointShadowMapFBO);

    m_pointDepthShader->use();
    m_pointDepthShader->setUniform("farPlane", POINT_FAR_PLANE);

    // This demo renders shadows for only the first point light for performance.
    // To render for all, you would need multiple cube map textures.
    const auto& lightPos = pointLights[0].position;
    m_pointDepthShader->setUniform("lightPos", lightPos);

    std::vector<glm::mat4> pointShadowTransforms;
    pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
    pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
    pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
    pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
    pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
    pointShadowTransforms.push_back(pointShadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

    for (int j = 0; j < 6; ++j) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, m_pointShadowMap, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        m_pointDepthShader->setUniform("lightSpaceMatrix", pointShadowTransforms[j]);
        renderScene(*m_pointDepthShader, world, scene, meshCache);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::spotShadowPass(const std::vector<SpotLight>& spotLights, const World& world, const Scene& scene,
                              const std::map<std::string, std::unique_ptr<Mesh>>& meshCache) {
    if (spotLights.empty()) return;

    glViewport(0, 0, SPOT_SHADOW_WIDTH, SPOT_SHADOW_HEIGHT);
    m_depthShader->use();

    m_spotLightSpaceMatrices.resize(spotLights.size());

    for (size_t i = 0; i < spotLights.size(); ++i) {
        const auto& light = spotLights[i];
        float fov = glm::acos(light.cosOuterCone) * 2.0f;

        glm::mat4 spotProjection = glm::perspective(fov, 1.0f, SPOT_NEAR_PLANE, SPOT_FAR_PLANE);
        glm::mat4 spotView = glm::lookAt(light.position, light.position + light.direction, glm::vec3(0.0f, 1.0f, 0.0f));
        m_spotLightSpaceMatrices[i] = spotProjection * spotView;

        glBindFramebuffer(GL_FRAMEBUFFER, m_spotShadowMapFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_spotShadowMaps[i], 0);
        glClear(GL_DEPTH_BUFFER_BIT);

        m_depthShader->setUniform("lightSpaceMatrix", m_spotLightSpaceMatrices[i]);
        renderScene(*m_depthShader, world, scene, meshCache);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::mainRenderPass(const FPSCamera& camera, const World& world, const Scene& scene,
                              const std::map<std::string, std::unique_ptr<Mesh>>& meshCache,
                              const std::map<std::string, std::unique_ptr<Texture2D>>& modelTextureCache,
                              const Texture2D* blockTextures,
                              const std::vector<PointLight>& pointLights, const std::vector<SpotLight>& spotLights,
                              int windowWidth, int windowHeight) {

    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_minecraftShader->use();

    // Camera and projection
    glm::mat4 view = camera.getViewMatrix();
    float aspectRatio = (float)windowWidth / (float)windowHeight;
    glm::mat4 projection = glm::perspective(glm::radians(camera.getFOV()), aspectRatio, 0.1f, 200.0f);
    m_minecraftShader->setUniform("view", view);
    m_minecraftShader->setUniform("projection", projection);
    m_minecraftShader->setUniform("viewPos", camera.getPosition());

    // Bind shadow maps
    int textureUnit = MAX_BLOCK_TEXTURES;
    m_minecraftShader->setUniform("lightSpaceMatrix", m_dirLightSpaceMatrix);
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_dirShadowMap);
    m_minecraftShader->setUniformSampler("dirShadowMap", textureUnit++);

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointShadowMap);
    m_minecraftShader->setUniformSampler("pointShadowMap", textureUnit++);
    m_minecraftShader->setUniform("pointFarPlane", POINT_FAR_PLANE);

    for (size_t i = 0; i < spotLights.size(); ++i) {
        m_minecraftShader->setUniform(("spotLightSpaceMatrices[" + std::to_string(i) + "]").c_str(), m_spotLightSpaceMatrices[i]);
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, m_spotShadowMaps[i]);
        m_minecraftShader->setUniformSampler(("spotShadowMaps[" + std::to_string(i) + "]").c_str(), textureUnit++);
    }

    // Set light uniforms
    m_minecraftShader->setUniform("dirLight.direction", m_dirLight.direction);
    m_minecraftShader->setUniform("dirLight.ambient", m_dirLight.ambient);
    m_minecraftShader->setUniform("dirLight.diffuse", m_dirLight.diffuse);
    m_minecraftShader->setUniform("dirLight.specular", m_dirLight.specular);

    m_minecraftShader->setUniform("numPointLights", (int)pointLights.size());
    for (size_t i = 0; i < pointLights.size(); i++) {
        std::string base = "pointLights[" + std::to_string(i) + "]";
        m_minecraftShader->setUniform((base + ".position").c_str(), pointLights[i].position);
        m_minecraftShader->setUniform((base + ".ambient").c_str(), pointLights[i].ambient);
        m_minecraftShader->setUniform((base + ".diffuse").c_str(), pointLights[i].diffuse);
        m_minecraftShader->setUniform((base + ".specular").c_str(), pointLights[i].specular);
        m_minecraftShader->setUniform((base + ".constant").c_str(), pointLights[i].constant);
        m_minecraftShader->setUniform((base + ".linear").c_str(), pointLights[i].linear);
        m_minecraftShader->setUniform((base + ".exponant").c_str(), pointLights[i].exponant);
    }

    m_minecraftShader->setUniform("numSpotLights", (int)spotLights.size());
    for (size_t i = 0; i < spotLights.size(); i++) {
        std::string base = "spotLights[" + std::to_string(i) + "]";
        m_minecraftShader->setUniform((base + ".position").c_str(), spotLights[i].position);
        m_minecraftShader->setUniform((base + ".direction").c_str(), spotLights[i].direction);
        m_minecraftShader->setUniform((base + ".ambient").c_str(), spotLights[i].ambient);
        m_minecraftShader->setUniform((base + ".diffuse").c_str(), spotLights[i].diffuse);
        m_minecraftShader->setUniform((base + ".specular").c_str(), spotLights[i].specular);
        m_minecraftShader->setUniform((base + ".constant").c_str(), spotLights[i].constant);
        m_minecraftShader->setUniform((base + ".linear").c_str(), spotLights[i].linear);
        m_minecraftShader->setUniform((base + ".exponant").c_str(), spotLights[i].exponant);
        m_minecraftShader->setUniform((base + ".cosInnerCone").c_str(), spotLights[i].cosInnerCone);
        m_minecraftShader->setUniform((base + ".cosOuterCone").c_str(), spotLights[i].cosOuterCone);
    }

    // Material
    m_minecraftShader->setUniform("material.ambient", glm::vec3(1.0f));
    m_minecraftShader->setUniform("material.specular", glm::vec3(0.1f));
    m_minecraftShader->setUniform("material.shininess", 8.0f);

    // Bind block textures
    const auto& pathToIndex = Chunk::m_pathToTextureIndex;
    for (size_t i = 0; i < pathToIndex.size(); i++) {
        blockTextures[i].bind(i);
        m_minecraftShader->setUniformSampler(("material.diffuseMaps[" + std::to_string(i) + "]").c_str(), i);
    }

    // Draw world
    m_minecraftShader->setUniform("model", glm::mat4(1.0f));
    world.draw();

    // Draw models
    for (const auto& modelData : scene.models) { // No change needed here
        if (meshCache.count(modelData.meshFile)) {
            Mesh* mesh = meshCache.at(modelData.meshFile).get();
            Texture2D* texture = modelTextureCache.count(modelData.textureFile) ? modelTextureCache.at(modelData.textureFile).get() : nullptr;

            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelData.position);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelData.rotation.angle), modelData.rotation.axis);
            modelMatrix = glm::scale(modelMatrix, modelData.scale);
            m_minecraftShader->setUniform("model", modelMatrix);

            if (texture) {
                texture->bind(0);
                m_minecraftShader->setUniformSampler("material.diffuseMap", 0); // For single-textured models
            }

            mesh->draw();

            if (texture) {
                texture->unbind(0);
            }
        }
    }

    // Unbind all textures
    for (size_t i = 0; i < pathToIndex.size(); i++) {
        blockTextures[i].unbind(i);
    }
    for (int i = 0; i < textureUnit - MAX_BLOCK_TEXTURES; ++i) {
        glActiveTexture(GL_TEXTURE0 + MAX_BLOCK_TEXTURES + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
}

void Renderer::drawCrosshair(int windowWidth, int windowHeight) {
    float centerX = windowWidth / 2.0f;
    float centerY = windowHeight / 2.0f;
    float size = 10.0f;
    float gap = 3.0f;

    GLfloat verts[] = {
        centerX - size - gap, centerY, 0.0f,
        centerX - gap, centerY, 0.0f,
        centerX + gap, centerY, 0.0f,
        centerX + size + gap, centerY, 0.0f,
        centerX, centerY + gap, 0.0f,
        centerX, centerY + size + gap, 0.0f,
        centerX, centerY - gap, 0.0f,
        centerX, centerY - size - gap, 0.0f
    };

    glBindVertexArray(m_crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_crosshairVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glm::mat4 ortho = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);

    m_crosshairShader->use();
    m_crosshairShader->setUniform("projection", ortho);

    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);
    glBindVertexArray(m_crosshairVAO);
    glDrawArrays(GL_LINES, 0, 8);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::initGUIMesh() {
    float quadVertices[] = {
        // pos (x, y, z=0) // tex (u, v)
        // Triangle 1
        0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        // Triangle 2
        0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,   1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_guiVAO);
    glGenBuffers(1, &m_guiVBO);
    glBindVertexArray(m_guiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_guiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::drawInventoryHUD(const Texture2D* blockTextures, int numTextures, int selectedIndex,
                               const std::vector<BlockType>& selectableBlocks,
                               int windowWidth, int windowHeight) {
    if (m_guiVAO == 0 || selectableBlocks.empty()) return;

    m_guiShader->use();

    m_guiShader->setUniform("is3D", 0);
    m_guiShader->setUniform("view", glm::mat4(1.0f));

    glm::mat4 ortho = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    m_guiShader->setUniform("projection", ortho);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    float iconSize = 64.0f;
    float padding = 10.0f;
    float barWidth = selectableBlocks.size() * iconSize + (selectableBlocks.size() + 1) * padding;
    float startX = (windowWidth - barWidth) / 2.0f;
    float startY = padding;

    float frameSize = iconSize + 10.0f;
    float frameOffset = (iconSize - frameSize) / 2.0f;

    int safeTextureIndex = 10;

    glBindVertexArray(m_guiVAO);

    const auto& pathToIndex = Chunk::m_pathToTextureIndex;

    for (size_t i = 0; i < selectableBlocks.size(); ++i) {
        BlockType type = selectableBlocks[i];

        float currentX = startX + padding + i * (iconSize + padding);
        float currentY = startY;

        if ((int)i == selectedIndex) {
            glm::mat4 frameModel = glm::mat4(1.0f);
            frameModel = glm::translate(frameModel, glm::vec3(currentX + frameOffset, currentY + frameOffset, 0.0f));
            frameModel = glm::scale(frameModel, glm::vec3(frameSize, frameSize, 1.0f));
            m_guiShader->setUniform("model", frameModel);

            blockTextures[safeTextureIndex+1].bind(0);
            m_guiShader->setUniformSampler("guiTexture", 0);
            m_guiShader->setUniform("tintColor", glm::vec3(1.0f, 1.0f, 1.0f)); // Cadre purement blanc
            glDrawArrays(GL_TRIANGLES, 0, 6);
            blockTextures[safeTextureIndex+1].unbind(0);
        } else {
            glm::mat4 frameModel = glm::mat4(1.0f);
            frameModel = glm::translate(frameModel, glm::vec3(currentX + frameOffset, currentY + frameOffset, 0.0f));
            frameModel = glm::scale(frameModel, glm::vec3(frameSize, frameSize, 1.0f));
            m_guiShader->setUniform("model", frameModel);

            blockTextures[safeTextureIndex].bind(0);
            m_guiShader->setUniformSampler("guiTexture", 0);
            m_guiShader->setUniform("tintColor", glm::vec3(1.0f, 1.0f, 1.0f)); // Cadre purement blanc
            glDrawArrays(GL_TRIANGLES, 0, 6);
            blockTextures[safeTextureIndex].unbind(0);
        }

        std::string texturePath = "";
        if (Chunk::m_textureConfig.count(type)) {
            const auto& config = Chunk::m_textureConfig.at(type);
            texturePath = config.special.empty() ? config.top : config.special;
        }

        if (texturePath.empty()) continue;

        int textureIndex = -1;
        if (pathToIndex.count(texturePath)) {
            textureIndex = pathToIndex.at(texturePath);
        }

        if (textureIndex == -1 || textureIndex >= numTextures) continue;

        glm::mat4 iconModel = glm::mat4(1.0f);
        iconModel = glm::translate(iconModel, glm::vec3(currentX, currentY, 0.0f));
        iconModel = glm::scale(iconModel, glm::vec3(iconSize, iconSize, 1.0f));
        m_guiShader->setUniform("model", iconModel);

        blockTextures[textureIndex].bind(0);
        m_guiShader->setUniformSampler("guiTexture", 0);

        m_guiShader->setUniform("tintColor", glm::vec3(1.0f));

        glDrawArrays(GL_TRIANGLES, 0, 6);

        blockTextures[textureIndex].unbind(0);
    }

    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

void Renderer::drawSunGizmo(const FPSCamera& camera, int windowWidth, int windowHeight, bool debug) {
    if (m_guiVAO == 0) return;

    // Constante pour la taille désirée du gizmo à l'écran (en pixels)
    const float GIZMO_PIXEL_SIZE = 60.0f;

    // 1. Calculer la position 3D du soleil (éloignée de la caméra dans la direction opposée à la lumière)
    glm::vec3 sunWorldPos = camera.getPosition() - m_dirLight.direction * 100.0f;

    m_guiShader->use();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 model = glm::mat4(1.0f);

    if (debug) {
        // --- MODE DEBUG (2D, par-dessus tout) ---
        // Le gizmo est rendu en 2D avec une taille fixe en pixels.

        glm::mat4 view = camera.getViewMatrix();
        float aspectRatio = (float)windowWidth / (float)windowHeight;
        glm::mat4 projection = glm::perspective(glm::radians(camera.getFOV()), aspectRatio, 0.1f, 200.0f);
        glm::vec4 sunClipSpace = projection * view * glm::vec4(sunWorldPos, 1.0f);

        // Vérifier si le soleil est derrière
        if (sunClipSpace.w < 0.0f) {
            glDisable(GL_BLEND);
            return;
        }

        // Projection en coordonnées écran
        glm::vec3 sunNDC = glm::vec3(sunClipSpace) / sunClipSpace.w;
        float screenX = (sunNDC.x + 1.0f) / 2.0f * windowWidth;
        float screenY = (sunNDC.y + 1.0f) / 2.0f * windowHeight;

        glm::mat4 ortho = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);

        m_guiShader->setUniform("is3D", 0);
        m_guiShader->setUniform("view", glm::mat4(1.0f));
        m_guiShader->setUniform("projection", ortho);

        glDisable(GL_DEPTH_TEST);

        // Positionnement et mise à l'échelle en 2D (taille fixe GIZMO_PIXEL_SIZE)
        model = glm::translate(model, glm::vec3(screenX - GIZMO_PIXEL_SIZE / 2.0f, screenY - GIZMO_PIXEL_SIZE / 2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(GIZMO_PIXEL_SIZE, GIZMO_PIXEL_SIZE, 1.0f));

    } else {
        // --- MODE NORMAL (3D, avec occlusion) ---
        // Le gizmo est rendu comme un billboard 3D soumis au test de profondeur.

        glm::mat4 view = camera.getViewMatrix();
        float aspectRatio = (float)windowWidth / (float)windowHeight;
        glm::mat4 projection = glm::perspective(glm::radians(camera.getFOV()), aspectRatio, 0.1f, 200.0f);

        m_guiShader->setUniform("is3D", 1);
        m_guiShader->setUniform("view", view);
        m_guiShader->setUniform("projection", projection);

        glEnable(GL_DEPTH_TEST);

        // 3. Modèle 3D: Position + Billboard + Redimensionnement

        // a) Positionnement au point du soleil
        model = glm::translate(model, sunWorldPos);

        // b) Orientation Billboard (annule la rotation de la vue pour toujours faire face à la caméra)
        model = model * glm::mat4(glm::mat3(view));

        // c) Mise à l'échelle pour une taille constante (en pixels)
        float distance = glm::length(sunWorldPos - camera.getPosition());
        float fovRad = glm::radians(camera.getFOV());

        // Calcul du facteur d'échelle 3D pour obtenir la taille GIZMO_PIXEL_SIZE à l'écran.
        // WorldScale = 2 * distance * tan(FOV_rad/2) * (Target_Pixels / Window_Height)
        float frustumHalfHeight = distance * glm::tan(fovRad / 2.0f);
        float frustumWorldHeight = frustumHalfHeight * 2.0f;
        float scaleFactor = frustumWorldHeight * (GIZMO_PIXEL_SIZE / (float)windowHeight);

        // Taille minimale pour éviter qu'il ne devienne invisible
        scaleFactor = glm::max(scaleFactor, 0.5f);

        model = glm::scale(model, glm::vec3(scaleFactor));
    }

    m_guiShader->setUniform("model", model);
    m_guiShader->setUniform("tintColor", glm::vec3(1.0f, 1.0f, 0.0f));
    m_whiteTexture->bind(0);
    m_guiShader->setUniformSampler("guiTexture", 0);

    glBindVertexArray(m_guiVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Nettoyage
    m_guiShader->setUniform("is3D", 0);
    m_whiteTexture->unbind(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}