#ifndef CUBE_H
#define CUBE_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

enum class BlockType {
	AIR,
	GRASS,
	DIRT,
	STONE,
	REDSTONE,
	WOOD,
	LEAVES
};

struct CubeVertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

class Cube {
	public:
	Cube();
	~Cube();

	void initialize();
	void draw();
	void drawInstanced(int count);

	// Get cube mesh data for building chunks
	static const std::vector<CubeVertex>& getVertices();
	static const std::vector<unsigned int>& getIndices();

	private:
	void setupMesh();

	GLuint mVAO, mVBO, mEBO;
	static std::vector<CubeVertex> vertices;
	static std::vector<unsigned int> indices;
	static bool meshDataInitialized;

	static void initializeMeshData();
};

class Chunk {
	public:
	static const int CHUNK_SIZE = 16;
	static const int CHUNK_HEIGHT = 16;

	Chunk(int chunkX, int chunkZ);
	~Chunk();

	void generate();
	void buildMesh();
	void draw();

	BlockType getBlock(int x, int y, int z) const;
	void setBlock(int x, int y, int z, BlockType type);

	glm::vec3 getWorldPosition() const { return glm::vec3(mChunkX * CHUNK_SIZE, 0, mChunkZ * CHUNK_SIZE); }

	private:
	int mChunkX, mChunkZ;
	BlockType mBlocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];

	GLuint mVAO, mVBO;
	std::vector<CubeVertex> mVertices;
	int mVertexCount;

	bool isSolidBlock(BlockType type) const;
	bool shouldRenderFace(int x, int y, int z, int nx, int ny, int nz) const;
	void addFace(int x, int y, int z, const glm::vec3& normal, BlockType type);
	glm::vec2 getTextureCoords(BlockType type, const glm::vec3& normal, int corner);
};

class World {
	public:
	World();
	~World();

	void generate(int renderDistance = 3);
	void draw();

	std::vector<glm::vec3> getRedstoneLightPositions() const;

	private:
	std::vector<Chunk*> mChunks;
};

#endif // CUBE_H