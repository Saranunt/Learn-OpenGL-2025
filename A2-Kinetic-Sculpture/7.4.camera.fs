#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D waterTexture;
uniform sampler2D boxTexture;
uniform float time;
uniform vec3 viewPos;
uniform bool isBox;

void main()
{
    if (isBox) {
        // Box rendering - textured box with brown tint
        vec4 boxColor = texture(boxTexture, TexCoord);
        
        // Apply brown tint to the texture
        vec3 brownTint = vec3(1.0f, 1.0f, 1.0f); // Brown color
        vec3 tintedColor = brownTint; // boxColor.rgb *
        
        // Simple lighting for box
         vec3 lightDir = normalize(vec3(1.0f, 1.0f, 1.0f));
         float diff = max(dot(Normal, lightDir), 0.0f);
         vec3 diffuse = diff * vec3(0.8f, 0.6f, 0.4f); // Warm lighting
        
        // Add some specular highlights
        // vec3 viewDir = normalize(viewPos - FragPos);
        // vec3 reflectDir = reflect(-lightDir, Normal);
        // float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f);
        // vec3 specular = spec * vec3(0.3f, 0.2f, 0.1f); // Brownish specular
        
        vec3 finalColor = tintedColor * 0.9f + diffuse; // x + specular
        FragColor = vec4(finalColor, 1.0f);
    } else {
        // Water rendering - animated texture coordinates for flowing water effect
        vec2 animatedTexCoord = TexCoord + vec2(time * 0.1f, time * 0.05f);
        
        // Sample water texture with animated coordinates
        vec4 waterColor = texture(waterTexture, animatedTexCoord);
        
        // Add some color variation based on wave height
        float waveIntensity = length(Normal - vec3(0.0f, 1.0f, 0.0f));
        vec3 waveTint = mix(vec3(0.0f, 0.3f, 0.8f), vec3(0.2f, 0.6f, 1.0f), waveIntensity);
        
        // Simple lighting calculation
        vec3 lightDir = normalize(vec3(1.0f, 1.0f, 1.0f));
        float diff = max(dot(Normal, lightDir), 0.0f);
        vec3 diffuse = diff * vec3(0.8f, 0.9f, 1.0f);
        
        // Add some specular highlights for water-like appearance
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, Normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f);
        vec3 specular = spec * vec3(0.5f, 0.7f, 1.0f);
        
        // Combine all effects
        vec3 finalColor = waterColor.rgb * waveTint + diffuse + specular * 0.3f;
        
        // Add some transparency for water effect
        float alpha = 0.9f + 0.1f * sin(time * 2.0f + FragPos.x * 3.0f + FragPos.z * 2.0f);
        
        FragColor = vec4(finalColor, alpha);
    }
}