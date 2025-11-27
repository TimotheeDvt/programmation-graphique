#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN; // Tangent space matrix pour normal mapping

void main() {
        Normal = mat3(transpose(inverse(model))) * aNormal;
        FragPos = vec3(model * vec4(aPos, 1.0f));
        TexCoords = aTexCoord;

        // Calculer la matrice TBN pour normal mapping
        // On génère les tangentes à partir des normales
        vec3 T, B;
        vec3 N = normalize(Normal);

        // Choisir un vecteur arbitraire qui n'est pas parallèle à N
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        T = normalize(cross(up, N));
        B = cross(N, T);

        TBN = mat3(T, B, N);

        gl_Position = projection * view * model * vec4(aPos, 1.0);
}