#version 330 core
out vec4 FragColor;

in vec3 WorldPos;

void main()
{
    // Create gradient based on world position x, z
    // Use modulo to create repeating pattern (tile every 200 units)
    float tileSize = 200.0;
    float localX = mod(WorldPos.x, tileSize);
    float localZ = mod(WorldPos.z, tileSize);
    
    // Normalize to 0-1 range for gradient
    float normalizedX = localX / tileSize;
    float normalizedZ = localZ / tileSize;
    
    // Create gradient colors based on position
    vec3 color1 = vec3(0.3, 0.5, 0.3); // Green
    vec3 color2 = vec3(0.2, 0.4, 0.2); // Dark green
    vec3 color3 = vec3(0.4, 0.6, 0.4); // Light green
    
    // Mix colors based on x and z position
    vec3 colorX = mix(color2, color3, normalizedX);
    vec3 colorZ = mix(color2, color1, normalizedZ);
    vec3 finalColor = (colorX + colorZ) * 0.5;
    
    FragColor = vec4(finalColor, 1.0);
}

