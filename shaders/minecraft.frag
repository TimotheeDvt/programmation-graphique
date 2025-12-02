#version 330 core

#define MAX_BLOCK_TEXTURES 16
struct Material {
        vec3 ambient;
        sampler2D diffuseMaps[MAX_BLOCK_TEXTURES];
        vec3 specular;
        float shininess;
};

struct DirectionalLight {
        vec3 direction;
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
};

struct PointLight {
        vec3 position;
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;

        float constant;
        float linear;
        float exponant;
};

#define MAX_POINT_LIGHTS 50

in vec3 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform DirectionalLight dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights;

uniform Material material;
uniform vec3 viewPos;

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor);

void main() {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);

        int texIndex = int(TexCoord.z);
        vec2 uv = TexCoord.xy;

        vec3 texColor;
        // Sélection de la bonne texture
        if (texIndex >= 0 && texIndex < MAX_BLOCK_TEXTURES) {
                texColor = vec3(texture(material.diffuseMaps[texIndex], uv)); // Utilisation du tableau
        } else {
            // Couleur de débogage (e.g. magenta) si l'index est invalide
                texColor = vec3(1.0f, 0.0f, 1.0f);
        }

        // Point lights (redstone)
        vec3 result = calcDirectionalLight(dirLight, norm, viewDir, texColor);
        for (int i = 0; i < numPointLights && i < MAX_BLOCK_TEXTURES; i++) {
                result += calcPointLight(pointLights[i], norm, FragPos, viewDir, texColor);
        }

        FragColor = vec4(result, 1.0);
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor) {
        vec3 lightDir = normalize(-light.direction);

        // Ambient
        vec3 ambient = light.ambient * material.ambient * texColor;

        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * texColor;

        // Specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), material.shininess);
        vec3 specular = light.specular * material.specular * spec;

        return ambient + diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor) {
        vec3 lightDir = normalize(light.position - fragPos);

        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * texColor;

        // Specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), material.shininess);
        vec3 specular = light.specular * material.specular * spec;

        // Attenuation
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.exponant * (distance * distance));

        diffuse *= attenuation;
        specular *= attenuation;

        return diffuse + specular;
}