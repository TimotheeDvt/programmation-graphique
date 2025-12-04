#include "Cube.h"
#include <iostream>
#include <cmath>
#include <random>

std::vector<CubeVertex> Cube::vertices;
std::vector<unsigned int> Cube::indices;
bool Cube::meshDataInitialized = false;

std::map<BlockType, BlockTexturePaths> Chunk::m_textureConfig;
std::map<std::string, int> Chunk::m_pathToTextureIndex;
int Chunk::m_nextTextureIndex = 0;

Cube::Cube() : mVAO(0), mVBO(0), mEBO(0) {
        if (!meshDataInitialized) {
                initializeMeshData();
        }
}

Cube::~Cube() {
        glDeleteVertexArrays(1, &mVAO);
        glDeleteBuffers(1, &mVBO);
        glDeleteBuffers(1, &mEBO);
}

void Cube::initializeMeshData() {
        // Define cube vertices with normals and texture coordinates
        // Front face (+Z)
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f, 0.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f, 0.0f}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}});

        // Back face (-Z)
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}});
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}});

        // Top face (+Y)
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f, 0.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f, 0.0f}});

        // Bottom face (-Y)
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f, 0.0f}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f, 0.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f, 0.0f}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f, 0.0f}});

        // Right face (+X)
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f, 0.0f}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f, 0.0f}});

        // Left face (-X)
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f, 0.0f}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f, 0.0f}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f, 0.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f, 0.0f}});

        // Define indices for triangles (2 triangles per face)
        for (int i = 0; i < 6; i++) {
                int offset = i * 4;
                indices.push_back(offset + 0);
                indices.push_back(offset + 1);
                indices.push_back(offset + 2);
                indices.push_back(offset + 0);
                indices.push_back(offset + 2);
                indices.push_back(offset + 3);
        }

        meshDataInitialized = true;
}

void Cube::initialize() {
        setupMesh();
}

void Cube::setupMesh() {
        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &mVBO);
        glGenBuffers(1, &mEBO);

        glBindVertexArray(mVAO);

        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(CubeVertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)offsetof(CubeVertex, normal));
        glEnableVertexAttribArray(1);

        // TexCoords
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)offsetof(CubeVertex, texCoords));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
}

void Cube::draw() {
        glBindVertexArray(mVAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
}

const std::vector<CubeVertex>& Cube::getVertices() {
        if (!meshDataInitialized) {
                initializeMeshData();
        }
        return vertices;
}

const std::vector<unsigned int>& Cube::getIndices() {
        if (!meshDataInitialized) {
                initializeMeshData();
        }
        return indices;
}

Chunk::Chunk(int chunkX, int chunkZ)
: mChunkX(chunkX), mChunkZ(chunkZ), mVAO(0), mVBO(0), mVertexCount(0) {
        if (m_textureConfig.empty()) {
                initializeTextureConfig();
        }
        for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                        for (int z = 0; z < CHUNK_SIZE; z++) {
                                mBlocks[x][y][z] = BlockType::AIR;
                        }
                }
        }
}

Chunk::~Chunk() {
        glDeleteVertexArrays(1, &mVAO);
        glDeleteBuffers(1, &mVBO);
}

