#include "World.h"
#include "Chunk.h"
#include <chrono>
#include <iostream>

World::World() {}

World::~World() {
        for (auto chunk : mChunks) {
                delete chunk;
        }
        mChunks.clear();
}

void World::generate(int renderDistance, long long seed) {
        if (seed == -1) {
                m_seed = std::chrono::system_clock::now().time_since_epoch().count();
        } else {
                m_seed = seed;
        }
        std::cout << "World generated with seed: " << m_seed << std::endl;

        // Clear old chunks if any
        for (auto chunk : mChunks) {
                delete chunk;
        }
        mChunks.clear();

        if (renderDistance == 1) {
                Chunk* chunk = new Chunk(0, 0);
                chunk->generate(m_seed);
                chunk->buildMesh();
                mChunks.push_back(chunk);
                return;
        }

        for (int x = -renderDistance; x <= renderDistance; x++) {
                for (int z = -renderDistance; z <= renderDistance; z++) {
                        Chunk* chunk = new Chunk(x, z);
                        chunk->generate(m_seed);
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
                                                // positions.push_back(basePos + glm::vec3(0.0f, -1.01f, 0.0f));
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
                                                        + glm::vec3(x + 0.2f, y + 0.2f, z)
                                                );
                                                positions.push_back(
                                                        chunkPos
                                                        + glm::vec3(x - 0.2f, y + 0.2f, z)
                                                );
                                                positions.push_back(
                                                        chunkPos
                                                        + glm::vec3(x, y + 0.2f, z + 0.2f)
                                                );
                                                positions.push_back(
                                                        chunkPos
                                                        + glm::vec3(x, y + 0.2f, z - 0.2f)
                                                );
                                        }
                                }
                        }
                }
        }
        return positions;
}

void World::localToChunkCoords(int worldX, int worldY, int worldZ, int& chunkX, int& chunkZ, int& localX, int& localY, int& localZ) const {
    chunkX = (worldX >= 0) ? worldX / Chunk::CHUNK_SIZE : (worldX + 1) / Chunk::CHUNK_SIZE - 1;
    chunkZ = (worldZ >= 0) ? worldZ / Chunk::CHUNK_SIZE : (worldZ + 1) / Chunk::CHUNK_SIZE - 1;
    localX = worldX - chunkX * Chunk::CHUNK_SIZE;
    localZ = worldZ - chunkZ * Chunk::CHUNK_SIZE;
    localY = worldY;
}

BlockType World::getBlock(int x, int y, int z) const {
    int chunkX, chunkZ, localX, localY, localZ;
    localToChunkCoords(x, y, z, chunkX, chunkZ, localX, localY, localZ);
    Chunk* chunk = findChunk(chunkX, chunkZ);
    if (!chunk) return BlockType::AIR;
    return chunk->getBlock(localX, localY, localZ);
}