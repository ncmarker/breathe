#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float uAltitudeOffset; // Push vertices outward along normal

out vec3 FragPos;
out vec3 Normal;

void main() {
    // Push vertex outward along normal for layering
    vec3 pos = aPos + aNormal * uAltitudeOffset;
    
    FragPos = vec3(model * vec4(pos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
