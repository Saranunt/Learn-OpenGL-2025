#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform bool isBox;

void main()
{
    vec3 pos = aPos;
    
    if (isBox) {
        // Boat rendering - calculate proper normals for lighting
        // For a boat hull, we'll use a simple approach
        vec3 worldPos = vec3(model * vec4(pos, 1.0f));
        if (pos.y < 0.0f) {
            // Bottom of hull - normal pointing down
            Normal = vec3(0.0f, -1.0f, 0.0f);
        } else if (pos.y > 0.0f) {
            // Top/deck - normal pointing up
            Normal = vec3(0.0f, 1.0f, 0.0f);
        } else {
            // Sides - calculate based on position
            float side = sign(pos.x);
            Normal = normalize(vec3(side, 0.3f, 0.0f));
        }
    } else {
        // Water rendering - apply wave animation
        // Add subtle high-frequency waves in the shader
        float wave = 0.05f * sin(pos.x * 8.0f + time * 3.0f) * cos(pos.z * 6.0f + time * 2.5f);
        pos.y += wave;
        
        // Calculate normal for lighting (simplified)
        float dx = 0.05f * 8.0f * cos(pos.x * 8.0f + time * 3.0f) * cos(pos.z * 6.0f + time * 2.5f);
        float dz = -0.05f * 6.0f * sin(pos.x * 8.0f + time * 3.0f) * sin(pos.z * 6.0f + time * 2.5f);
        Normal = normalize(vec3(-dx, 1.0f, -dz));
    }
    
    FragPos = vec3(model * vec4(pos, 1.0f));
    gl_Position = projection * view * model * vec4(pos, 1.0f);
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}