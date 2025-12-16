#version 330 core

in vec2 vUV;
in vec3 vWorldPos;
out vec4 FragColor;

uniform float uTime; 

void main()
{
    // vUV.x = normalized emission value (0.0 to 1.0)
    // vUV.y = distance indicator (0.0 = center, 1.0 = edge)
    
    // Compute radial distance from center (0.0 = center, 1.0 = edge)
    float distance = vUV.y;
    
    // Create smooth radial gradient: opaque at center, transparent at edge
    // Using smoothstep for smooth falloff - creates fog/smog effect
    float alpha = 1.0 - smoothstep(0.0, 1.0, distance);
    
    // Continuous color gradient based on emission value
    // Orange/amber spectrum: soft amber (low) -> intense orange (high)
    vec3 color;
    
    // Continuous gradient: amber (low) -> orange (high)
    if (vUV.x < 0.5) {
        // Low to medium: soft amber -> medium orange
        float t = vUV.x / 0.5; // 0.0 to 1.0 across first half
        color = mix(vec3(0.9, 0.7, 0.4), vec3(0.95, 0.55, 0.2), t);
    } else {
        // Medium to high: medium orange -> intense orange
        float t = (vUV.x - 0.5) / 0.5; // 0.0 to 1.0 across second half
        color = mix(vec3(0.95, 0.55, 0.2), vec3(1.0, 0.4, 0.1), t);
    }
    
    // Keep colors visible but matte 
    color *= 0.8;
    
    // Apply alpha for transparency gradient
    // Additive blending will make overlapping areas blend into clouds
    FragColor = vec4(color, alpha);
}



