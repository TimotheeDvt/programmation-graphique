#pragma once

#include <glm/glm.hpp>
#include <string>
#include <map>
#include <vector>

enum class BlockType {
	AIR,
	GRASS,
	DIRT,
	STONE,
	REDSTONE,
	WOOD,
	LEAVES,
	TORCH,
	GLASS,
	HUD,
	HUD_SELECTED,
};

struct BlockTexturePaths {
    std::string top;
    std::string bottom;
    std::string side;
    std::string special;
};

struct CubeVertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 texCoords;
};

struct RaycastHit {
    bool hit = false;
    bool isModel = false;
    glm::vec3 blockPos;
    glm::vec3 hitPos;
    glm::vec3 normal;
};