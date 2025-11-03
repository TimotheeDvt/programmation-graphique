#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <map>
#include <GL/glew.h>
#include "glm/glm.hpp"

struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
};

struct Material {
        glm::vec3 ambient = glm::vec3(0.2f);
        glm::vec3 diffuse = glm::vec3(0.8f);
        glm::vec3 specular = glm::vec3(1.0f);
        float shininess = 32.0f;
        float transparency = 1.0f;

        std::string diffuseMap;
        std::string specularMap;
        std::string normalMap;
};

struct SubMesh {
        std::string materialName;
        unsigned int startIndex;
        unsigned int indexCount;
        int smoothingGroup;
};

class Mesh {
        public:
                Mesh();
                ~Mesh();

                bool loadObj(const std::string& filename);
                bool loadMaterial(const std::string& filename);

                void draw();
                void drawSubMesh(const std::string& materialName);

                const std::map<std::string, SubMesh>& getSubMeshes() const;
                const std::map<std::string, Material>& getMaterials() const;
                const Material* getMaterial(const std::string& name) const;

        private:
                void initBuffers();

                bool mLoaded;
                std::vector<Vertex> mVertices;
                GLuint mVBO, mVAO;

                std::string mMaterialLibrary;
                std::map<std::string, Material> mMaterials;
                std::map<std::string, SubMesh> mSubMeshes;
};

#endif