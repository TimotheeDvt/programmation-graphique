#include "Camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/normalize_dot.hpp"
#include "glm/geometric.hpp"
#include <iostream>

const float DEF_FOV = 45.0f;

Camera::Camera()
    : mPosition(glm::vec3(0.0f, 0.0f, 0.0f)),
    mTargetPos(glm::vec3(0.0f, 0.0f, 0.0f)),
    mUp(glm::vec3(0.0f, 1.0f, 0.0f)),
    mRight(glm::vec3(0.0f, 0.0f, 0.0f)),
    WORLD_UP(glm::vec3(0.0f, 1.0f, 0.0f)),
    mYaw(1.5f * glm::pi<float>()),
    mPitch(0.0f),
    mFOV(DEF_FOV)
{ }

glm::mat4 Camera::getViewMatrix()const {
    return glm::lookAt(mPosition, mTargetPos, mUp);
}

const glm::vec3& Camera::getLook() const {
    return mLook;
}

const glm::vec3& Camera::getRight() const {
    return mRight;
}

const glm::vec3& Camera::getUp() const {
    return mUp;
}

const glm::vec3& Camera::getPosition() const {
    return mPosition;
}

// FPSCamera
FPSCamera::FPSCamera(glm::vec3 position, float yaw, float pitch) {
    // We inherit mYaw's default value from the base Camera constructor
    mPosition = position;
    if (yaw != 0.0f || pitch != 0.0f) { // Only override if values are provided
        mYaw = yaw;
        mPitch = pitch;
    }
    mVelocity = glm::vec3(0.0f);
    mIsOnGround = false;
    // mPlayerSize est initialisé dans le .h

    updateCameraVectors(); // IMPORTANT: Initialize camera vectors
}

void FPSCamera::setPosition(const glm::vec3& position) {
    mPosition = position;
    updateCameraVectors();
}

void FPSCamera::move(const glm::vec3& offsetPos) {
    mPosition += offsetPos;
    updateCameraVectors();
}

void FPSCamera::rotate(float yaw, float pitch) {
    mYaw += glm::radians(yaw);
    mPitch += glm::radians(pitch);

    // Clamp pitch to prevent gimbal lock
    mPitch = glm::clamp(mPitch, -glm::pi<float>() / 2.0f + 0.1f, glm::pi<float>() / 2.0f - 0.1f);
    updateCameraVectors();
}

void FPSCamera::updateCameraVectors() {
    // Calculate the look direction vector
    glm::vec3 look;
    look.x = cosf(mPitch) * cosf(mYaw);
    look.y = sinf(mPitch);
    look.z = cosf(mPitch) * sinf(mYaw);

    // Normalize to ensure it's a unit vector
    mLook = glm::normalize(look);

    // Calculate right and up vectors
    mRight = glm::normalize(glm::cross(mLook, WORLD_UP));
    mUp = glm::normalize(glm::cross(mRight, mLook));

    // Update target position for view matrix
    mTargetPos = mPosition + mLook;
}

void FPSCamera::jump() {
    if (mIsOnGround) {
        mIsOnGround = false;
        mVelocity.y = 8.0f; // Puissance du saut
    }
}

void FPSCamera::applyPhysics(World& world, double elapsedTime) {
    const auto isBlocked = [&](const glm::vec3& pos) {
        return world.getBlockAt(pos) != BlockType::AIR && world.getBlockAt(pos) != BlockType::TORCH;
    };

    const float GRAVITY = 20.0f;
    const float FRICTION = 8.0f;

    // 1. Appliquer la gravité
    mVelocity.y -= GRAVITY * (float)elapsedTime;

    // 2. Appliquer la friction (ralentissement horizontal)
    mVelocity.x -= mVelocity.x * FRICTION * (float)elapsedTime;
    mVelocity.z -= mVelocity.z * FRICTION * (float)elapsedTime;

    // 3. Calculer la nouvelle position potentielle
    glm::vec3 newPos = mPosition + mVelocity * (float)elapsedTime;

    // 4. Détection et résolution des collisions (AABB)
    glm::vec3 playerHalfSize = mPlayerSize / 2.0f;
    mIsOnGround = false;

    // Itérer sur les 8 coins de la boîte de collision du joueur
    for (int i = 0; i < 8; ++i) {
        glm::vec3 cornerOffset(
            (i & 1) ? playerHalfSize.x : -playerHalfSize.x,
            (i & 2) ? playerHalfSize.y : -playerHalfSize.y,
            (i & 4) ? playerHalfSize.z : -playerHalfSize.z
        );

        // --- Collision sur l'axe Y ---
        glm::vec3 yCollisionPoint = mPosition + cornerOffset;
        yCollisionPoint.y = newPos.y + cornerOffset.y;
        if (isBlocked(yCollisionPoint)) {
            if (mVelocity.y < 0) { // Collision en tombant
                mIsOnGround = true;
            }
            mVelocity.y = 0;
            newPos.y = floor(yCollisionPoint.y) + ( (i & 2) ? 0.0f : 1.0f ) - cornerOffset.y;
        }

        // --- Collision sur l'axe X ---
        glm::vec3 xCollisionPoint = mPosition + cornerOffset;
        xCollisionPoint.x = newPos.x + cornerOffset.x;
        if (isBlocked(xCollisionPoint)) {
            mVelocity.x = 0;
            newPos.x = floor(xCollisionPoint.x) + ( (i & 1) ? 0.0f : 1.0f ) - cornerOffset.x;
            std::cout << "Collision X at: " << xCollisionPoint.x << ", " << xCollisionPoint.y << ", " << xCollisionPoint.z << std::endl;
        }

        // --- Collision sur l'axe Z ---
        glm::vec3 zCollisionPoint = mPosition + cornerOffset;
        zCollisionPoint.z = newPos.z + cornerOffset.z;
        if (isBlocked(zCollisionPoint)) {
            mVelocity.z = 0;
            newPos.z = floor(zCollisionPoint.z) + ( (i & 4) ? 0.0f : 1.0f ) - cornerOffset.z;
            std::cout << "Collision Z at: " << zCollisionPoint.x << ", " << zCollisionPoint.y << ", " << zCollisionPoint.z << std::endl;
        }
    }

    // 5. Mettre à jour la position
    mPosition = newPos;

    // Mettre à jour les vecteurs de la caméra après le déplacement
    updateCameraVectors();
}


// OrbitCamera
OrbitCamera::OrbitCamera()
    : mRadius(10.0f)
{ }

void OrbitCamera::setLookAt(const glm::vec3& target) {
    mTargetPos = target;
}

void OrbitCamera::setRadius(float radius) {
    mRadius = glm::clamp(radius, 2.0f, 80.0f);
}

void OrbitCamera::rotate(float yaw, float pitch) {
    mYaw = glm::radians(yaw);
    mPitch = glm::radians(pitch);

    mPitch = glm::clamp(mPitch, -glm::pi<float>() / 2.0f + 0.1f, glm::pi<float>() / 2.0f - 0.1f);
    updateCameraVectors();
}

void OrbitCamera::updateCameraVectors() {
    mPosition.x = mTargetPos.x + mRadius * cosf(mPitch) * sinf(mYaw);
    mPosition.y = mTargetPos.y + mRadius * sinf(mPitch);
    mPosition.z = mTargetPos.z + mRadius * cosf(mPitch) * cosf(mYaw);
}