void Chunk::generate() {
        std::mt19937 rng(mChunkX * 928371 + mChunkZ * 1231237);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                        int worldX = mChunkX * CHUNK_SIZE + x;
                        int worldZ = mChunkZ * CHUNK_SIZE + z;

                        float h =
                        sin(worldX * 0.05f) * 4.0f +
                        cos(worldZ * 0.05f) * 4.0f;

                        int height = 16 + (int)h;      // Middle terrain height
                        height = glm::clamp(height, 4, CHUNK_HEIGHT - 10);

                        for (int y = 0; y < CHUNK_HEIGHT; y++) {
                                if (y < height - 4) {
                                        mBlocks[x][y][z] = BlockType::STONE;
                                }
                                else if (y < height - 1) {
                                        mBlocks[x][y][z] = BlockType::DIRT;
                                }
                                else if (y == height - 1) {
                                        mBlocks[x][y][z] = BlockType::GRASS;
                                }
                                else if (mBlocks[x][y][z] != BlockType::LEAVES) {
                                        mBlocks[x][y][z] = BlockType::AIR;
                                }
                        }

                        // Generate torches
                        if (dist(rng) < 0.0004f && height + 1 < CHUNK_HEIGHT - 1) {
                                mBlocks[x][height][z] = BlockType::TORCH;
                        }

                        // Generate redstone blocks
                        if (dist(rng) < 0.0002f) {
                                mBlocks[x][height - 1][z] = BlockType::REDSTONE;
                        }


                        // Tree generation
                        if (dist(rng) < 0.01f && height < CHUNK_HEIGHT - 7 && x > 0 && x < CHUNK_SIZE - 3 && z > 0 && z < CHUNK_SIZE - 3) {
                                if (mBlocks[x][height - 1][z] != BlockType::GRASS) {
                                        continue;
                                }

                                // Check for nearby trees
                                bool hasNearbyTree = false;
                                for (int dx = -3; dx <= 3; dx++) {
                                        for (int dz = -3; dz <= 3; dz++) {
                                                if (dx == 0 && dz == 0) continue;

                                                int checkX = x + dx;
                                                int checkZ = z + dz;

                                                if (checkX >= 0 && checkX < CHUNK_SIZE && checkZ >= 0 && checkZ < CHUNK_SIZE) {
                                                        if (mBlocks[checkX][height][checkZ] == BlockType::WOOD || mBlocks[checkX][height + 1][checkZ] == BlockType::WOOD) {
                                                                hasNearbyTree = true;
                                                                break;
                                                        }
                                                }
                                        }
                                        if (hasNearbyTree) break;
                                }

                                if (hasNearbyTree) continue;

                                // Generate tree
                                int trunkHeight = 4 + (rng() % 2);  // 4-5 blocks tall

                                // Build trunk
                                for (int ty = 0; ty < trunkHeight; ty++) {
                                        if (height + ty < CHUNK_HEIGHT) {
                                                mBlocks[x][height + ty][z] = BlockType::WOOD;
                                        }
                                }

                                // Build leaves (3D sphere)
                                int leafTop = height + trunkHeight - 1;
                                for (int lx = -2; lx <= 2; lx++) {
                                        for (int ly = -2; ly <= 2; ly++) {
                                                for (int lz = -2; lz <= 2; lz++) {
                                                        float distToCenter = sqrtf(lx * lx + ly * ly + lz * lz);
                                                        if (distToCenter < 3.0f) {
                                                                int leafX = x + lx;
                                                                int leafY = leafTop + ly;
                                                                int leafZ = z + lz;

                                                                if (leafX >= 0 && leafX < CHUNK_SIZE &&
                                                                    leafY >= 0 && leafY < CHUNK_HEIGHT &&
                                                                    leafZ >= 0 && leafZ < CHUNK_SIZE) {
                                                                        if (mBlocks[leafX][leafY][leafZ] == BlockType::AIR) {
                                                                                mBlocks[leafX][leafY][leafZ] = BlockType::LEAVES;
                                                                        }
                                                                }
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }
}

BlockType Chunk::getBlock(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
                return BlockType::AIR;
        }
        return mBlocks[x][y][z];
}

void Chunk::initializeTextureConfig() {
        auto getIndex = [](const std::string& path) -> int {
                if (m_pathToTextureIndex.find(path) == m_pathToTextureIndex.end()) {
                        m_pathToTextureIndex[path] = m_nextTextureIndex++;
                }
                return m_pathToTextureIndex[path];
        };

        m_textureConfig[BlockType::GRASS] = {
                "./textures/grass-top.jpg",   // Top
                "./textures/dirt.png",        // Bottom
                "./textures/grass-side.png",  // Side
                ""
        };

        m_textureConfig[BlockType::WOOD] = {
                "./textures/wood-top.png",
                "./textures/wood-top.png",
                "./textures/wood-side.png",
                ""
        };

        m_textureConfig[BlockType::DIRT] = {
                "./textures/dirt.png",
                "./textures/dirt.png",
                "./textures/dirt.png",
                ""
        };

        m_textureConfig[BlockType::STONE] = {
                "./textures/stone.png",
                "./textures/stone.png",
                "./textures/stone.png",
                ""
        };

        m_textureConfig[BlockType::REDSTONE] = {
                "./textures/redstone.png",
                "./textures/redstone.png",
                "./textures/redstone.png",
                ""
        };

        m_textureConfig[BlockType::LEAVES] = {
                "./textures/leaves.png",
                "./textures/leaves.png",
                "./textures/leaves.png",
                ""
        };

        m_textureConfig[BlockType::TORCH] = {
                "", "", "",
                "./textures/torch.png"
        };

        m_textureConfig[BlockType::GLASS] = {
                "./textures/glass.png",
                "./textures/glass.png",
                "./textures/glass.png",
                ""
        };

        for (const auto& pair : m_textureConfig) {
                if (!pair.second.top.empty()) getIndex(pair.second.top);
                if (!pair.second.bottom.empty()) getIndex(pair.second.bottom);
                if (!pair.second.side.empty()) getIndex(pair.second.side);
                if (!pair.second.special.empty()) getIndex(pair.second.special);
        }
}

void World::localToChunkCoords(int worldX, int worldY, int worldZ,
        int& chunkX, int& chunkZ,
        int& localX, int& localY, int& localZ
) const{
        chunkX = (worldX >= 0) ? worldX / Chunk::CHUNK_SIZE : (worldX + 1) / Chunk::CHUNK_SIZE - 1;
        chunkZ = (worldZ >= 0) ? worldZ / Chunk::CHUNK_SIZE : (worldZ + 1) / Chunk::CHUNK_SIZE - 1;

        localX = worldX - chunkX * Chunk::CHUNK_SIZE;
        localZ = worldZ - chunkZ * Chunk::CHUNK_SIZE;
        localY = worldY;
}

BlockType World::getBlock(int x, int y, int z) const {
        int chunkX, chunkZ, localX, localY, localZ;
        localToChunkCoords(x, y, z, chunkX, chunkZ, localX, localY, localZ);
        Chunk* chunk = const_cast<World*>(this)->findChunk(chunkX, chunkZ);
        if (!chunk) return BlockType::AIR;
        return chunk->getBlock(localX, localY, localZ);
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
                mBlocks[x][y][z] = type;
        }
}

bool Chunk::isSolidBlock(BlockType type) const {
        return type != BlockType::AIR;
}

bool Chunk::isFullBlock(BlockType type) const {
        if (type == BlockType::TORCH) return false;
        return type != BlockType::AIR;
}

bool Chunk::shouldRenderFace(int x, int y, int z, int nx, int ny, int nz) const {
        BlockType neighbor = getBlock(x + nx, y + ny, z + nz);
        BlockType current  = getBlock(x, y, z);

        // Don't render if neighbor is a full block (except for leaves and glass)
        if (isFullBlock(neighbor) && neighbor != BlockType::LEAVES && neighbor != BlockType::GLASS) {
                return false;
        }

        return true;
}

glm::vec3 Chunk::getTextureCoords(BlockType type, const glm::vec3& normal, int corner) {
        std::string path = "";
        if (m_textureConfig.count(type)) {
                const auto& config = m_textureConfig.at(type);

                if (glm::abs(normal.y) > 0.5f) {
                        path = (normal.y > 0.5f) ? config.top : config.bottom;
                } else {
                        path = config.side;
                }
        }

        int textureIndex = -1;
        if (m_pathToTextureIndex.count(path) && path != "") {
                textureIndex = m_pathToTextureIndex.at(path);
        }

        glm::vec2 uv;
        switch (corner) {
                case 0: uv = glm::vec2(0.0f, 0.0f); break;
                case 1: uv = glm::vec2(1.0f, 0.0f); break;
                case 2: uv = glm::vec2(1.0f, 1.0f); break;
                case 3: uv = glm::vec2(0.0f, 1.0f); break;
                default: uv = glm::vec2(0.0f, 0.0f); break;
        }

        return glm::vec3(uv.x, uv.y, (float)textureIndex);
}

void Chunk::addFace(int x, int y, int z, const glm::vec3& normal, BlockType type) {
        glm::vec3 chunkWorldPos = glm::vec3(mChunkX * CHUNK_SIZE, 0, mChunkZ * CHUNK_SIZE);
        glm::vec3 worldPos = chunkWorldPos + glm::vec3(x, y, z);

        glm::vec3 corners[4];

        if (normal.z > 0.5f) { // Front (+Z)
                corners[0] = worldPos + glm::vec3(-0.5f, -0.5f,  0.5f);
                corners[1] = worldPos + glm::vec3( 0.5f, -0.5f,  0.5f);
                corners[2] = worldPos + glm::vec3( 0.5f,  0.5f,  0.5f);
                corners[3] = worldPos + glm::vec3(-0.5f,  0.5f,  0.5f);
        } else if (normal.z < -0.5f) { // Back (-Z)
                corners[0] = worldPos + glm::vec3( 0.5f, -0.5f, -0.5f);
                corners[1] = worldPos + glm::vec3(-0.5f, -0.5f, -0.5f);
                corners[2] = worldPos + glm::vec3(-0.5f,  0.5f, -0.5f);
                corners[3] = worldPos + glm::vec3( 0.5f,  0.5f, -0.5f);
        } else if (normal.y > 0.5f) { // Top (+Y)
                corners[0] = worldPos + glm::vec3(-0.5f,  0.5f,  0.5f);
                corners[1] = worldPos + glm::vec3( 0.5f,  0.5f,  0.5f);
                corners[2] = worldPos + glm::vec3( 0.5f,  0.5f, -0.5f);
                corners[3] = worldPos + glm::vec3(-0.5f,  0.5f, -0.5f);
        } else if (normal.y < -0.5f) { // Bottom (-Y)
                corners[0] = worldPos + glm::vec3(-0.5f, -0.5f, -0.5f);
                corners[1] = worldPos + glm::vec3( 0.5f, -0.5f, -0.5f);
                corners[2] = worldPos + glm::vec3( 0.5f, -0.5f,  0.5f);
                corners[3] = worldPos + glm::vec3(-0.5f, -0.5f,  0.5f);
        } else if (normal.x > 0.5f) { // Right (+X)
                corners[0] = worldPos + glm::vec3( 0.5f, -0.5f,  0.5f);
                corners[1] = worldPos + glm::vec3( 0.5f, -0.5f, -0.5f);
                corners[2] = worldPos + glm::vec3( 0.5f,  0.5f, -0.5f);
                corners[3] = worldPos + glm::vec3( 0.5f,  0.5f,  0.5f);
        } else { // Left (-X)
                corners[0] = worldPos + glm::vec3(-0.5f, -0.5f, -0.5f);
                corners[1] = worldPos + glm::vec3(-0.5f, -0.5f,  0.5f);
                corners[2] = worldPos + glm::vec3(-0.5f,  0.5f,  0.5f);
                corners[3] = worldPos + glm::vec3(-0.5f,  0.5f, -0.5f);
        }

        // Triangle 1: corners 0, 1, 2
        CubeVertex v0, v1, v2;
        v0.position = corners[0];
        v0.normal = normal;
        v0.texCoords = getTextureCoords(type, normal, 0);

        v1.position = corners[1];
        v1.normal = normal;
        v1.texCoords = getTextureCoords(type, normal, 1);

        v2.position = corners[2];
        v2.normal = normal;
        v2.texCoords = getTextureCoords(type, normal, 2);

        mVertices.push_back(v0);
        mVertices.push_back(v1);
        mVertices.push_back(v2);

        // Triangle 2: corners 0, 2, 3
        CubeVertex v3, v4, v5;
        v3.position = corners[0];
        v3.normal = normal;
        v3.texCoords = getTextureCoords(type, normal, 0);

        v4.position = corners[2];
        v4.normal = normal;
        v4.texCoords = getTextureCoords(type, normal, 2);

        v5.position = corners[3];
        v5.normal = normal;
        v5.texCoords = getTextureCoords(type, normal, 3);

        mVertices.push_back(v3);
        mVertices.push_back(v4);
        mVertices.push_back(v5);
}

void Chunk::buildMesh() {
        mVertices.clear();

        for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                        for (int z = 0; z < CHUNK_SIZE; z++) {
                                BlockType type = getBlock(x, y, z);
                                if (type == BlockType::AIR) continue;

                                if (type == BlockType::TORCH) {
                                        addTorchMesh(x, y, z);
                                        continue;
                                }

                                if (!isFullBlock(type)) continue;

                                // Culling: only add faces that are exposed
                                if (shouldRenderFace(x, y, z, 0, 0, 1))  addFace(x, y, z, glm::vec3( 0,  0,  1), type);
                                if (shouldRenderFace(x, y, z, 0, 0, -1)) addFace(x, y, z, glm::vec3( 0,  0, -1), type);
                                if (shouldRenderFace(x, y, z, 0, 1, 0))  addFace(x, y, z, glm::vec3( 0,  1,  0), type);
                                if (shouldRenderFace(x, y, z, 0, -1, 0)) addFace(x, y, z, glm::vec3( 0, -1,  0), type);
                                if (shouldRenderFace(x, y, z, 1, 0, 0))  addFace(x, y, z, glm::vec3( 1,  0,  0), type);
                                if (shouldRenderFace(x, y, z, -1, 0, 0)) addFace(x, y, z, glm::vec3(-1,  0,  0), type);
                        }
                }
        }

        mVertexCount = mVertices.size();

        if (mVAO == 0) {
                glGenVertexArrays(1, &mVAO);
                glGenBuffers(1, &mVBO);
        }

        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(CubeVertex), mVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)offsetof(CubeVertex, normal));
        glEnableVertexAttribArray(1);

        // texCoords now contain (u, v, textureIndex) so we must read 3 floats
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)offsetof(CubeVertex, texCoords));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
}

