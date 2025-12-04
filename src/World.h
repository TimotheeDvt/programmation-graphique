#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Block.h"

class Chunk; // Forward declaration

class World {
public:
	World();
	~World();

	void generate(int renderDistance = 3, long long seed = -1);
	void draw() const;

	const std::vector<Chunk*>& getChunks() const { return mChunks; }

	std::vector<glm::vec3> getRedstoneLightPositions() const;
	std::vector<glm::vec3> getTorchLightPositions() const;

    bool setBlockAt(const glm::vec3& worldPos, BlockType type);
    BlockType getBlockAt(const glm::vec3& worldPos) const;

	BlockType getBlock(int x, int y, int z) const;
	void localToChunkCoords(int worldX, int worldY, int worldZ,
                        int& chunkX, int& chunkZ,
                        int& localX, int& localY, int& localZ
	) const;

private:
	long long m_seed;
	std::vector<Chunk*> mChunks;
	Chunk* findChunk(int chunkX, int chunkZ) const;
};