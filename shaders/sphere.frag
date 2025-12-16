#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 sphereColor;
uniform float ambientStrength;
uniform vec3 ambientColor;
uniform float emissiveStrength;

void main()
{
    // Tron-like glowing edges effect
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    // Fresnel effect for edge lighting
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 2.0);
    
    // Directional lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(N, lightDir), 0.2);
    
    vec3 ambient = ambientColor * ambientStrength;

    // for glowing edges
    vec3 emissive = sphereColor * emissiveStrength;
    
    vec3 finalColor = ambient + sphereColor * diff + emissive;
    finalColor += vec3(0.2, 0.6, 1.0) * fresnel * 0.5;
    
    FragColor = vec4(finalColor, 1.0);
}

