#version 330 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 FragPos[];
in vec3 Normal[];

out vec3 FragPosGS;
out vec3 NormalGS;
out float DistanceFromCenter;

uniform mat4 projection;
uniform mat4 view;
uniform float uLineWidth;
uniform float uAltitudeOffset; // Push vertices outward along normal for layering

void main() {
    vec3 pos0 = FragPos[0];
    vec3 pos1 = FragPos[1];
    
    // Push vertices outward along normal for layering (white line above blue)
    pos0 = pos0 + Normal[0] * uAltitudeOffset;
    pos1 = pos1 + Normal[1] * uAltitudeOffset;
    
    vec4 clipPos0 = projection * view * vec4(pos0, 1.0);
    vec4 clipPos1 = projection * view * vec4(pos1, 1.0);
    
    vec2 screen0 = clipPos0.xy / clipPos0.w;
    vec2 screen1 = clipPos1.xy / clipPos1.w;
    
    vec2 dir = screen1 - screen0;
    float len = length(dir);
    if (len < 0.0001) return; // Skip degenerate lines
    
    dir = normalize(dir);
    vec2 perp = vec2(-dir.y, dir.x);
    
    // Calculate line width in NDC space (perspective-correct)
    float widthNDC = uLineWidth * 0.0008;
    vec4 offset0 = vec4(perp * widthNDC * clipPos0.w, 0.0, 0.0);
    vec4 offset1 = vec4(perp * widthNDC * clipPos1.w, 0.0, 0.0);
    
    // Emit quad as triangle strip
    FragPosGS = pos0;
    NormalGS = Normal[0];
    DistanceFromCenter = -1.0;
    gl_Position = clipPos0 - offset0;
    EmitVertex();
    
    FragPosGS = pos1;
    NormalGS = Normal[1];
    DistanceFromCenter = -1.0;
    gl_Position = clipPos1 - offset1;
    EmitVertex();
    
    FragPosGS = pos0;
    NormalGS = Normal[0];
    DistanceFromCenter = 1.0;
    gl_Position = clipPos0 + offset0;
    EmitVertex();
    
    FragPosGS = pos1;
    NormalGS = Normal[1];
    DistanceFromCenter = 1.0;
    gl_Position = clipPos1 + offset1;
    EmitVertex();
    
    EndPrimitive();
}

