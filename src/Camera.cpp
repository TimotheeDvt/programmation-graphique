#include "Camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/normalize_dot.hpp"
#include "glm/geometric.hpp"

const float DEF_FOV = 45.0f;

Camera::Camera()
    : mPosition(glm::vec3(0.0f, 0.0f, 0.0f)),
    mTargetPos(glm::vec3(0.0f, 0.0f, 0.0f)),
    mUp(glm::vec3(0.0f, 1.0f, 0.0f)),
    mRight(glm::vec3(0.0f, 0.0f, 0.0f)),
    WORLD_UP(glm::vec3(0.0f, 1.0f, 0.0f)),
    mYaw(glm::pi<float>()),
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
    mPosition = position;
    mYaw = yaw;
    mPitch = pitch;
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
    look.x = cosf(mPitch) * sinf(mYaw);
    look.y = sinf(mPitch);
    look.z = cosf(mPitch) * cosf(mYaw);

    // Normalize to ensure it's a unit vector
    mLook = glm::normalize(look);

    // Calculate right and up vectors
    mRight = glm::normalize(glm::cross(mLook, WORLD_UP));
    mUp = glm::normalize(glm::cross(mRight, mLook));

    // Update target position for view matrix
    mTargetPos = mPosition + mLook;
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