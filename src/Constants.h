#pragma once

constexpr int MAX_BLOCK_TEXTURES = 16;
constexpr int MAX_POINT_LIGHTS = 32;
constexpr int MAX_SPOT_LIGHTS = 8;

constexpr unsigned int DIR_SHADOW_WIDTH = 2048;
constexpr unsigned int DIR_SHADOW_HEIGHT = 2048;

constexpr unsigned int POINT_SHADOW_WIDTH = 1024;
constexpr unsigned int POINT_SHADOW_HEIGHT = 1024;
constexpr float POINT_NEAR_PLANE = 0.1f;
constexpr float POINT_FAR_PLANE = 20.0f;

constexpr unsigned int SPOT_SHADOW_WIDTH = 1024;
constexpr unsigned int SPOT_SHADOW_HEIGHT = 1024;
constexpr float SPOT_NEAR_PLANE = 0.1f;
constexpr float SPOT_FAR_PLANE = 30.0f;