void Chunk::addTorchMesh(int x, int y, int z) {
        glm::vec3 chunkWorldPos = glm::vec3(mChunkX * CHUNK_SIZE, 0, mChunkZ * CHUNK_SIZE);
        glm::vec3 worldPos = chunkWorldPos + glm::vec3(x + 0.5f, y, z + 0.5f); // center of block

        float radius = 0.1f;
        float baseY  = -0.5f;
        float topY   = 0.5f;

        struct Quad { glm::vec3 p[4]; glm::vec3 normal; };
        Quad quads[6];

        // The torch mesh is made of two intersecting quads.
        // We define 4 faces for lighting to work from all sides.

        // Quad 1: along Z
        quads[0].p[0] = worldPos + glm::vec3(-0.5f, baseY, 0.0f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[0].p[1] = worldPos + glm::vec3( 0.5f, baseY, 0.0f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[0].p[2] = worldPos + glm::vec3( 0.5f, topY,  0.0f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[0].p[3] = worldPos + glm::vec3(-0.5f, topY,  0.0f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[0].normal = glm::vec3(0, 0, 1);

        // Quad 2: also along Z, but facing the other way for two-sided lighting
        quads[1].p[0] = worldPos + glm::vec3( 0.5f, baseY, 0.0f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[1].p[1] = worldPos + glm::vec3(-0.5f, baseY, 0.0f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[1].p[2] = worldPos + glm::vec3(-0.5f, topY,  0.0f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[1].p[3] = worldPos + glm::vec3( 0.5f, topY,  0.0f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[1].normal = glm::vec3(0, 0, -1);

        // Quad 3: along X
        quads[2].p[0] = worldPos + glm::vec3(0.0f, baseY, -0.5f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[2].p[1] = worldPos + glm::vec3(0.0f, baseY,  0.5f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[2].p[2] = worldPos + glm::vec3(0.0f, topY,   0.5f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[2].p[3] = worldPos + glm::vec3(0.0f, topY,  -0.5f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[2].normal = glm::vec3(1, 0, 0);

        // Quad 4: also along X, facing the other way
        quads[3].p[0] = worldPos + glm::vec3(0.0f, baseY,  0.5f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[3].p[1] = worldPos + glm::vec3(0.0f, baseY, -0.5f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[3].p[2] = worldPos + glm::vec3(0.0f, topY,  -0.5f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[3].p[3] = worldPos + glm::vec3(0.0f, topY,   0.5f) - glm::vec3(0.5f, 0.0f, 0.5f);
        quads[3].normal = glm::vec3(-1, 0, 0);

        int textureIndex = -1;
        const auto& config = m_textureConfig.at(BlockType::TORCH);
        if (m_pathToTextureIndex.count(config.special)) {
                textureIndex = m_pathToTextureIndex.at(config.special);
        }
        float texIdx = (float)textureIndex;

        for (int q = 0; q < 4; ++q) {
                glm::vec3 n = quads[q].normal;

                CubeVertex v[6];

                // Triangle 1: 0,1,2
                v[0].position = quads[q].p[0];
                v[0].normal   = n;
                v[0].texCoords = glm::vec3(0.0f, 0.0f, texIdx);

                v[1].position = quads[q].p[1];
                v[1].normal   = n;
                v[1].texCoords = glm::vec3(1.0f, 0.0f, texIdx);

                v[2].position = quads[q].p[2];
                v[2].normal   = n;
                v[2].texCoords = glm::vec3(1.0f, 1.0f, texIdx);

                // Triangle 2: 0,2,3
                v[3].position = quads[q].p[0];
                v[3].normal   = n;
                v[3].texCoords = glm::vec3(0.0f, 0.0f, texIdx);

                v[4].position = quads[q].p[2];
                v[4].normal   = n;
                v[4].texCoords = glm::vec3(1.0f, 1.0f, texIdx);

                v[5].position = quads[q].p[3];
                v[5].normal   = n;
                v[5].texCoords = glm::vec3(0.0f, 1.0f, texIdx);

                for (int i = 0; i < 6; ++i) {
                        mVertices.push_back(v[i]);
                }
        }
}


void Chunk::draw() {
        if (mVertexCount == 0) return;

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLES, 0, mVertexCount);
        glBindVertexArray(0);
}


World::World() {}

World::~World() {
        for (auto chunk : mChunks) {
                delete chunk;
        }
        mChunks.clear();
}

void World::generate(int renderDistance) {
        if (renderDistance == 1) {
                Chunk* chunk = new Chunk(0, 0);
                chunk->generate();
                chunk->buildMesh();
                mChunks.push_back(chunk);
                return;
        }

        for (int x = -renderDistance; x <= renderDistance; x++) {
                for (int z = -renderDistance; z <= renderDistance; z++) {
                        Chunk* chunk = new Chunk(x, z);
                        chunk->generate();
                        chunk->buildMesh();
                        mChunks.push_back(chunk);
                }
        }
}

void World::draw() const {
        for (auto chunk : mChunks) {
                chunk->draw();
        }
}

Chunk* World::findChunk(int chunkX, int chunkZ) const {
        for (auto chunk : mChunks) {
                glm::vec3 pos = chunk->getWorldPosition();
                int cx = (int)(pos.x / Chunk::CHUNK_SIZE);
                int cz = (int)(pos.z / Chunk::CHUNK_SIZE);
                if (cx == chunkX && cz == chunkZ) return chunk;
        }
        return nullptr;
}

BlockType World::getBlockAt(const glm::vec3& worldPos) const {
        int wx = (int)floor(worldPos.x);
        int wy = (int)floor(worldPos.y);
        int wz = (int)floor(worldPos.z);

        int chunkX = (int)floor((float)wx / Chunk::CHUNK_SIZE);
        int chunkZ = (int)floor((float)wz / Chunk::CHUNK_SIZE);

        Chunk* chunk = findChunk(chunkX, chunkZ);
        if (!chunk) return BlockType::AIR;

        int localX = wx - chunkX * Chunk::CHUNK_SIZE;
        int localZ = wz - chunkZ * Chunk::CHUNK_SIZE;
        return chunk->getBlock(localX, wy, localZ);
}

bool World::setBlockAt(const glm::vec3& worldPos, BlockType type) {
        int wx = (int)floor(worldPos.x);
        int wy = (int)floor(worldPos.y);
        int wz = (int)floor(worldPos.z);

        if (wy < 0 || wy >= Chunk::CHUNK_HEIGHT) return false;

        int chunkX = (int)floor((float)wx / Chunk::CHUNK_SIZE);
        int chunkZ = (int)floor((float)wz / Chunk::CHUNK_SIZE);

        Chunk* chunk = findChunk(chunkX, chunkZ);
        if (!chunk) return false;

        int localX = wx - chunkX * Chunk::CHUNK_SIZE;
        int localZ = wz - chunkZ * Chunk::CHUNK_SIZE;

        chunk->setBlock(localX, wy, localZ, type);
        chunk->buildMesh();
        return true;
}

std::vector<glm::vec3> World::getRedstoneLightPositions() const {
        std::vector<glm::vec3> positions;

        for (auto chunk : mChunks) {
                glm::vec3 chunkPos = chunk->getWorldPosition();

                for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
                        for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++) {
                                for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
                                        if (chunk->getBlock(x, y, z) == BlockType::REDSTONE) {
                                                glm::vec3 basePos = chunkPos + glm::vec3(x, y, z);

                                                positions.push_back(basePos + glm::vec3(0.0f, 0.0f, 1.01f));
                                                positions.push_back(basePos + glm::vec3(0.0f, 1.01f, 0.0f));
                                                positions.push_back(basePos + glm::vec3(1.01f, 0.0f, 0.0f));

                                                positions.push_back(basePos + glm::vec3(0.0f, 0.0f, -1.01f));
                                                positions.push_back(basePos + glm::vec3(0.0f, -1.01f, 0.0f));
                                                positions.push_back(basePos + glm::vec3(-1.01f, 0.0f, 0.0f));
                                        }
                                }
                        }
                }
        }

        return positions;
}

std::vector<glm::vec3> World::getTorchLightPositions() const {
        std::vector<glm::vec3> positions;

        for (auto chunk : mChunks) {
                glm::vec3 chunkPos = chunk->getWorldPosition();

                for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
                        for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++) {
                                for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
                                        if (chunk->getBlock(x, y, z) == BlockType::TORCH) {
                                                positions.push_back(
                                                        chunkPos
                                                        + glm::vec3(x + 0.5f, y + 0.5f, z + 0.5f)
                                                        - glm::vec3(0.5f, 0.3f, 0.5f)
                                                );
                                        }
                                }
                        }
                }
        }
        return positions;
}