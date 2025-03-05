#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fNormal;
out vec4 fPosEye;
out vec2 fTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform	mat3 normalMatrix;

//Aici am adaugat variabilele pentru timp
uniform float time;
float windAmplitude = 1.5f;
float windSpeed = 1.0f;

void main() 
{
	//compute eye space coordinates
	fPosEye = view * model * vec4(vPosition, 1.0f);
	fNormal = normalize(normalMatrix * vNormal);
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);
	fTexCoords = vTexCoords;

	// vec4 originalPos = model * vec4(vPosition, 1.0f);

	// float wave = sin(vPosition.x + time * windSpeed) 
	// 			* windAmplitude 
	// 			* vPosition.y;

	// originalPos.y += wave;
	// originalPos.x += wave * 0.7;
	// originalPos.x += wave * 0.3;

	// fPosEye = view * originalPos;

	// fNormal = normalize(normalMatrix * vNormal);

	// fTexCoords = vTexCoords;

	// gl_Position = projection * view * originalPos;
}
