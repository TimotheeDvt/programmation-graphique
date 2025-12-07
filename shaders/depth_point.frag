#version 330 core
in vec4 FragPos; // Reçoit la position du fragment en coordonnées mondiales
uniform vec3 lightPos; // Position de la lumière
uniform float farPlane; // Portée de la lumière

flat in int TexIndex;

void main() {
    if (TexIndex == 9 || TexIndex == 8 || TexIndex == 4) {
        discard; // discard Redstone, Torches and Glass
    }
    // Calculer la distance linéaire de la lumière au fragment
    float lightDistance = length(FragPos.xyz - lightPos);

    // Normaliser la distance entre 0 et 1 (0.0 pour proche, 1.0 pour loin)
    // et écrire dans la texture.
    gl_FragDepth = lightDistance / farPlane;
}