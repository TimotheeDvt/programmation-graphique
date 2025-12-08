#version 330 core

flat in int TexIndex;
in vec2 TexCoord;
uniform sampler2D diffuseMaps[MAX_BLOCK_TEXTURES]

void main() {
    if (TexIndex == 9 || TexIndex == 8 || TexIndex == 4) {
        discard;
    }

    if (TexIndex == 7) {
        vec4 texColor = texture(diffuseMaps[TexIndex], TexCoord);
        if (texColor.a < 0.1) {
            discard; // Jette le fragment si son alpha est trop bas (crée des trous dans l'ombre)
        }
    }
}