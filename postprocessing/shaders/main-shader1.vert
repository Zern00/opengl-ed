#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

out vec3 fragNormal;
out vec2 fragTextCoord;

void main() {
    gl_Position = proj * view * model * vec4(pos, 1.0f);
    fragNormal = transpose(inverse(mat3(view * model))) * normal;
    fragTextCoord = uv;
}
