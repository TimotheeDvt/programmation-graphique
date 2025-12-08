#include "Mesh.h"
#include <iostream>
#include <sstream>
#include <fstream>

std::vector<std::string> split(std::string s, std::string t) {
        std::vector<std::string> res;
        while (1) {
                int pos = s.find(t);
                if (pos == -1) {
                        res.push_back(s);
                        break;
                }
                res.push_back(s.substr(0, pos));
                s = s.substr(pos+1, s.size() - pos - 1);
        }
        return res;
}

Mesh::Mesh() : mLoaded(false) {
}

Mesh::~Mesh() {
        glDeleteVertexArrays(1, &mVAO);
        glDeleteBuffers(1, &mVBO);
}

bool Mesh::loadObj(const std::string& filename) {
        std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
        std::vector<glm::vec3> tempVertices;
        std::vector<glm::vec3> tempNormals;
        std::vector<glm::vec2> tempUVs;

        // Variables pour gérer les submeshes par matériau
        std::string currentMaterial = "default";
        std::string currentObject = "";
        std::string currentGroup = "";
        int currentSmoothingGroup = 0;

        if (filename.find(".obj") != std::string::npos) {
                std::ifstream fin(filename, std::ios::in);
                if (!fin) {
                        std::cerr << "Cannot open " << filename << std::endl;
                        return false;
                }

                std::string lineBuffer;
                while (std::getline(fin, lineBuffer)) {
                        // Ignorer les lignes vides et les commentaires
                        if (lineBuffer.empty() || lineBuffer[0] == '#')
                                continue;

                        std::stringstream ss(lineBuffer);
                        std::string cmd;
                        ss >> cmd;

                        if (cmd == "v") {
                                glm::vec3 vertex;
                                ss >> vertex.x >> vertex.y >> vertex.z;
                                min.x = std::min(min.x, vertex.x);
                                min.y = std::min(min.y, vertex.y);
                                min.z = std::min(min.z, vertex.z);
                                max.x = std::max(max.x, vertex.x);
                                max.y = std::max(max.y, vertex.y);
                                max.z = std::max(max.z, vertex.z);
                                tempVertices.push_back(vertex);
                        } else if (cmd == "vt") {
                                glm::vec2 uv;
                                ss >> uv.x >> uv.y;
                                tempUVs.push_back(uv);
                        } else if (cmd == "vn") {
                                glm::vec3 normal;
                                ss >> normal.x >> normal.y >> normal.z;
                                normal = glm::normalize(normal);
                                tempNormals.push_back(normal);
                        } else if (cmd == "mtllib") {
                                std::string mtlFile;
                                ss >> mtlFile;
                                mMaterialLibrary = mtlFile;
                                // Extraire le chemin du fichier OBJ
                                size_t lastSlash = filename.find_last_of("/\\");
                                std::string objPath = (lastSlash != std::string::npos) ? filename.substr(0, lastSlash + 1) : "";
                                loadMaterial(objPath + mtlFile);
                        } else if (cmd == "usemtl") {
                                std::string materialName;
                                ss >> materialName;
                                currentMaterial = materialName;
                        } else if (cmd == "o") {
                                std::string objectName;
                                std::getline(ss, objectName);
                                if (!objectName.empty() && objectName[0] == ' ')
                                        objectName = objectName.substr(1);
                                currentObject = objectName;
                        } else if (cmd == "g") {
                                std::string groupName;
                                std::getline(ss, groupName);
                                if (!groupName.empty() && groupName[0] == ' ')
                                        groupName = groupName.substr(1);
                                currentGroup = groupName;
                        } else if (cmd == "s") {
                                std::string smoothing;
                                ss >> smoothing;
                                if (smoothing == "off") {
                                        currentSmoothingGroup = 0;
                                } else {
                                        currentSmoothingGroup = std::stoi(smoothing);
                                }
                        } else if (cmd == "f") {
                                std::string faceData;
                                std::vector<std::string> faceVertices;

                                // Lire tous les sommets de la face
                                while (ss >> faceData) {
                                        faceVertices.push_back(faceData);
                                }

                                // Gérer les faces avec plus de 3 sommets (triangulation en éventail)
                                if (faceVertices.size() >= 3) {
                                        // Stocker les indices de début de ce sous-mesh
                                        if (mSubMeshes.find(currentMaterial) == mSubMeshes.end()) {
                                                SubMesh subMesh;
                                                subMesh.materialName = currentMaterial;
                                                subMesh.startIndex = vertexIndices.size();
                                                subMesh.indexCount = 0;
                                                subMesh.smoothingGroup = currentSmoothingGroup;
                                                mSubMeshes[currentMaterial] = subMesh;
                                        }

                                        for (size_t i = 1; i < faceVertices.size() - 1; i++) {
                                                // Triangle: sommet 0, sommet i, sommet i+1
                                                std::string vertices[3] = {
                                                        faceVertices[0],
                                                        faceVertices[i],
                                                        faceVertices[i + 1]
                                                };

                                                for (int j = 0; j < 3; j++) {
                                                        std::vector<std::string> data = split(vertices[j], "/");

                                                        // Vertex index (toujours présent)
                                                        if (data.size() > 0 && !data[0].empty()) {
                                                                int vertexIndex = std::stoi(data[0]);
                                                                // Gérer les indices négatifs (relatifs à la fin)
                                                                if (vertexIndex < 0)
                                                                        vertexIndex = tempVertices.size() + vertexIndex + 1;
                                                                vertexIndices.push_back(vertexIndex);
                                                        }

                                                        // Texture coordinate index (optionnel)
                                                        if (data.size() > 1 && !data[1].empty()) {
                                                                int uvIndex = std::stoi(data[1]);
                                                                if (uvIndex < 0)
                                                                        uvIndex = tempUVs.size() + uvIndex + 1;
                                                                uvIndices.push_back(uvIndex);
                                                        } else {
                                                                uvIndices.push_back(0); // Pas de coordonnées de texture
                                                        }

                                                        // Normal index (optionnel)
                                                        if (data.size() > 2 && !data[2].empty()) {
                                                                int normalIndex = std::stoi(data[2]);
                                                                if (normalIndex < 0)
                                                                        normalIndex = tempNormals.size() + normalIndex + 1;
                                                                normalIndices.push_back(normalIndex);
                                                        } else {
                                                                normalIndices.push_back(0); // Pas de normale
                                                        }
                                                }

                                                mSubMeshes[currentMaterial].indexCount += 3;
                                        }
                                }
                        }
                }

                fin.close();

                // Validation des données
                if (tempVertices.empty()) {
                        return false;
                }

                // Calculer les normales si elles sont absentes
                bool needToGenerateNormals = tempNormals.empty();
                if (needToGenerateNormals) {
                        tempNormals.resize(tempVertices.size(), glm::vec3(0.0f));

                        // Calculer les normales par face et les accumuler
                        for (size_t i = 0; i < vertexIndices.size(); i += 3) {
                                unsigned int i0 = vertexIndices[i] - 1;
                                unsigned int i1 = vertexIndices[i + 1] - 1;
                                unsigned int i2 = vertexIndices[i + 2] - 1;

                                glm::vec3 v0 = tempVertices[i0];
                                glm::vec3 v1 = tempVertices[i1];
                                glm::vec3 v2 = tempVertices[i2];

                                glm::vec3 edge1 = v1 - v0;
                                glm::vec3 edge2 = v2 - v0;
                                glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

                                tempNormals[i0] += normal;
                                tempNormals[i1] += normal;
                                tempNormals[i2] += normal;
                        }

                        // Normaliser toutes les normales
                        for (auto& normal : tempNormals) {
                                if (glm::length(normal) > 0.0f)
                                normal = glm::normalize(normal);
                        }
                }

                // Générer des UVs par défaut si absents
                if (tempUVs.empty()) {
                        tempUVs.resize(tempVertices.size(), glm::vec2(0.0f));
                }

                // Construire les vertices finaux
                for (size_t i = 0; i < vertexIndices.size(); i++) {
                        Vertex meshVertex;

                        // Position (toujours présente)
                        unsigned int vertexIndex = vertexIndices[i] - 1;
                        if (vertexIndex < tempVertices.size()) {
                                meshVertex.position = tempVertices[vertexIndex];
                        }

                        // Normale
                        if (needToGenerateNormals) {
                                meshVertex.normal = tempNormals[vertexIndex];
                        } else if (i < normalIndices.size() && normalIndices[i] > 0) {
                                unsigned int normalIndex = normalIndices[i] - 1;
                                if (normalIndex < tempNormals.size()) {
                                        meshVertex.normal = tempNormals[normalIndex];
                                }
                        } else {
                                meshVertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Normale par défaut
                        }

                        // Coordonnées de texture
                        if (i < uvIndices.size() && uvIndices[i] > 0) {
                                unsigned int uvIndex = uvIndices[i] - 1;
                                if (uvIndex < tempUVs.size()) {
                                        meshVertex.texCoords = tempUVs[uvIndex];
                                }
                        } else {
                                meshVertex.texCoords = glm::vec2(0.0f); // UV par défaut
                        }

                        mVertices.push_back(meshVertex);
                }

                // Créer et initialiser les buffers
                initBuffers();

                return (mLoaded = true);
        }

        return false;
}

