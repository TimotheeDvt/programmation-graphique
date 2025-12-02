#ifndef CAMERA_H
#define CAMERA_H
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "Cube.h"

class Camera {
    public:
        glm::mat4 getViewMatrix()const;

        virtual void setPosition(const glm::vec3& position) {}
        virtual void rotate(float yaw, float pitch) {} // in degrees
        virtual void move(const glm::vec3& offsetPos) {}

        const glm::vec3& getLook() const;
        const glm::vec3& getRight() const;
        const glm::vec3& getUp() const;
        const glm::vec3& getPosition() const;

        float getFOV() const { return mFOV; }
        void setFOV(float fov) { mFOV = fov; } // in degrees

    protected:
        Camera();

        virtual void updateCameraVectors() {}

        glm::vec3 mPosition;
        glm::vec3 mTargetPos;
        glm::vec3 mUp;
        glm::vec3 mLook;
        glm::vec3 mRight;
        const glm::vec3 WORLD_UP;

        // Eulers angles (in radians)
        float mYaw;
        float mPitch;

        float mFOV; // degrees
};


class FPSCamera : public Camera {
    public:
        FPSCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), float yaw = 0.0f, float pitch = 0.0f); // Note: yaw = 0.0f is a placeholder, the base class default is used.

        virtual void setPosition(const glm::vec3& position);
        virtual void rotate(float yaw, float pitch); // in degrees
        virtual void move(const glm::vec3& offsetPos);
        void FPSCamera::jump();
        void FPSCamera::applyPhysics(World& world, double elapsedTime);

        glm::vec3 mVelocity;
        bool mIsOnGround;
        glm::vec3 mPlayerSize = glm::vec3(0.5f, 2.0f, 0.5f); // Width, Height, Depth


    private:
        void updateCameraVectors();
};

class OrbitCamera : public Camera {
    public:
        OrbitCamera();

        virtual void rotate(float yaw, float pitch); // in degrees

        void setLookAt(const glm::vec3& target);
        void setRadius(float radius);

    private:
        void updateCameraVectors();
        float mRadius;
};

#endif