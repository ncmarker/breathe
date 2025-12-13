#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 ringColor;
uniform float uAlphaMultiplier = 1.0;

void main() {
    // Solid color for 3D rings - 100% opacity to eliminate z-fighting
    FragColor = vec4(ringColor, 1.0 * uAlphaMultiplier);
}


