#include "Chunk.h"
#include <iostream>
#include <cmath>
#include <random>

std::map<BlockType, BlockTexturePaths> Chunk::m_textureConfig;
std::map<std::string, int> Chunk::m_pathToTextureIndex;
int Chunk::m_nextTextureIndex = 0;

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

void Chunk::generate(long long worldSeed) {
        // Combine world seed with chunk position for a unique, deterministic chunk seed
        long long chunkSeed = worldSeed + mChunkX * 928371 + mChunkZ * 1231237;
        std::mt19937 rng(chunkSeed);
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

void Chunk::setBlock(int x, int y, int z, BlockType type) {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
                mBlocks[x][y][z] = type;
        }
}

bool isSolidBlock(BlockType type) {
        return type != BlockType::AIR;
}

bool isFullBlock(BlockType type) {
        if (type == BlockType::TORCH) return false;
        return type != BlockType::AIR;
}

bool shouldRenderFace(const Chunk* chunk, int x, int y, int z, int nx, int ny, int nz) {
        BlockType neighbor = chunk->getBlock(x + nx, y + ny, z + nz);

        // Don't render if neighbor is a full block (except for leaves and glass)
        if (isFullBlock(neighbor) && neighbor != BlockType::LEAVES && neighbor != BlockType::GLASS) {
                return false;
        }

        return true;
}

glm::vec3 getTextureCoords(BlockType type, const glm::vec3& normal, int corner) {
        std::string path = "";
        if (Chunk::m_textureConfig.count(type)) {
                const auto& config = Chunk::m_textureConfig.at(type);

                if (glm::abs(normal.y) > 0.5f) {
                        path = (normal.y > 0.5f) ? config.top : config.bottom;
                } else {
                        path = config.side;
                }
        }

        int textureIndex = -1;
        if (Chunk::m_pathToTextureIndex.count(path) && path != "") {
                textureIndex = Chunk::m_pathToTextureIndex.at(path);
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

void addFace(std::vector<CubeVertex>& vertices, int chunkX, int chunkZ, int x, int y, int z, const glm::vec3& normal, BlockType type) {
    glm::vec3 chunkWorldPos = glm::vec3(chunkX * Chunk::CHUNK_SIZE, 0, chunkZ * Chunk::CHUNK_SIZE);
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
    v0.position = corners[0]; v0.normal = normal; v0.texCoords = getTextureCoords(type, normal, 0);
    v1.position = corners[1]; v1.normal = normal; v1.texCoords = getTextureCoords(type, normal, 1);
    v2.position = corners[2]; v2.normal = normal; v2.texCoords = getTextureCoords(type, normal, 2);
    vertices.push_back(v0); vertices.push_back(v1); vertices.push_back(v2);

    // Triangle 2: corners 0, 2, 3
    CubeVertex v3, v4, v5;
    v3.position = corners[0]; v3.normal = normal; v3.texCoords = getTextureCoords(type, normal, 0);
    v4.position = corners[2]; v4.normal = normal; v4.texCoords = getTextureCoords(type, normal, 2);
    v5.position = corners[3]; v5.normal = normal; v5.texCoords = getTextureCoords(type, normal, 3);
    vertices.push_back(v3); vertices.push_back(v4); vertices.push_back(v5);
}

void addTorchMesh(std::vector<CubeVertex>& vertices, int chunkX, int chunkZ, int x, int y, int z) {
    glm::vec3 chunkWorldPos = glm::vec3(chunkX * Chunk::CHUNK_SIZE, 0, chunkZ * Chunk::CHUNK_SIZE);
    glm::vec3 worldPos = chunkWorldPos + glm::vec3(x + 0.5f, y, z + 0.5f); // center of block

    float baseY  = -0.5f;
    float topY   = 0.5f;

    struct Quad { glm::vec3 p[4]; glm::vec3 normal; };
    Quad quads[4];

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
    const auto& config = Chunk::m_textureConfig.at(BlockType::TORCH);
    if (Chunk::m_pathToTextureIndex.count(config.special)) {
        textureIndex = Chunk::m_pathToTextureIndex.at(config.special);
    }
    float texIdx = (float)textureIndex;

    for (int q = 0; q < 4; ++q) {
        glm::vec3 n = quads[q].normal;
        CubeVertex v[6];
        // Tri 1
        v[0] = {quads[q].p[0], n, glm::vec3(0.0f, 0.0f, texIdx)};
        v[1] = {quads[q].p[1], n, glm::vec3(1.0f, 0.0f, texIdx)};
        v[2] = {quads[q].p[2], n, glm::vec3(1.0f, 1.0f, texIdx)};
        // Tri 2
        v[3] = {quads[q].p[0], n, glm::vec3(0.0f, 0.0f, texIdx)};
        v[4] = {quads[q].p[2], n, glm::vec3(1.0f, 1.0f, texIdx)};
        v[5] = {quads[q].p[3], n, glm::vec3(0.0f, 1.0f, texIdx)};
        for (int i = 0; i < 6; ++i) vertices.push_back(v[i]);
    }
}

void Chunk::buildMesh() {
        mVertices.clear();

        for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                        for (int z = 0; z < CHUNK_SIZE; z++) {
                                BlockType type = getBlock(x, y, z);
                                if (type == BlockType::AIR) continue;

                                if (type == BlockType::TORCH) {
                                        addTorchMesh(mVertices, mChunkX, mChunkZ, x, y, z);
                                        continue;
                                }

                                if (!isFullBlock(type)) continue;

                                // Culling: only add faces that are exposed
                                if (shouldRenderFace(this, x, y, z, 0, 0, 1))  addFace(mVertices, mChunkX, mChunkZ, x, y, z, glm::vec3( 0,  0,  1), type);
                                if (shouldRenderFace(this, x, y, z, 0, 0, -1)) addFace(mVertices, mChunkX, mChunkZ, x, y, z, glm::vec3( 0,  0, -1), type);
                                if (shouldRenderFace(this, x, y, z, 0, 1, 0))  addFace(mVertices, mChunkX, mChunkZ, x, y, z, glm::vec3( 0,  1,  0), type);
                                if (shouldRenderFace(this, x, y, z, 0, -1, 0)) addFace(mVertices, mChunkX, mChunkZ, x, y, z, glm::vec3( 0, -1,  0), type);
                                if (shouldRenderFace(this, x, y, z, 1, 0, 0))  addFace(mVertices, mChunkX, mChunkZ, x, y, z, glm::vec3( 1,  0,  0), type);
                                if (shouldRenderFace(this, x, y, z, -1, 0, 0)) addFace(mVertices, mChunkX, mChunkZ, x, y, z, glm::vec3(-1,  0,  0), type);
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

void Chunk::draw() {
        if (mVertexCount == 0) return;

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLES, 0, mVertexCount);
        glBindVertexArray(0);
}