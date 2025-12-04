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

struct SpotLight {
        vec3 position;
        vec3 direction;
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
        float constant;
        float linear;
        float exponant;
        float cosInnerCone;
        float cosOuterCone;
};

#define MAX_POINT_LIGHTS 32
#define MAX_SPOT_LIGHTS 8
#define MAX_CHUNKS_RENDERED 81 // (2*4+1)^2 = 81 chunks max

// Global Uniforms (Lights and Camera)
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform sampler2D shadowMap;

// Vertex Shader Inputs
flat in int TexIndex;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 LightSpacePos;

// Fragment Shader Outputs
out vec4 FragColor;

// Global Uniforms (World and Voxel Data)
uniform DirectionalLight dirLight;
uniform int numPointLights;
uniform int numSpotLights;
uniform Material material;
uniform vec3 viewPos;
uniform int chunkSize; // CORRIGÉ: Déclaré globalement
uniform int chunkHeight; // CORRIGÉ: Déclaré globalement
uniform int numChunks; // CORRIGÉ: Déclaré globalement
uniform ivec3 chunkWorldOffsets[MAX_CHUNKS_RENDERED]; // CORRIGÉ: Déclaré globalement avec taille fixe

// SSBO for Block Data
uniform isamplerBuffer voxelDataSampler;

// Function Prototypes
vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor);
vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor);
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

// ====================================================================
// NOUVELLES FONCTIONS D'OMBRE POUR LUMIÈRES PONCTUELLES/SPOTLIGHTS
// ====================================================================

// NEW: Helper function to get block type from world position
int get_block_type_from_world_pos(vec3 worldPos) {
    ivec3 pos = ivec3(floor(worldPos));
    int chunkVoxelCount = chunkSize * chunkHeight * chunkSize;

    // 1. Find the chunk that contains the block
    int chunkIndex = -1;
    for(int i = 0; i < numChunks; ++i) {
        ivec3 chunkStart = chunkWorldOffsets[i];
        ivec3 chunkEnd = chunkStart + ivec3(chunkSize, chunkHeight, chunkSize);

        if (pos.x >= chunkStart.x && pos.x < chunkEnd.x &&
            pos.y >= chunkStart.y && pos.y < chunkEnd.y &&
            pos.z >= chunkStart.z && pos.z < chunkEnd.z) {
            chunkIndex = i;
            break;
        }
    }

    if (chunkIndex == -1) {
        return 0; // AIR si hors zone rendue
    }

    // 2. Calculate local coordinates
    ivec3 chunkStart = chunkWorldOffsets[chunkIndex];
    ivec3 localPos = pos - chunkStart;
    
    if (localPos.y < 0 || localPos.y >= chunkHeight) {
        return 0; // AIR
    }

    // 3. Calculate 1D TBO index (x*(CH*CS) + y*CS + z)
    int voxelIndex = localPos.z + (localPos.y * chunkSize) + (localPos.x * chunkSize * chunkHeight);

    // 4. Calculate final TBO index
    int tboIndex = chunkIndex * chunkVoxelCount + voxelIndex;

    // Fetch the integer value from the TBO
    return texelFetch(voxelDataSampler, tboIndex).r; // .r car c'est un GL_R32I
}

// Re-implementation utilisant les vrais types de blocs
bool is_solid_block_heuristic(vec3 worldPos) {
    int blockType = get_block_type_from_world_pos(worldPos);

    // BlockType enum: AIR=0, TORCH=7.
    // Tous les autres sont considérés comme solides pour l'occlusion.
    return blockType != 0 && blockType != 7;
}

// Ray Marching simplifié du fragment vers la lumière
float VoxelShadowCheck(vec3 fragPos, vec3 lightPos) {
    if (length(lightPos - fragPos) < 1.0) return 1.0;

    vec3 lightDir = normalize(lightPos - fragPos);
    float maxDist = length(lightPos - fragPos) - 0.05; // Arrêter juste avant la lumière
    float step = 0.2f; // Taille de pas (ajustable)
    
    // On commence après le fragment pour éviter l'auto-occlusion
    for (float dist = 0.5; dist < maxDist; dist += step) {
        vec3 rayPos = fragPos + lightDir * dist;
        // On vérifie si un bloc sur le chemin est solide
        if (is_solid_block_heuristic(rayPos)) {
            return 0.0; // Ombre complète
        }
    }
    return 1.0; // Pas d'ombre
}

