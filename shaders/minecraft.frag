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

#define MAX_POINT_LIGHTS 64
uniform PointLight pointLights[MAX_POINT_LIGHTS];

flat in int TexIndex;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform DirectionalLight dirLight;
uniform int numPointLights;

uniform Material material;
uniform vec3 viewPos;
uniform int u_torchTexIndex;   // NEW: For self-illumination
uniform int u_leavesTexIndex;  // NEW: For shadow exclusion
uniform int u_glassTexIndex;   // NEW: For shadow exclusion
uniform sampler2D shadowMap;   // NEW: Directional light shadow map

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor);

void main() {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec2 uv = TexCoord;

        // 1. Sample the full texture including alpha (vec4)
        vec4 texData;
        if (TexIndex >= 0 && TexIndex < MAX_BLOCK_TEXTURES) {
                // Change: Sample the full vec4 from the diffuse map
                texData = texture(material.diffuseMaps[TexIndex], uv);
        } else {
                // Magenta fallback
                texData = vec4(1.0f, 0.0f, 1.0f, 1.0f);
        }

        // 2. Alpha Testing: Discard fragment if alpha is too low
        // This is essential for non-cubic geometry like the torch (and leaves).
        if (texData.a < 0.1) {
            discard;
        }

        // Now use the RGB part for lighting calculation
        vec3 texColor = texData.rgb;

        vec3 result;
        // START MODIFICATION FOR TORCH SELF-ILLUMINATION
        // Torches are self-illuminated and should not be affected by external light.
        if (TexIndex == u_torchTexIndex) {
            result = texColor; // Full brightness based on texture color
        } else {
            // // Directional lighting (sun/moon)
            result = calcDirectionalLight(dirLight, norm, viewDir, texColor);
            // // Point lights (redstone and torch)
            for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; i++) {
                result += calcPointLight(pointLights[i], norm, FragPos, viewDir, texColor);
            }
        }

        // Final color is the calculated light * the sampled color, with full opacity.
        FragColor = vec4(result, 1.0);
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor) {
        vec3 lightDir = normalize(-light.direction);
        // Ambient
        vec3 ambient = light.ambient * material.ambient * texColor;

        // START MODIFICATION FOR SHADOWS
        float shadow = 1.0;
        // Skip shadow calculation for transparent/non-occluding blocks (Leaves, Glass)
        if (TexIndex != u_leavesTexIndex && TexIndex != u_glassTexIndex) {
            vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
            // Transform from NDC [-1, 1] to texture space [0, 1]
            projCoords = projCoords * 0.5 + 0.5;

            // Check if fragment is within the light's frustum and visible
            if(projCoords.z <= 1.0 && projCoords.x >= 0.0 && projCoords.x <= 1.0 && projCoords.y >= 0.0 && projCoords.y <= 1.0) {
                float currentDepth = projCoords.z;
                // Get the closest depth from the light's perspective
                float closestDepth = texture(shadowMap, projCoords.xy).r;

                // Use a small bias to prevent "shadow acne"
                float bias = 0.005;
                shadow = currentDepth - bias > closestDepth ? 0.0 : 1.0;
            }
        }
        // END MODIFICATION FOR SHADOWS

        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * texColor * shadow; // Apply shadow factor

        // Specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), material.shininess);
        vec3 specular = light.specular * material.specular * spec * shadow; // Apply shadow factor
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