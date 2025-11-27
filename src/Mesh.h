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
        glm::vec3 emissive = glm::vec3(0.0f); // Ke

        float shininess = 32.0f;
        float transparency = 1.0f; // d
        float opticalDensity = 1.5f; // Ni (index of refraction)
        int illuminationModel = 2; // illum

        // Textures
        std::string diffuseMap;      // map_Kd
        std::string specularMap;     // map_Ks
        std::string normalMap;       // map_Bump / bump
        std::string roughnessMap;    // map_Ns (nouveau)
        std::string metallicMap;     // map_refl (nouveau)
        std::string emissiveMap;     // map_Ke

        // Paramètres de bump mapping
        float bumpMultiplier = 1.0f; // -bm parameter
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