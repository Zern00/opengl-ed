#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 cameraSpacePosition;
out vec3 cameraSpaceNormal;

uniform mat4 modelToCameraMatrix;
uniform mat3 normalModelToCameraMatrix;
uniform mat4 cameraToClipMatrix;

void main() {
    vec4 cameraPosition = modelToCameraMatrix * vec4(position, 1.0);
    cameraSpacePosition = cameraPosition.xyz;
    cameraSpaceNormal = normalize(normalModelToCameraMatrix * normal);
    gl_Position = cameraToClipMatrix * cameraPosition;
}