void main() {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec2 uv = TexCoord;

        vec4 texData;
        if (TexIndex >= 0 && TexIndex < MAX_BLOCK_TEXTURES) {
                texData = texture(material.diffuseMaps[TexIndex], uv);
        } else {
                texData = vec4(1.0f, 0.0f, 1.0f, 1.0f);
        }

        if (texData.a < 0.1) {
            discard;
        }

        vec3 texColor = texData.rgb;

        // Start with only ambient from DirLight for global lighting
        vec3 result = dirLight.ambient * material.ambient * texColor;

        result += calcDirectionalLight(dirLight, norm, viewDir, texColor);

        for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; i++) {
                result += calcPointLight(pointLights[i], norm, FragPos, viewDir, texColor);
        }

        for (int i = 0; i < numSpotLights && i < MAX_SPOT_LIGHTS; i++) {
                result += calcSpotLight(spotLights[i], norm, FragPos, viewDir, texColor);
        }

        FragColor = vec4(result, 1.0);
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor) {
        vec3 lightDir = normalize(-light.direction);
        
        vec3 ambient = vec3(0.0);

        // Shadow Mapping pour la lumière directionnelle (soleil)
        float shadow = ShadowCalculation(LightSpacePos, normal, lightDir);
        
        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * texColor * shadow; // Applique l'ombre

        // Specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), material.shininess);
        vec3 specular = light.specular * material.specular * spec * shadow; // Applique l'ombre
        
        return ambient + diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor) {
        vec3 lightDir = normalize(light.position - fragPos);
        float distance = length(light.position - fragPos);

        // Ombre Voxel Raycasting
        float shadow = VoxelShadowCheck(fragPos, light.position);
        
        vec3 ambient = vec3(0.0); 

        // Diffuse 
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * texColor;

        // Specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), material.shininess);
        vec3 specular = light.specular * material.specular * spec;

        // Attenuation
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.exponant * (distance * distance));
        
        diffuse *= attenuation * shadow; // Applique l'ombre
        specular *= attenuation * shadow; // Applique l'ombre
        
        return ambient + diffuse + specular;
}

vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor) {
        vec3 lightDir = normalize(light.position - fragPos);
        vec3 spotDir = normalize(light.direction);
        float distance = length(light.position - fragPos);

        // Calculate spotlight intensity
        float cosDir = dot(-lightDir, spotDir);
        float spotIntensity = smoothstep(light.cosOuterCone, light.cosInnerCone, cosDir);

        // Ombre Voxel Raycasting
        float shadow = VoxelShadowCheck(fragPos, light.position);

        vec3 ambient = vec3(0.0);
        
        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * texColor;

        // Specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), material.shininess);
        vec3 specular = light.specular * material.specular * spec;

        // Attenuation
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.exponant * (distance * distance));
        
        // Apply spot intensity, attenuation and shadow
        diffuse *= attenuation * spotIntensity * shadow; // Applique l'ombre
        specular *= attenuation * spotIntensity * shadow; // Applique l'ombre

        return ambient + diffuse + specular;
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // 1. Division de perspective
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // 2. Transformation vers l'espace [0, 1]
    projCoords = projCoords * 0.5 + 0.5;

    // 3. Vérification de la portée (si hors champ, pas d'ombre)
    if(projCoords.z > 1.0)
        return 1.0;

    // 4. Lecture de la profondeur la plus proche (Depth Map)
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // 5. Profondeur du fragment actuel
    float currentDepth = projCoords.z;

    // 6. Bias pour éviter le Shadow Acne
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.002);

    // 7. Comparaison de la profondeur
    float shadow = currentDepth - bias > closestDepth ?
    0.0 : 1.0;

    return shadow;
}