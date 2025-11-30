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
	LEAVES,
	TORCH
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
	static const int CHUNK_HEIGHT = 64;

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
	bool isFullBlock(BlockType type) const;
	bool shouldRenderFace(int x, int y, int z, int nx, int ny, int nz) const;
	void addFace(int x, int y, int z, const glm::vec3& normal, BlockType type);
    void addTorchMesh(int x, int y, int z);
	glm::vec2 getTextureCoords(BlockType type, const glm::vec3& normal, int corner);
};

struct RaycastHit {
    bool hit;
    glm::vec3 blockPos;   // integer coords of hit block
    glm::vec3 hitPos;     // exact position
    glm::vec3 normal;     // face normal
};

class World {
public:
	World();
	~World();

	void generate(int renderDistance = 3);
	void draw();

	std::vector<glm::vec3> getRedstoneLightPositions() const;

    bool setBlockAt(const glm::vec3& worldPos, BlockType type); // <--- NEW
    BlockType getBlockAt(const glm::vec3& worldPos) const;      // <--- NEW

	// Block interaction
	RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = 5.0f);
	bool breakBlock(const glm::vec3& origin, const glm::vec3& direction);
	bool placeBlock(const glm::vec3& origin, const glm::vec3& direction, BlockType type);

	BlockType getBlock(int x, int y, int z) const;
	void setBlock(int x, int y, int z, BlockType type);
	BlockType getBlock2(int x, int y, int z) const;
	void localToChunkCoords(int worldX, int worldY, int worldZ,
                        int& chunkX, int& chunkZ,
                        int& localX, int& localY, int& localZ) const;

private:
	std::vector<Chunk*> mChunks;

	Chunk* getChunkAt(int x, int z);
	Chunk* findChunk(int chunkX, int chunkZ) const;
};

#endif // CUBE_H