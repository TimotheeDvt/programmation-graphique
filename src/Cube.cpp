#include "Cube.h"
#include <iostream>
#include <cmath>
#include <random>

std::vector<CubeVertex> Cube::vertices;
std::vector<unsigned int> Cube::indices;
bool Cube::meshDataInitialized = false;

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
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}});

        // Back face (-Z)
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}});
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}});

        // Top face (+Y)
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}});

        // Bottom face (-Y)
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}});

        // Right face (+X)
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}});

        // Left face (-X)
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}});

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
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)offsetof(CubeVertex, texCoords));
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

// Chunk Implementation
Chunk::Chunk(int chunkX, int chunkZ)
: mChunkX(chunkX), mChunkZ(chunkZ), mVAO(0), mVBO(0), mVertexCount(0) {
        // Initialize all blocks to air
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
        std::mt19937 rng(mChunkX * 1000 + mChunkZ);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                        // Simple terrain generation with some randomness
                        int worldX = mChunkX * CHUNK_SIZE + x;
                        int worldZ = mChunkZ * CHUNK_SIZE + z;

                        float noise = std::sin(worldX * 0.1f) * std::cos(worldZ * 0.1f);
                        int height = 4 + (int)(noise * 3.0f);

                        for (int y = 0; y < CHUNK_HEIGHT; y++) {
                                if (y == 0) {
                                        mBlocks[x][y][z] = BlockType::STONE;
                                } else if (y < height - 1) {
                                        mBlocks[x][y][z] = BlockType::DIRT;
                                } else if (y == height - 1) {
                                        mBlocks[x][y][z] = BlockType::GRASS;
                                } else {
                                        mBlocks[x][y][z] = BlockType::AIR;
                                }
                        }

                        // Add random redstone blocks on surface
                        if (dist(rng) < 0.05f && height < CHUNK_HEIGHT - 1) {
                                mBlocks[x][height][z] = BlockType::REDSTONE;
                        }

                        // Add some trees
                        if (dist(rng) < 0.02f && height < CHUNK_HEIGHT - 5) {
                                int treeHeight = 4;
                                for (int ty = 0; ty < treeHeight; ty++) {
                                        if (height + ty < CHUNK_HEIGHT) {
                                                mBlocks[x][height + ty][z] = BlockType::WOOD;
                                        }
                                }
                                // Add leaves
                                for (int lx = -1; lx <= 1; lx++) {
                                        for (int lz = -1; lz <= 1; lz++) {
                                                for (int ly = treeHeight - 2; ly < treeHeight + 1; ly++) {
                                                        int px = x + lx;
                                                        int pz = z + lz;
                                                        int py = height + ly;
                                                        if (px >= 0 && px < CHUNK_SIZE && pz >= 0 && pz < CHUNK_SIZE && py < CHUNK_HEIGHT) {
                                                                if (mBlocks[px][py][pz] == BlockType::AIR) {
                                                                        mBlocks[px][py][pz] = BlockType::LEAVES;
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

void Chunk::setBlock(int x, int y, int z, BlockType type) {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
                mBlocks[x][y][z] = type;
        }
}

bool Chunk::isSolidBlock(BlockType type) const {
        return type != BlockType::AIR;
}

bool Chunk::shouldRenderFace(int x, int y, int z, int nx, int ny, int nz) const {
        BlockType neighbor = getBlock(x + nx, y + ny, z + nz);
        BlockType current = getBlock(x, y, z);

        // Don't render if neighbor is solid (except for leaves)
        if (isSolidBlock(neighbor) && neighbor != BlockType::LEAVES) {
                return false;
        }

        // Always render leaves faces
        if (current == BlockType::LEAVES) {
                return neighbor != BlockType::LEAVES;
        }

        return true;
}

glm::vec2 Chunk::getTextureCoords(BlockType type, const glm::vec3& normal, int corner) {
        // Simple texture atlas: each block type gets different UV coordinates
        float u = 0.0f, v = 0.0f;

        // Simplified texture mapping
        switch (type) {
                case BlockType::GRASS:
                if (normal.y > 0.5f) { // Top face
                        u = 0.0f; v = 0.0f; // Grass top
                } else if (normal.y < -0.5f) { // Bottom face
                        u = 0.5f; v = 0.0f; // Dirt
                } else { // Side faces
                        u = 0.25f; v = 0.0f; // Grass side
                }
                break;
                case BlockType::DIRT:
                u = 0.5f; v = 0.0f;
                break;
                case BlockType::STONE:
                u = 0.75f; v = 0.0f;
                break;
                case BlockType::REDSTONE:
                u = 0.0f; v = 0.5f;
                break;
                case BlockType::WOOD:
                u = 0.25f; v = 0.5f;
                break;
                case BlockType::LEAVES:
                u = 0.5f; v = 0.5f;
                break;
                default:
                break;
        }

        // Add corner offset (simplified)
        if (corner == 1 || corner == 2) u += 0.24f;
        if (corner == 2 || corner == 3) v += 0.24f;

        return glm::vec2(u, v);
}

void Chunk::addFace(int x, int y, int z, const glm::vec3& normal, BlockType type) {
        glm::vec3 worldPos = glm::vec3(x, y, z);

        // Define the four corners of the face based on normal direction
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

        // Add two triangles for the face
        for (int i = 0; i < 3; i++) {
                CubeVertex vertex;
                vertex.position = corners[i];
                vertex.normal = normal;
                vertex.texCoords = getTextureCoords(type, normal, i);
                mVertices.push_back(vertex);
        }
}

void Chunk::buildMesh() {
        mVertices.clear();

        // Build mesh with face culling
        for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                        for (int z = 0; z < CHUNK_SIZE; z++) {
                                BlockType type = getBlock(x, y, z);
                                if (!isSolidBlock(type)) continue;

                                // Check each face
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

        // Setup OpenGL buffers
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

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)offsetof(CubeVertex, texCoords));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
}

void Chunk::draw() {
        if (mVertexCount == 0) return;

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLES, 0, mVertexCount);
        glBindVertexArray(0);
}

// World Implementation
World::World() {}

World::~World() {
        for (auto chunk : mChunks) {
                delete chunk;
        }
        mChunks.clear();
}

void World::generate(int renderDistance) {
        for (int x = -renderDistance; x <= renderDistance; x++) {
                for (int z = -renderDistance; z <= renderDistance; z++) {
                        Chunk* chunk = new Chunk(x, z);
                        chunk->generate();
                        chunk->buildMesh();
                        mChunks.push_back(chunk);
                }
        }
}

void World::draw() {
        for (auto chunk : mChunks) {
                chunk->draw();
        }
}

std::vector<glm::vec3> World::getRedstoneLightPositions() const {
        std::vector<glm::vec3> positions;

        for (auto chunk : mChunks) {
                glm::vec3 chunkPos = chunk->getWorldPosition();

                for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
                        for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++) {
                                for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
                                        if (chunk->getBlock(x, y, z) == BlockType::REDSTONE) {
                                                positions.push_back(chunkPos + glm::vec3(x, y, z));
                                        }
                                }
                        }
                }
        }

        return positions;
}