#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <GL/glew.h>
#include <string>
using std::string;

class Texture2D {
    public:
        Texture2D();
        virtual ~Texture2D();

        bool loadTexture(const string& fileName, bool generateMipMaps = true);
        bool loadFromMemory(int width, int height, const unsigned char* data, bool generateMipMaps = false);
        void bind(GLuint textUnit = 0) const;
        void unbind(GLuint textUnit = 0) const;
    private:
        Texture2D(const Texture2D& rhs) {}
        Texture2D& operator = (const Texture2D& rhs) {}
        GLuint mTexture;
};

#endif