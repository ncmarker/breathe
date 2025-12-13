#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 discColor;
uniform float uAlphaMultiplier = 1.0;

void main() {
    // UV.x represents distance from center (0.0 = center, 1.0 = edge)
    float distanceFromCenter = TexCoord.x;
    
    // Exponential falloff: bright at center, transparent at edge
    // Doesn't reach fully transparent (stops before outer ring)
    float alpha = pow(1.0 - distanceFromCenter, 2.5);
    alpha *= 0.9; // Max opacity at center
    alpha *= uAlphaMultiplier;
    
    FragColor = vec4(discColor, alpha);
}

