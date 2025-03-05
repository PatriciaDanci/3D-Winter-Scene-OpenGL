#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out vec4 fragPosLightSpace; // Pass light space position to fragment shader

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceTrMatrix; // Light space transformation matrix

uniform float time;
uniform bool applyWave;
uniform bool applyTreeWind;

float treeWindAmplitude = 0.1f;
float treeWindSpeed = 0.4f;
float waveAmplitude = 1.0f;
float waveSpeed = 3.2f;

void main() {
    vec3 modifiedPosition = position; // Start with the original vertex position

    if (applyWave) {
        float wave = sin(position.x + time * waveSpeed) 
                     * waveAmplitude 
                     * position.y;

        modifiedPosition.y += wave;
        modifiedPosition.x += wave * 0.7;
        modifiedPosition.z += wave * 0.3;
    }

    if (applyTreeWind) {
        float wind = sin(position.x + time * treeWindSpeed) 
                     * treeWindAmplitude 
                     * position.y;

        modifiedPosition.y += wind;
        modifiedPosition.x += wind * 0.7;
        modifiedPosition.z += wind * 0.3;
    }

    vec4 worldPosition = model * vec4(modifiedPosition, 1.0);

    fPosition = worldPosition.xyz;
    fNormal = mat3(transpose(inverse(model))) * normal;
    fTexCoords = texCoords;
    fragPosLightSpace = lightSpaceTrMatrix * worldPosition;

    gl_Position = projection * view * worldPosition;
}
