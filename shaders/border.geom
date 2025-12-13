#version 330 core

layout(lines) in;
layout(triangle_strip, max_vertices = 16) out;

in vec3 FragPos[];
in vec3 Normal[];

out vec3 FragPosGS;
out vec3 NormalGS;
out float DistanceFromCenter;

uniform mat4 projection;
uniform mat4 view;
uniform float uLineWidth;
uniform float uAltitudeOffset; // Push vertices outward along normal for layering
uniform float uLineDepth = 0.01; // Depth for 3D thick lines

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
    
    // For thick lines (>20), create 3D box; otherwise flat quad
    if (uLineWidth > 20.0) {
        // Create 3D box by extruding upward (Y direction) for horizontal lines
        vec3 upDir = vec3(0.0, 1.0, 0.0); // Up direction for extrusion
        vec3 depthOffset = upDir * uLineDepth;
        
        // Calculate world space width offset for horizontal lines on XZ plane
        vec3 worldDir = normalize(pos1 - pos0);
        vec3 worldPerp = normalize(cross(worldDir, upDir));
        worldPerp.y = 0.0;
        worldPerp = normalize(worldPerp);
        
        float worldWidth = uLineWidth * 0.0005;
        vec3 worldWidthOffset = worldPerp * worldWidth * 0.5;
        worldWidthOffset.y = 0.0;
        
        // Bottom quad vertices - preserve original Y coordinate exactly
        vec3 bottom0Left = vec3(pos0.x - worldWidthOffset.x, pos0.y, pos0.z - worldWidthOffset.z);
        vec3 bottom0Right = vec3(pos0.x + worldWidthOffset.x, pos0.y, pos0.z + worldWidthOffset.z);
        vec3 bottom1Left = vec3(pos1.x - worldWidthOffset.x, pos1.y, pos1.z - worldWidthOffset.z);
        vec3 bottom1Right = vec3(pos1.x + worldWidthOffset.x, pos1.y, pos1.z + worldWidthOffset.z);
        
        // Top quad vertices
        vec3 top0Left = bottom0Left + depthOffset;
        vec3 top0Right = bottom0Right + depthOffset;
        vec3 top1Left = bottom1Left + depthOffset;
        vec3 top1Right = bottom1Right + depthOffset;
        
        // Bottom face (counter-clockwise when viewed from below)
        FragPosGS = bottom0Left;
        NormalGS = -upDir;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(bottom0Left, 1.0);
        EmitVertex();
        
        FragPosGS = bottom0Right;
        NormalGS = -upDir;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(bottom0Right, 1.0);
        EmitVertex();
        
        FragPosGS = bottom1Left;
        NormalGS = -upDir;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(bottom1Left, 1.0);
        EmitVertex();
        
        FragPosGS = bottom1Right;
        NormalGS = -upDir;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(bottom1Right, 1.0);
        EmitVertex();
        
        EndPrimitive();
        
        // Top face (counter-clockwise when viewed from above)
        FragPosGS = top0Left;
        NormalGS = upDir;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(top0Left, 1.0);
        EmitVertex();
        
        FragPosGS = top1Left;
        NormalGS = upDir;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(top1Left, 1.0);
        EmitVertex();
        
        FragPosGS = top0Right;
        NormalGS = upDir;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(top0Right, 1.0);
        EmitVertex();
        
        FragPosGS = top1Right;
        NormalGS = upDir;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(top1Right, 1.0);
        EmitVertex();
        
        EndPrimitive();
        
        // Side faces (4 sides of the box)
        // Left side
        FragPosGS = bottom0Left;
        NormalGS = -worldPerp;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(bottom0Left, 1.0);
        EmitVertex();
        
        FragPosGS = bottom1Left;
        NormalGS = -worldPerp;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(bottom1Left, 1.0);
        EmitVertex();
        
        FragPosGS = top0Left;
        NormalGS = -worldPerp;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(top0Left, 1.0);
        EmitVertex();
        
        FragPosGS = top1Left;
        NormalGS = -worldPerp;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(top1Left, 1.0);
        EmitVertex();
        
        EndPrimitive();
        
        // Right side
        FragPosGS = bottom1Right;
        NormalGS = worldPerp;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(bottom1Right, 1.0);
        EmitVertex();
        
        FragPosGS = bottom0Right;
        NormalGS = worldPerp;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(bottom0Right, 1.0);
        EmitVertex();
        
        FragPosGS = top1Right;
        NormalGS = worldPerp;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(top1Right, 1.0);
        EmitVertex();
        
        FragPosGS = top0Right;
        NormalGS = worldPerp;
        DistanceFromCenter = 0.0;
        gl_Position = projection * view * vec4(top0Right, 1.0);
        EmitVertex();
        
        EndPrimitive();
    } else {
        // Flat quad for thin lines
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
}

