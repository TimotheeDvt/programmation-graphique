#version 330 core

in vec2 TexCoord;

uniform vec4 vertColor;
uniform sampler2D myTexture1;
uniform sampler2D myTexture2;

out vec4 frag_color;

void main() {
       // frag_color = vertColor;
       frag_color = mix(mix(texture(myTexture1, TexCoord), texture(myTexture2, TexCoord), 0.2), vertColor, 0.5);
};