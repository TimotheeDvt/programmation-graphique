#ifndef CUBE_H
#define CUBE_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "Block.h"

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

#endif // CUBE_H