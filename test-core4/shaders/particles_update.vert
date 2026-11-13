#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aVelocity;
layout(location = 2) in float aAge;
layout(location = 3) in float aLifetime;

uniform float uDeltaTime;
uniform float uTime;

out vec3 tfPosition;
out vec3 tfVelocity;
out float tfAge;
out float tfLifetime;

float randomValue(float seed) {
    return fract(sin(seed * 12.9898 + 78.233) * 43758.5453);
}

void main() {
    tfPosition = aPosition;
    tfVelocity = aVelocity;
    tfAge = aAge + uDeltaTime;
    tfLifetime = aLifetime;

    if (tfAge >= 0.0) {
        if (aAge < 0.0 || tfAge >= aLifetime || aPosition.y < 0.05) {
            float seed = float(gl_VertexID) + floor(uTime * 60.0) * 0.37;
            float angle = randomValue(seed) * 6.2831853;
            float radialSpeed = 0.3 + randomValue(seed + 1.0) * 0.9;
            tfPosition = vec3(0.0, 0.15, 0.0);
            tfVelocity = vec3(cos(angle) * radialSpeed, 3.0 + randomValue(seed + 2.0) * 2.0, sin(angle) * radialSpeed);
            tfAge = 0.0;
            tfLifetime = 2.2 + randomValue(seed + 3.0) * 1.3;
        } else {
            vec3 gravity = vec3(0.0, -2.5, 0.0);
            tfPosition += aVelocity * uDeltaTime + 0.5 * gravity * uDeltaTime * uDeltaTime;
            tfVelocity += gravity * uDeltaTime;
        }
    }
    gl_Position = vec4(tfPosition, 1.0);
}
