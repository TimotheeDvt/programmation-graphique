#version 330 core
layout(location = 0) in vec3 aPos; // <-- This is the correct position variable
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTexCoord; // Input est le vec3 (u, v, index)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix; // NEW uniform
out vec4 FragPosLightSpace;    // NEW output

flat out int TexIndex; // Index de texture non interpolé
out vec2 TexCoord;     // Coordonnées UV interpolées
out vec3 Normal;
out vec3 FragPos;

void main() {
        // Correct : Lire les coordonnées UV (xy) et l'index (z) du vec3 d'entrée
        TexCoord = aTexCoord.xy;
        TexIndex = int(aTexCoord.z);

        FragPos = vec3(model * vec4(aPos, 1.0f));
        Normal = mat3(transpose(inverse(model))) * aNormal;

        // FIX: Replace 'position' with 'aPos'
        FragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0);

        gl_Position = projection * view * model * vec4(aPos, 1.0);
}