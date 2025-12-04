#ifndef CUBE_H
#define CUBE_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>

enum class BlockType {
	AIR,
	GRASS,
	DIRT,
	STONE,
	REDSTONE,
	WOOD,
	LEAVES,
	TORCH,
	GLASS
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

class Cube {
public:
	Cube();
	~Cube();

	void initialize();
	void draw();
	void drawInstanced(int count);

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

	static const int CHUNK_VOXEL_COUNT = CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE;

	Chunk(int chunkX, int chunkZ);
	~Chunk();

	void generate(long long worldSeed);
	void buildMesh();
	void draw();

	BlockType getBlock(int x, int y, int z) const;
	void setBlock(int x, int y, int z, BlockType type);

	glm::vec3 getWorldPosition() const { return glm::vec3(mChunkX * CHUNK_SIZE, 0, mChunkZ * CHUNK_SIZE); }

	const BlockType* getRawBlocks() const { return &mBlocks[0][0][0]; }

	static std::map<BlockType, BlockTexturePaths> m_textureConfig;
    static std::map<std::string, int> m_pathToTextureIndex;
    static int m_nextTextureIndex;

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

	static void initializeTextureConfig();

	glm::vec3 getTextureCoords(BlockType type, const glm::vec3& normal, int corner);
};

struct RaycastHit {
    bool hit;
    glm::vec3 blockPos;
    glm::vec3 hitPos;
    glm::vec3 normal;
};

class World {
public:
	World();
	~World();

	void generate(int renderDistance = 3, long long seed = -1);
	void draw() const;

	const std::vector<Chunk*>& getChunks() const { return mChunks; }

	std::vector<glm::vec3> getRedstoneLightPositions() const;
	std::vector<glm::vec3> World::getTorchLightPositions() const;

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

	Chunk* getChunkAt(int x, int z);
	Chunk* findChunk(int chunkX, int chunkZ) const;
};

#endif // CUBE_H