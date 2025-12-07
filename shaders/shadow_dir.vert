#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aTexCoord;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

flat out int TexIndex;

void main() {
    TexIndex = int(aTexCoord.z);
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}