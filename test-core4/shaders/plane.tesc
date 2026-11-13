#version 430 core

layout(vertices = 4) out;

uniform vec3 uCameraPosition;

float GetLevel(vec3 position) {
    float d = distance(position, uCameraPosition);
    return mix(32.0, 8.0, smoothstep(5.0, 60.0, d));
}

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    if (gl_InvocationID == 0) {
        vec3 p0 = gl_in[0].gl_Position.xyz;
        vec3 p1 = gl_in[1].gl_Position.xyz;
        vec3 p2 = gl_in[2].gl_Position.xyz;
        vec3 p3 = gl_in[3].gl_Position.xyz;

        gl_TessLevelOuter[0] = GetLevel((p0 + p3) * 0.5);
        gl_TessLevelOuter[1] = GetLevel((p0 + p1) * 0.5);
        gl_TessLevelOuter[2] = GetLevel((p1 + p2) * 0.5);
        gl_TessLevelOuter[3] = GetLevel((p3 + p2) * 0.5);

        float inner = GetLevel((p0 + p1 + p2 + p3) * 0.25);

        gl_TessLevelInner[0] = inner;
        gl_TessLevelInner[1] = inner;
    }
}
