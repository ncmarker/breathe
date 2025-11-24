#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform vec3 baseColor;

void main()
{
    float intensity = clamp(vUV.x, 0.0, 1.0);
    float brightness = mix(0.3, 1.0, intensity);
    FragColor = vec4(baseColor * brightness, 1.0);
}



