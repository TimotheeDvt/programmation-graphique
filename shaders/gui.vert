#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform bool is3D;

out vec2 TexCoord;

void main() {
TexCoord = aTexCoord;
    if (is3D) {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    } else {
        gl_Position = projection * model * vec4(aPos, 1.0);
    }
}