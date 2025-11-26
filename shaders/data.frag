#version 330 core

in vec2 vUV;
in vec3 vWorldPos;
out vec4 FragColor;

uniform float uTime; // For future animations (not used yet)

void main()
{
    // vUV.x = normalized emission value (0.0 to 1.0)
    // vUV.y = distance indicator (0.0 = center, 1.0 = edge)
    
    // Compute radial distance from center (0.0 = center, 1.0 = edge)
    float distance = vUV.y;
    
    // Create smooth radial gradient: opaque at center, transparent at edge
    // Using smoothstep for smooth falloff - creates fog/smog effect
    float alpha = 1.0 - smoothstep(0.0, 1.0, distance);
    
    // Continuous color gradient based on emission value (not discrete thirds)
    // Smooth transition from yellow -> orange -> red based on vUV.x (0.0 to 1.0)
    // More vibrant colors for better visibility while maintaining matte appearance
    vec3 color;
    
    // Continuous gradient: yellow (low) -> orange (medium) -> red (high)
    if (vUV.x < 0.5) {
        // Low to medium: yellow-gray -> orange-brown
        float t = vUV.x / 0.5; // 0.0 to 1.0 across first half
        color = mix(vec3(0.8, 0.75, 0.6), vec3(0.75, 0.55, 0.35), t);
    } else {
        // Medium to high: orange-brown -> red-brown
        float t = (vUV.x - 0.5) / 0.5; // 0.0 to 1.0 across second half
        color = mix(vec3(0.75, 0.55, 0.35), vec3(0.7, 0.3, 0.25), t);
    }
    
    // Keep colors visible but matte (not glowing)
    color *= 0.8; // Slightly brighter than before for better visibility
    
    // Apply alpha for transparency gradient
    // Additive blending will make overlapping areas blend into clouds
    FragColor = vec4(color, alpha);
}



