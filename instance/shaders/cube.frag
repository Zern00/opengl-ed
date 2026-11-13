#version 330 core

in vec3 worldNormal;
in vec3 color;
uniform int fragmentWorkIterations;
out vec4 fragmentColor;

void main() {
    vec3 lightDirection = normalize(vec3(0.4, 0.7, 1.0));
    float diffuse = max(dot(normalize(worldNormal), lightDirection), 0.0);

    // Keep the workload visible to the compiler.
    vec3 work = vec3(gl_FragCoord.xy * 0.01, diffuse + gl_FragCoord.z);
    for (int i = 0; i < fragmentWorkIterations; ++i) {
        work = sin(work.yzx * 1.73 + vec3(0.13, 0.37, 0.71));
        work += cos(work.zxy * 2.11 + vec3(0.53, 0.29, 0.17));
    }
    float modulation = fragmentWorkIterations > 0 ? 0.95 + 0.05 * sin(work.x + work.y + work.z) : 1.0;
    fragmentColor = vec4(color * (0.25 + 0.75 * diffuse) * modulation, 1.0);
}
