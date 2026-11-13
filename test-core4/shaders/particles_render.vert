#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 2) in float aAge;
layout(location = 3) in float aLifetime;

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uViewportHeight;

out vec4 vColor;

void main() {
    vec4 viewPosition = uView * vec4(aPosition, 1.0);
    gl_Position = uProjection * viewPosition;
    float life = clamp(aAge / aLifetime, 0.0, 1.0);
    float opacity = smoothstep(0.0, 0.1, aAge) * (1.0 - smoothstep(0.5, 1.0, life));
    vColor = vec4(mix(vec3(1.0, 0.75, 0.15), vec3(1.0, 0.15, 0.02), life), opacity);
    float diameter = mix(0.055, 0.025, life);
    gl_PointSize = clamp(diameter * uViewportHeight * uProjection[1][1] / (2.0 * max(-viewPosition.z, 0.1)), 2.0, 14.0);
}
