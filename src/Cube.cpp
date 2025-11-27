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

                        // --- Fill blocks (Minecraft-like layers) ---
                        for (int y = 0; y < CHUNK_HEIGHT; y++) {
                                if (y < height - 4) {
                                        mBlocks[x][y][z] = BlockType::STONE; // deep layer
                                }
                                else if (y < height - 1) {
                                        mBlocks[x][y][z] = BlockType::DIRT; // dirt layer
                                }
                                else if (y == height - 1) {
                                        mBlocks[x][y][z] = BlockType::GRASS; // surface
                                }
                                else {
                                        mBlocks[x][y][z] = BlockType::AIR;
                                }
                        }

                        if (dist(rng) < 0.01f && height + 1 < CHUNK_HEIGHT - 1) {
                                mBlocks[x][height + 1][z] = BlockType::REDSTONE;
                        }

                        if (dist(rng) < 0.01f && height < CHUNK_HEIGHT - 7) {  // 3% chance, need room for tree
                                // Check if this position has grass
                                if (mBlocks[x][height - 1][z] != BlockType::GRASS) {
                                        continue;
                                }

                                // Check for nearby trees to avoid clumping
                                bool hasNearbyTree = false;
                                for (int dx = -3; dx <= 3; dx++) {
                                        for (int dz = -3; dz <= 3; dz++) {
                                                if (dx == 0 && dz == 0) continue;

                                                int checkX = x + dx;
                                                int checkZ = z + dz;

                                                if (checkX >= 0 && checkX < CHUNK_SIZE && checkZ >= 0 && checkZ < CHUNK_SIZE) {
                                                        // Check if there's wood at or above ground level
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

                                // Build leaves (3D spherical blob)
                                int leafTop = height + trunkHeight;
                                mBlocks[x][leafTop][z] = BlockType::LEAVES;
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
        // Texture atlas is 1024x512, each texture is 256x256
        // UV coordinates: 0.0 to 1.0 maps to entire atlas
        // Each texture takes up: 256/1024 = 0.25 horizontally, 256/512 = 0.5 vertically

        float u = 0.0f, v = 0.0f;
        float texSizeU = 0.25f;  // 256/1024
        float texSizeV = 0.5f;   // 256/512

        // Layout: Row 0: [Grass Top] [Grass Side] [Dirt] [Stone]
        //         Row 1: [Redstone] [Wood] [Leaves] [Empty]

        switch (type) {
                case BlockType::GRASS:
                        if (normal.y > 0.5f) { // Top face
                                u = 0.0f; v = 0.5f; // Grass top (0,0)
                        } else if (normal.y < -0.5f) { // Bottom face
                                u = 0.5f; v = 0.5f; // (2,0)
                        } else { // Side faces
                                u = 0.25f; v = 0.5f; // (2,0)
                        }
                        break;
                case BlockType::DIRT:
                        u = 0.5f; v = 0.5f; // (2,0)
                        break;
                case BlockType::STONE:
                        u = 0.75f; v = 0.5f; // (3,0)
                        break;
                case BlockType::REDSTONE:
                        u = 0.0f; v = 0.0f; // (0,1)
                        break;
                case BlockType::WOOD:
                        u = 0.25f; v = 0.0f; // (1,1)
                        break;
                case BlockType::LEAVES:
                        u = 0.5f; v = 0.0f; // (2,1)
                        break;
                case BlockType::AIR:
                        u = 0.75f; v = 0.0f; // (2,1)
                        break;
                default:
                        u = 0.75f; v = 0.0f; // (2,1)
                        break;
        }

        // Add corner offset - each texture is 0.25 x 0.5
        if (corner == 1 || corner == 2) u += texSizeU;
        if (corner == 2 || corner == 3) v += texSizeV;

        return glm::vec2(u, v);
}

void Chunk::addFace(int x, int y, int z, const glm::vec3& normal, BlockType type) {
        glm::vec3 chunkWorldPos = glm::vec3(mChunkX * CHUNK_SIZE, 0, mChunkZ * CHUNK_SIZE);
        glm::vec3 worldPos = chunkWorldPos + glm::vec3(x, y, z);
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

        // CRITICAL FIX: Add TWO triangles (6 vertices) per face, not 3!
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
                                                positions.push_back(chunkPos + glm::vec3(x + 0.5f, y + 0.5f, z + 0.5f));
                                        }
                                }
                        }
                }
        }

        return positions;
}