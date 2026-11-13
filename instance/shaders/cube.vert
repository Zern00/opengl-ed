#version 330 core

layout(location = 0) in vec3 position;
layout(location = 2) in vec3 normal;
layout(location = 3) in mat4 model;
layout(location = 7) in vec3 colorVert;

uniform mat4 viewProjection;
out vec3 worldNormal;
out vec3 color;
void main() {
    worldNormal = mat3(model) * normal;
    gl_Position = viewProjection * model * vec4(position, 1.0);
    color = colorVert;
}
