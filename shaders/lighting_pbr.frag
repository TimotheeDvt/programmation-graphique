#version 330 core

// Fragment shader for PBR lighting (works with a typical lighting_pbr.vert)
// Inputs expected from the vertex shader:
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 camPos;

struct Light {
    vec3 Position;
    vec3 Color;
};
const int MAX_LIGHTS = 4;
uniform Light lights[MAX_LIGHTS];
uniform int lightCount;

// Material / IBL textures
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// Helpers
const float PI = 3.14159265359;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    // Assume the vertex shader provided a TBN matrix as vec3s in extra varyings if needed.
    // If not available, fallback to interpolated normal
    // Here we just orthonormalize around the interpolated normal:
    vec3 N = normalize(Normal);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

// Microfacet BRDF (Cook-Torrance)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 1e-5);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Material properties from textures
    vec3 albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2)); // assume sRGB -> linear
    float metallic = texture(metallicMap, TexCoords).r;
    float roughness = texture(roughnessMap, TexCoords).r;
    float ao = texture(aoMap, TexCoords).r;

    // Normal mapping
    vec3 N = getNormalFromMap();
    vec3 V = normalize(camPos - FragPos);
    vec3 R = reflect(-V, N);

    // Calculate reflectance at normal incidence; if metallic use albedo as F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Accumulate lighting from punctual lights
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < lightCount && i < MAX_LIGHTS; ++i)
    {
        vec3 L = normalize(lights[i].Position - FragPos);
        vec3 H = normalize(V + L);
        float distance = length(lights[i].Position - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lights[i].Color * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-5;
        vec3 specular = numerator / denom;

        // kS is specular ratio, kD is diffuse component (energy conservation)
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);

        // Add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // IBL: ambient lighting
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    // Diffuse irradiance
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;

    // Specular IBL
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (diffuse * (1.0 - F) * (1.0 - metallic) + specular) * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping (simple ACES approximation) and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}