#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>

#include "Block.h"

class Chunk {
public:
	static const int CHUNK_SIZE = 16;
	static const int CHUNK_HEIGHT = 64;

	static const int CHUNK_VOXEL_COUNT = CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE;

	Chunk(int chunkX, int chunkZ);
	~Chunk();

	void generate(long long worldSeed);
	void buildMesh();
	void draw();

	BlockType getBlock(int x, int y, int z) const;
	void setBlock(int x, int y, int z, BlockType type);

	glm::vec3 getWorldPosition() const { return glm::vec3(mChunkX * CHUNK_SIZE, 0, mChunkZ * CHUNK_SIZE); }

	static std::map<BlockType, BlockTexturePaths> m_textureConfig;
	static std::map<BlockType, BlockMaterial> m_materialConfig;
    static std::map<std::string, int> m_pathToTextureIndex;
    static int m_nextTextureIndex;

	static BlockMaterial getMaterialForTextureIndex(int textureIndex);

private:
	int mChunkX, mChunkZ;
	BlockType mBlocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];

	GLuint mVAO, mVBO;
	std::vector<CubeVertex> mVertices;
	int mVertexCount;

	static void initializeTextureConfig();
};