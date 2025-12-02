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

// Chunk Implementation
Chunk::Chunk(int chunkX, int chunkZ)
: mChunkX(chunkX), mChunkZ(chunkZ), mVAO(0), mVBO(0), mVertexCount(0) {
        // Initialize all blocks to air
        if (m_textureConfig.empty()) { // S'assurer que la configuration est faite une seule fois
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
int countRedstone = 0;

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

                        if (dist(rng) < 0.01f && height + 1 < CHUNK_HEIGHT - 1 && countRedstone == 0) {
                                mBlocks[x][height + 1][z] = BlockType::REDSTONE;
                                std::cout << "Placed redstone at (" << (mChunkX * CHUNK_SIZE + x) << ", " << (height + 1) << ", " << (mChunkZ * CHUNK_SIZE + z) << ")\n";
                                countRedstone++;
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

void Chunk::initializeTextureConfig() {
        // Helper lambda pour obtenir un index unique pour un chemin de texture
        auto getIndex = [](const std::string& path) -> int {
                if (m_pathToTextureIndex.find(path) == m_pathToTextureIndex.end()) {
                        m_pathToTextureIndex[path] = m_nextTextureIndex++;
                }
                return m_pathToTextureIndex[path];
        };

        // --- Configuration des Textures (selon votre demande) ---

        // BLOCK_TYPE::GRASS
        m_textureConfig[BlockType::GRASS] = {
                "./textures/grass-top.jpg",   // Top
                "./textures/dirt.png",        // Bottom
                "./textures/grass-side.png",  // Side
                ""
        };

        // BLOCK_TYPE::WOOD
        m_textureConfig[BlockType::WOOD] = {
                "./textures/wood-top.png",    // Top
                "./textures/wood-top.png",    // Bottom
                "./textures/wood-side.png",   // Side (J'utilise wood-side, plus logique que grass-side)
                ""
        };

        // BLOCK_TYPE::DIRT
        m_textureConfig[BlockType::DIRT] = {
                "./textures/dirt.png",
                "./textures/dirt.png",
                "./textures/dirt.png",
                ""
        };

        // BLOCK_TYPE::STONE (Utilisation de .png comme dans vos fichiers)
        m_textureConfig[BlockType::STONE] = {
                "./textures/stone.png",
                "./textures/stone.png",
                "./textures/stone.png",
                ""
        };

        // BLOCK_TYPE::REDSTONE
        m_textureConfig[BlockType::REDSTONE] = {
                "./textures/redstone.png",
                "./textures/redstone.png",
                "./textures/redstone.png",
                ""
        };

        // BLOCK_TYPE::LEAVES
        m_textureConfig[BlockType::LEAVES] = {
                "./textures/leaves.png",
                "./textures/leaves.png",
                "./textures/leaves.png",
                ""
        };

        // BLOCK_TYPE::TORCH
        m_textureConfig[BlockType::TORCH] = {
                "", "", "",
                "./textures/torch.png" // Texture spéciale
        };

        // --- Fin de la Configuration ---

        // Récupérer les index pour tous les chemins de texture
        for (const auto& pair : m_textureConfig) {
                getIndex(pair.second.top);
                getIndex(pair.second.bottom);
                getIndex(pair.second.side);
                getIndex(pair.second.special);
        }
        if (m_pathToTextureIndex.count("")) { // Supprimer le chemin vide s'il existe
                m_pathToTextureIndex.erase("");
        }
}

void World::localToChunkCoords(int worldX, int worldY, int worldZ,
                               int& chunkX, int& chunkZ,
                               int& localX, int& localY, int& localZ) const
{
        chunkX = (worldX >= 0) ? worldX / Chunk::CHUNK_SIZE
                                : (worldX + 1) / Chunk::CHUNK_SIZE - 1;
        chunkZ = (worldZ >= 0) ? worldZ / Chunk::CHUNK_SIZE
                                : (worldZ + 1) / Chunk::CHUNK_SIZE - 1;

        localX = worldX - chunkX * Chunk::CHUNK_SIZE;
        localZ = worldZ - chunkZ * Chunk::CHUNK_SIZE;
        localY = worldY; // you store Y directly
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
        // Torch is solid (collidable / occupies space logically) but not a full cube mesh
        if (type == BlockType::TORCH) return false;
        return type != BlockType::AIR;
}

bool Chunk::shouldRenderFace(int x, int y, int z, int nx, int ny, int nz) const {
        BlockType neighbor = getBlock(x + nx, y + ny, z + nz);
        BlockType current  = getBlock(x, y, z);

        // Don't render if neighbor is a full block (except for leaves)
        if (isFullBlock(neighbor) && neighbor != BlockType::LEAVES) {
                return false;
        }

        // Always render leaves faces
        if (current == BlockType::LEAVES) {
                return neighbor != BlockType::LEAVES;
        }

        return true;
}

// Dans src/Cube.cpp (Remplacer la version existante)
glm::vec3 Chunk::getTextureCoords(BlockType type, const glm::vec3& normal, int corner) {
        // 1. Déterminer le chemin de texture basé sur BlockType et Normal
        std::string path = "";
        if (m_textureConfig.count(type)) {
                const auto& config = m_textureConfig.at(type);

                if (glm::abs(normal.y) > 0.5f) {
                        path = (normal.y > 0.5f) ? config.top : config.bottom;
                } else {
                        path = config.side;
                }
        }

        // 2. Récupérer l'index de texture
        int textureIndex = -1;
        if (m_pathToTextureIndex.count(path) && path != "") {
                textureIndex = m_pathToTextureIndex.at(path);
        }

        // 3. Déterminer les UV standards (0.0 à 1.0)
        glm::vec2 uv;
        switch (corner) {
                case 0: uv = glm::vec2(0.0f, 0.0f); break;
                case 1: uv = glm::vec2(1.0f, 0.0f); break;
                case 2: uv = glm::vec2(1.0f, 1.0f); break;
                case 3: uv = glm::vec2(0.0f, 1.0f); break;
                default: uv = glm::vec2(0.0f, 0.0f); break;
        }

        // Retourner vec3(u, v, textureIndex)
        return glm::vec3(uv.x, uv.y, (float)textureIndex);
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
                                if (type == BlockType::AIR) continue;

                                if (type == BlockType::TORCH) {
                                        addTorchMesh(x, y, z);
                                        continue;
                                }

                                if (!isFullBlock(type)) continue;
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

        // texCoords now contain (u, v, textureIndex) so we must read 3 floats
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)offsetof(CubeVertex, texCoords));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
}

void Chunk::addTorchMesh(int x, int y, int z) {
        glm::vec3 chunkWorldPos = glm::vec3(mChunkX * CHUNK_SIZE, 0, mChunkZ * CHUNK_SIZE);
        glm::vec3 worldPos = chunkWorldPos + glm::vec3(x + 0.5f, y, z + 0.5f); // center in block

        float radius = 0.15f;
        float baseY  = 0.0f;
        float topY   = 0.7f;

        struct Quad { glm::vec3 p[4]; glm::vec3 normal; };

        Quad quads[2];

        // Diagonal in XZ: (-r,0,-r) .. (r,0,r)
        quads[0].p[0] = worldPos + glm::vec3(-radius, baseY, -radius);
        quads[0].p[1] = worldPos + glm::vec3( radius, baseY,  radius);
        quads[0].p[2] = worldPos + glm::vec3( radius, topY,  radius);
        quads[0].p[3] = worldPos + glm::vec3(-radius, topY, -radius);
        quads[0].normal = glm::normalize(glm::vec3(1, 0, 1));

        // Other diagonal: (-r,0,r) .. (r,0,-r)
        quads[1].p[0] = worldPos + glm::vec3(-radius, baseY,  radius);
        quads[1].p[1] = worldPos + glm::vec3( radius, baseY, -radius);
        quads[1].p[2] = worldPos + glm::vec3( radius, topY, -radius);
        quads[1].p[3] = worldPos + glm::vec3(-radius, topY,  radius);
        quads[1].normal = glm::normalize(glm::vec3(1, 0,-1));

        int textureIndex = -1;
        const auto& config = m_textureConfig.at(BlockType::TORCH);
        if (m_pathToTextureIndex.count(config.special)) {
                textureIndex = m_pathToTextureIndex.at(config.special);
        }
        float texIdx = (float)textureIndex;

        for (int q = 0; q < 2; ++q) {
                glm::vec3 n = quads[q].normal;

                CubeVertex v[6];

                // Triangle 1: 0,1,2
                v[0].position = quads[q].p[0];
                v[0].normal   = n;
                v[0].texCoords = glm::vec3(0.0f, 0.0f, texIdx); // Ajout de l'index

                v[1].position = quads[q].p[1];
                v[1].normal   = n;
                v[1].texCoords = glm::vec3(1.0f, 0.0f, texIdx); // Ajout de l'index

                v[2].position = quads[q].p[2];
                v[2].normal   = n;
                v[2].texCoords = glm::vec3(1.0f, 1.0f, texIdx); // Ajout de l'index

                // Triangle 2: 0,2,3
                v[3].position = quads[q].p[0];
                v[3].normal   = n;
                v[3].texCoords = glm::vec3(0.0f, 0.0f, texIdx); // Ajout de l'index

                v[4].position = quads[q].p[2];
                v[4].normal   = n;
                v[4].texCoords = glm::vec3(1.0f, 1.0f, texIdx); // Ajout de l'index

                v[5].position = quads[q].p[3];
                v[5].normal   = n;
                v[5].texCoords = glm::vec3(0.0f, 1.0f, texIdx); // Ajout de l'index

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

Chunk* World::findChunk(int chunkX, int chunkZ) const {
        for (auto chunk : mChunks) {
                glm::vec3 pos = chunk->getWorldPosition(); // returns (chunkX*CHUNK_SIZE,0,chunkZ*CHUNK_SIZE)
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
        chunk->buildMesh();            // rebuild only that chunk
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
                                                // Block center (offset by 0.5 since blocks are 1 unit cubes centered at integer coords)
                                                glm::vec3 blockCenter = chunkPos + glm::vec3(x + 0.5f, y + 0.5f, z + 0.5f);
                                                // Add one light at the center of each face (6 faces)
                                                std::cout << "Redstone block at (" << blockCenter.x << ", " << blockCenter.y << ", " << blockCenter.z << ")\n";
                                                const float faceOffset = 0.51f; // slightly outside the cube face
                                                // +X face
                                                positions.push_back(blockCenter + glm::vec3(faceOffset, 0.0f, 0.0f));
                                                // -X face
                                                positions.push_back(blockCenter + glm::vec3(-faceOffset, 0.0f, 0.0f));
                                                // +Y face (top)
                                                positions.push_back(blockCenter + glm::vec3(0.0f, faceOffset, 0.0f));
                                                // -Y face (bottom)
                                                positions.push_back(blockCenter + glm::vec3(0.0f, -faceOffset, 0.0f));
                                                // +Z face
                                                positions.push_back(blockCenter + glm::vec3(0.0f, 0.0f, faceOffset));
                                                // -Z face
                                                positions.push_back(blockCenter + glm::vec3(0.0f, 0.0f, -faceOffset));
                                        }
                                }
                        }
                }
        }

        return positions;
}