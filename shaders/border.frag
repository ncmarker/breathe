#version 330 core
out vec4 FragColor;

in vec3 FragPosGS;
in vec3 NormalGS;
in float DistanceFromCenter;

uniform vec3 borderColor;
uniform float uTime;
uniform float uGlowIntensity; // 0.0 = thin white line, 1.0 = thick blue glow
uniform float uLineWidth;
uniform float uAlphaMultiplier = 1.0; // Multiplier for alpha (for sphere grid transparency)

void main()
{
    if (uGlowIntensity > 0.5) {
        vec3 neonBlue = vec3(0.4, 0.7, 1.0);
        float pulse = 1.0 + 0.05 * sin(uTime * 2.0);
        
        // For thick lines (>20), use solid fill - brighter like country borders
        if (uLineWidth > 20.0) {
            vec3 color = neonBlue;
            FragColor = vec4(color, 0.3 * uAlphaMultiplier); // Much brighter - 30% opacity
        } else {
            // Thick blue glow layer with radial gradient
            float dist = abs(DistanceFromCenter);
            float alpha = pow(1.0 - dist, 3.0);
            alpha *= 0.5;
            
            vec3 color = neonBlue * pulse;
            FragColor = vec4(color, alpha * uAlphaMultiplier);
        }
    } else {
        // Thin white center line
        float dist = abs(DistanceFromCenter);
        if (dist < 0.25) {
            float alpha = pow(1.0 - (dist / 0.25), 3.0);
            FragColor = vec4(1.0, 1.0, 1.0, alpha);
        } else {
            discard;
        }
    }
}

