#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 diffuseColor;
layout(location = 2) in vec3 normal;

smooth out vec4 baseColor;

uniform vec3 lightPosCamera;
uniform vec4 lightIntency;
uniform float attenuationFactor;
uniform vec4 ambientIntensity;
uniform mat4 modelToCameraMatrix;
uniform mat4 cameraToClipMatrix;

void main() {
    vec4 cameraPos = modelToCameraMatrix * vec4(position, 1.0);

    vec3 lightDirCamera = cameraPos.xyz - lightPosCamera;
    vec3 normalCamera = normalize(transpose(inverse(mat3(modelToCameraMatrix))) * normal);

    float cosCof = clamp(dot(normalCamera, normalize(-lightDirCamera)), 0, 1);

    vec3 toLight = vec3(cameraPos) - lightPosCamera;

    float toLightDistanceSquared = dot(toLight, toLight);

    float attenuation = 1.0f / (1.0f + attenuationFactor * toLightDistanceSquared);
    baseColor = (vec4(diffuseColor.xyz, 1.0f) * cosCof * lightIntency * attenuation) +
                (vec4(diffuseColor.xyz, 1.0f) * ambientIntensity);

    baseColor = sqrt(baseColor); // Gamma 2.0.
    gl_Position = cameraToClipMatrix * cameraPos;
}
