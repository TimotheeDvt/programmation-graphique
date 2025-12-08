#version 330 core

#define MAX_BLOCK_TEXTURES 16

flat in int TexIndex;
in vec2 TexCoord;
uniform sampler2D diffuseMaps[MAX_BLOCK_TEXTURES];

void main() {
    if (TexIndex == 8 || TexIndex == 4) {
        discard; // discard redstone and torches
    }

    if (TexIndex == 7 || TexIndex == 9) { // Leaves and Glass
        vec4 texColor = texture(diffuseMaps[TexIndex], TexCoord);
        if (texColor.a < 0.1) {
            discard;
        }
    }
}