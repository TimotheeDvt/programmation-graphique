#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D guiTexture;
uniform vec3 tintColor;

void main() {
    FragColor = texture(guiTexture, TexCoord) * vec4(tintColor, 1.0);

    if (FragColor.a < 0.1) {
        discard;
    }
}