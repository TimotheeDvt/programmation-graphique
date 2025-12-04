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

uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

flat in int TexIndex;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform DirectionalLight dirLight;
uniform int numPointLights;
uniform int numSpotLights;

uniform Material material;
uniform vec3 viewPos;

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor);
vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor);

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

        vec3 result = calcDirectionalLight(dirLight, norm, viewDir, texColor);

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

vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor) {
        vec3 lightDir = normalize(light.position - fragPos);
        vec3 spotDir = normalize(light.direction);

        // Calculate spotlight intensity
        float cosDir = dot(-lightDir, spotDir);
        float spotIntensity = smoothstep(light.cosOuterCone, light.cosInnerCone, cosDir);

        // Ambient (minimal, not affected by spotlight cone)
        vec3 ambient = light.ambient * material.ambient * texColor;

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

        diffuse *= attenuation * spotIntensity;
        specular *= attenuation * spotIntensity;

        return ambient + diffuse + specular;
}