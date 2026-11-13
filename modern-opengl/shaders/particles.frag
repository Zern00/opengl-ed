#version 450 core

in float life;
out vec4 FragColor;

void main() {
    float radius = length(gl_PointCoord * 2.0 - 1.0);
    if (radius >= 1.0) {
        discard;
    }
    vec3 color = mix(vec3(1.0, 0.8, 0.2), vec3(1.0, 0.15, 0.02), life);
    FragColor = vec4(color, (1.0 - smoothstep(0.0, 1.0, radius)) * (1.0 - life) * 0.5);
}
