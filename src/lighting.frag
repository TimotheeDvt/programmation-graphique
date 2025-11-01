#version 330 core
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    float ambiantFactor = 0.05f;
    vec3 ambiant = lightColor * ambiantFactor;

    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float NDotL = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = lightColor * NDotL;

    float specularFactor = 0.8f;
    float shininess = 120.0f;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float NDotH = max(dot(normal, halfDir), 0.0f);
    vec3 specular = lightColor * specularFactor * pow(NDotH, shininess);

    vec4 texel = texture(texture1, TexCoord);
    FragColor = vec4(ambiant + diffuse + specular, 1.0f) * texel;
}