bool Mesh::loadMaterial(const std::string& filename) {
        std::ifstream fin(filename, std::ios::in);
        if (!fin) {
                std::cerr << "Cannot open material file " << filename << std::endl;
                return false;
        }

        Material currentMaterial;
        std::string currentMaterialName = "";
        bool hasMaterial = false;

        std::string lineBuffer;
        while (std::getline(fin, lineBuffer)) {
                if (lineBuffer.empty() || lineBuffer[0] == '#')
                        continue;

                std::stringstream ss(lineBuffer);
                std::string cmd;
                ss >> cmd;

                if (cmd == "newmtl") {
                        // Sauvegarder le matériau précédent
                        if (hasMaterial && !currentMaterialName.empty()) {
                                mMaterials[currentMaterialName] = currentMaterial;
                        }

                        // Nouveau matériau
                        ss >> currentMaterialName;
                        currentMaterial = Material(); // Reset
                        hasMaterial = true;
                } else if (cmd == "Ka") {
                        ss >> currentMaterial.ambient.r >> currentMaterial.ambient.g >> currentMaterial.ambient.b;
                } else if (cmd == "Kd") {
                        ss >> currentMaterial.diffuse.r >> currentMaterial.diffuse.g >> currentMaterial.diffuse.b;
                } else if (cmd == "Ks") {
                        ss >> currentMaterial.specular.r >> currentMaterial.specular.g >> currentMaterial.specular.b;
                } else if (cmd == "Ke") {
                        ss >> currentMaterial.emissive.r >> currentMaterial.emissive.g >> currentMaterial.emissive.b;
                } else if (cmd == "Ns") {
                        ss >> currentMaterial.shininess;
                } else if (cmd == "d" || cmd == "Tr") {
                        ss >> currentMaterial.transparency;
                } else if (cmd == "Ni") {
                        ss >> currentMaterial.opticalDensity;
                } else if (cmd == "illum") {
                        ss >> currentMaterial.illuminationModel;
                } else if (cmd == "map_Kd") {
                        std::string texturePath;
                        std::getline(ss, texturePath);
                        if (!texturePath.empty() && texturePath[0] == ' ')
                                texturePath = texturePath.substr(1);
                        currentMaterial.diffuseMap = texturePath;
                } else if (cmd == "map_Ks") {
                        std::string texturePath;
                        std::getline(ss, texturePath);
                        if (!texturePath.empty() && texturePath[0] == ' ')
                                texturePath = texturePath.substr(1);
                        currentMaterial.specularMap = texturePath;
                } else if (cmd == "map_Bump" || cmd == "bump") {
                        // Parser les paramètres optionnels comme -bm
                        std::string param;
                        while (ss >> param) {
                                if (param == "-bm") {
                                        ss >> currentMaterial.bumpMultiplier;
                                } else {
                                        // C'est le chemin de la texture
                                        std::string restOfLine;
                                        std::getline(ss, restOfLine);
                                        currentMaterial.normalMap = param + restOfLine;
                                        if (!currentMaterial.normalMap.empty() && currentMaterial.normalMap[0] == ' ')
                                                currentMaterial.normalMap = currentMaterial.normalMap.substr(1);
                                        break;
                                }
                        }
                } else if (cmd == "map_Ns") {
                        std::string texturePath;
                        std::getline(ss, texturePath);
                        if (!texturePath.empty() && texturePath[0] == ' ')
                                texturePath = texturePath.substr(1);
                        currentMaterial.roughnessMap = texturePath;
                } else if (cmd == "map_refl") {
                        std::string texturePath;
                        std::getline(ss, texturePath);
                        if (!texturePath.empty() && texturePath[0] == ' ')
                                texturePath = texturePath.substr(1);
                        currentMaterial.metallicMap = texturePath;
                } else if (cmd == "map_Ke") {
                        std::string texturePath;
                        std::getline(ss, texturePath);
                        if (!texturePath.empty() && texturePath[0] == ' ')
                                texturePath = texturePath.substr(1);
                        currentMaterial.emissiveMap = texturePath;
                }
        }

        // Sauvegarder le dernier matériau
        if (hasMaterial && !currentMaterialName.empty()) {
                mMaterials[currentMaterialName] = currentMaterial;
        }

        fin.close();
        return true;
}

void Mesh::draw() {
        if(!mLoaded) return;

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLES, 0, mVertices.size());
        glBindVertexArray(0);
}

void Mesh::drawSubMesh(const std::string& materialName) {
        if(!mLoaded) return;

        auto it = mSubMeshes.find(materialName);
        if (it == mSubMeshes.end()) return;

        const SubMesh& subMesh = it->second;

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLES, subMesh.startIndex, subMesh.indexCount);
        glBindVertexArray(0);
}

const std::map<std::string, SubMesh>& Mesh::getSubMeshes() const {
        return mSubMeshes;
}

const std::map<std::string, Material>& Mesh::getMaterials() const {
        return mMaterials;
}

const Material* Mesh::getMaterial(const std::string& name) const {
        auto it = mMaterials.find(name);
        if (it != mMaterials.end()) {
                return &(it->second);
        }
        return nullptr;
}

void Mesh::initBuffers() {
        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &mVBO);

        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(Vertex), &mVertices[0], GL_STATIC_DRAW);

        // Vertex Positions
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        // Normals
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(1);

        // Texture Coords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texCoords)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
}