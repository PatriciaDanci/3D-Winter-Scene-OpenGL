#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace; // Light space position from vertex shader

out vec4 fColor;

// Uniforms
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap; 
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientLight;
uniform vec3 pointLightPosition;
uniform vec3 lightColorPoint;
uniform bool fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;
uniform bool dirLightEnabled; // Toggle for directional light
uniform bool pointLightEnabled;  // Toggle for point light

uniform bool transparentLake;

// Attenuation factors
uniform float constantAttenuation;
uniform float linearAttenuation;
uniform float quadraticAttenuation;

// Components for lighting calculations
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;

uniform vec3 pointLight1Position;
uniform vec3 pointLight1Color;
uniform vec3 pointLight2Position;
uniform vec3 pointLight2Color;
uniform vec3 pointLight3Position;
uniform vec3 pointLight3Color;
uniform bool additionalPointLightsEnabled; // Toggle for additional point lights

// Function to compute directional light
void computeDirLight() {
    if (!dirLightEnabled) { // Skip directional light if disabled
        ambient = ambientLight; // Use only ambient light
        diffuse = vec3(0.0f);
        specular = vec3(0.0f);
        return;
    }

    // Compute eye-space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    // Normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    // Compute view direction (camera at origin in eye space)
    vec3 viewDir = normalize(-fPosEye.xyz);

    // Compute ambient light
    ambient = ambientStrength * lightColor;

    // Compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    // Compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
}


// Function to compute point light
void computePointLight(vec3 fragPos, vec3 normal, vec3 viewDir) {
    if (!pointLightEnabled) {
        ambient += vec3(0.0); // No contribution if point light is off
        diffuse += vec3(0.0);
        specular += vec3(0.0);
        return;
    }

    // Compute light direction and distance
    vec3 lightPosEye = (view * vec4(pointLightPosition, 1.0)).xyz; // Transform light position to eye space)
    vec3 lightDir = normalize(lightPosEye - fragPos);
    float distance = length(pointLightPosition - fragPos);

    // Adjust attenuation factors to make the light more intense
    float constantAttenuation = 0.8; // No constant falloff
    float linearAttenuation = 0.45;  // Lower linear attenuation for wider spread
    float quadraticAttenuation = 0.001; // Lower quadratic attenuation
    float attenuation = 1.0 / (constantAttenuation + linearAttenuation * distance + quadraticAttenuation * (distance * distance));

    // Ambient component
    ambient += 0.1 * lightColorPoint * attenuation; // Increased ambient intensity

    // Diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    diffuse += diff * lightColorPoint * attenuation * 2.0; // Scale up diffuse intensity

    // Specular component
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    specular += 1.0 * spec * lightColorPoint * attenuation; // Brighter specular highlights
}


// Function to compute additional point lights
void computeAdditionalPointLight(vec3 fragPos, vec3 normal, vec3 viewDir, vec3 lightPos, vec3 lightColor) {
    // Compute light direction and distance
    vec3 lightPosEye = (view * vec4(lightPos, 1.0)).xyz; // Transform light position to eye space
    vec3 lightDir = normalize(lightPosEye - fragPos);
    float distance = length(lightPos - fragPos);

    // Adjust attenuation factors to make the light more intense
    float constantAttenuation = 0.8; // Minimal constant attenuation
    float linearAttenuation = 0.45;  // Medium linear attenuation for wider spread
    float quadraticAttenuation = 0.001; // Lower quadratic attenuation
    float attenuation = 1.0 / (constantAttenuation + linearAttenuation * distance + quadraticAttenuation * (distance * distance));

    // Ambient component
    ambient += 0.1 * lightColor * attenuation; // Increased ambient intensity for each light

    // Diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    diffuse += diff * lightColor * attenuation * 2.0; // Scale up diffuse intensity

    // Specular component
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    specular += 1.0 * spec * lightColor * attenuation; // Brighter specular highlights
}


// Function to compute shadow
float computeShadow() {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // Transform to [0,1] range
   
    // Read depth from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    //float bias = 0.005;
    float bias = max(0.001f * (1.0f - dot(fNormal, normalize(lightDir))), 0.0001f);
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    // Avoid shadowing artifacts beyond the far plane
    if (projCoords.z > 1.0) {
        shadow = 0.0;
    }

    return shadow;
}

uniform bool isSmoke; // New uniform to differentiate smoke particles
uniform float smokeTransparency; // Uniform for smoke transparency

// Main function
void main() {
    vec4 fragPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 fragPos = fragPosEye.xyz;
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 viewDir = normalize(-fragPos);

    // Reset lighting components
    ambient = vec3(0.0);
    diffuse = vec3(0.0);
    specular = vec3(0.0);

    // Compute directional light
    computeDirLight();

    computePointLight(fragPos, normalEye, viewDir);

     // Compute additional point lights
    if (additionalPointLightsEnabled) {
        computeAdditionalPointLight(
            fragPos,
            normalEye,
            viewDir,
            pointLight1Position,
            pointLight1Color
        );
        computeAdditionalPointLight(
            fragPos,
            normalEye,
            viewDir,
            pointLight2Position,
            pointLight2Color
        );
        computeAdditionalPointLight(
            fragPos,
            normalEye,
            viewDir,
            pointLight3Position,
            pointLight3Color
        );
    }

    // Compute shadow
    float shadow = computeShadow();

    // Apply shadows to the diffuse and specular components
    vec3 shadowedDiffuse = (1.0 - shadow) * diffuse;
    vec3 shadowedSpecular = (1.0 - shadow) * specular;

    // Final color calculation
    vec3 color = ambient + shadowedDiffuse + shadowedSpecular;
    color *= texture(diffuseTexture, fTexCoords).rgb;

    // Apply fog if enabled
    if (fogEnabled) {
        float fogFactor = exp(-pow(length(fragPosEye.xyz) * fogDensity, 0.5));
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        fColor = mix(fogColor, vec4(color, 1.0f), fogFactor);
        }
    else {
        fColor = vec4(color, 1.0f);
    }

    if (isSmoke) {
        fColor.w = smokeTransparency; 
    } else if (transparentLake) {
        fColor.w = 0.4f; 
    } else {
        fColor.w = 1.0f; 
    }
}