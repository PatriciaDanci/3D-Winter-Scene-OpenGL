#version 410 core

layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 lightSpaceTrMatrix;
uniform float time;
uniform bool applyTreeWind;

float treeWindAmplitude = 0.1f;
float treeWindSpeed = 0.8f;

void main() {
    vec3 transformedPosition = position;

    // Apply the wind effect if enabled
    if (applyTreeWind) {
        float wind = sin(position.x + time * treeWindSpeed) 
                   * treeWindAmplitude 
                   * position.y;

        transformedPosition.y += wind;
        transformedPosition.x += wind * 0.7;
        transformedPosition.z += wind * 0.3;
    }

    // Compute final position in light space
    gl_Position = lightSpaceTrMatrix * model * vec4(transformedPosition, 1.0);
}
