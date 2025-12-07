#version 330 core

// These values should match Constants.h
#define MAX_BLOCK_TEXTURES 16
#define MAX_POINT_LIGHTS 32
#define MAX_SPOT_LIGHTS 8

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

// Global Uniforms (Lights and Camera)
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

// NOUVEAU: Shadow Maps
uniform sampler2D dirShadowMap;
uniform samplerCube pointShadowMap;
uniform float pointFarPlane;
uniform sampler2D spotShadowMaps[MAX_SPOT_LIGHTS];
uniform mat4 spotLightSpaceMatrices[MAX_SPOT_LIGHTS];

// Vertex Shader Inputs
flat in int TexIndex;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 LightSpacePos;

// Fragment Shader Outputs
out vec4 FragColor;
// Global Uniforms (World and Light Counts)
uniform DirectionalLight dirLight;
uniform int numPointLights;
uniform int numSpotLights;
uniform Material material;
uniform vec3 viewPos;


// Function Prototypes
vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor, samplerCube shadowMap, float farPlane);
vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor, sampler2D shadowMap, mat4 lightSpaceMatrix);
float DirShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);
float PointShadowCalculation(vec3 fragPos, vec3 lightPos, samplerCube shadowMap, float farPlane);
float SpotShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2D shadowMap);


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
                result += calcPointLight(pointLights[i], norm, FragPos, viewDir, texColor, pointShadowMap, pointFarPlane);
        }

        for (int i = 0; i < numSpotLights && i < MAX_SPOT_LIGHTS; i++) {
                result += calcSpotLight(spotLights[i], norm, FragPos, viewDir, texColor, spotShadowMaps[i], spotLightSpaceMatrices[i]);
        }

        FragColor = vec4(result, 1.0);
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor) {
        vec3 lightDir = normalize(-light.direction);
        vec3 ambient = vec3(0.0);

        float shadow = DirShadowCalculation(LightSpacePos, normal, lightDir);
        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * texColor * shadow;
        // Applique l'ombre

        // Specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), material.shininess);
        vec3 specular = light.specular * material.specular * spec * shadow;
        // Applique l'ombre

        return ambient + diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor, samplerCube shadowMap, float farPlane) {
        vec3 lightDir = normalize(light.position - fragPos);
        float distance = length(light.position - fragPos);

        float shadow = PointShadowCalculation(fragPos, light.position, shadowMap, farPlane);
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

        diffuse *= attenuation * shadow;
        specular *= attenuation * shadow;

        return ambient + diffuse + specular;
}

vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor, sampler2D shadowMap, mat4 lightSpaceMatrix) {
        vec3 lightDir = normalize(light.position - fragPos);
        vec3 spotDir = normalize(normalize(light.direction));
        float distance = length(light.position - fragPos);
        // Calculate spotlight intensity
        float cosDir = dot(-lightDir, spotDir);
        float spotIntensity = smoothstep(light.cosOuterCone, light.cosInnerCone, cosDir);

        vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0f);
        float shadow = SpotShadowCalculation(fragPosLightSpace, normal, lightDir, shadowMap);

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
        diffuse *= attenuation * spotIntensity * shadow;
        specular *= attenuation * spotIntensity * shadow;

        return ambient + diffuse + specular;
}

float DirShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // 1. Division de perspective
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // 2. Transformation vers l'espace [0, 1]
    projCoords = projCoords * 0.5 + 0.5;
    // 3. Vérification de la portée (si hors champ, pas d'ombre)
    if(projCoords.z > 1.0)
        return 1.0;
    // 4. Lecture de la profondeur la plus proche (Depth Map)
    float closestDepth = texture(dirShadowMap, projCoords.xy).r;
    // 5. Profondeur du fragment actuel
    float currentDepth = projCoords.z;
    // 6. Bias pour éviter le Shadow Acne
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.002);
    // 7. Comparaison de la profondeur
    float shadow = currentDepth - bias > closestDepth ? 0.0 : 1.0;

    return shadow;
}

float SpotShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2D shadowMap) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0)
        return 1.0;
    // Assurez-vous que le fragment est dans le cône du spot
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;

    // PCF pour lissage
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            float currentDepth = projCoords.z;
            float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.002);
            shadow += currentDepth - bias > closestDepth ? 0.0 : 1.0;
        }
    }
    return shadow / 9.0;
}

// NOUVEAU: Shadow Cube Map pour Point Light (avec PCF)
float PointShadowCalculation(vec3 fragPos, vec3 lightPos, samplerCube shadowMap, float farPlane) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDistance = length(fragToLight);

    // PCF
    float shadow_total = 0.0;
    float samples = 25.0;
    float layerSize = 5.0; // 5x5 cube face samples
    float diskRadius = 0.5f; // Ajustez cette valeur pour l'effet de flou

    // Pour chaque échantillon dans un disque autour de la direction
    for (float i = -layerSize/2.0; i <= layerSize/2.0; ++i) {
        for (float j = -layerSize/2.0; j <= layerSize/2.0; ++j) {
            // Calculer la direction échantillonnée
            vec3 sampleDirection = normalize(fragToLight + vec3(i, j, 0.0) * diskRadius / layerSize);

            // Lire la profondeur stockée (distance linéaire normalisée)
            // Note: La fonction `texture` sur samplerCube lit la profondeur basée sur le vecteur directionnel
            float closestDepth = texture(shadowMap, sampleDirection).r;

            // Dé-normaliser la distance lue
            float closestDistance = closestDepth * farPlane;

            // Bias (plus le fragment est proche de la lumière et plus la normale est alignée)
            // Un petit bias constant peut être plus stable
            float bias = 0.05 * (1.0 - abs(dot(normalize(Normal), normalize(fragToLight))));
            if (currentDistance - bias > closestDistance)
                shadow_total += 0.0;
            else
                shadow_total += 1.0;
        }
    }

    return shadow_total / (layerSize * layerSize);
}