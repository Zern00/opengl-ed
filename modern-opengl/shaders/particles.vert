#version 450 core

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inVelocity;
uniform float aspect;
out float life;

void main() {
    life = clamp(inPosition.w / inVelocity.w, 0.0, 1.0);
    gl_Position = inPosition.w < 0.0 ? vec4(2.0, 2.0, 0.0, 1.0) : vec4(inPosition.x / aspect, inPosition.yz, 1.0);
    gl_PointSize = 7.0 * (1.0 - life) + 2.0;
}
