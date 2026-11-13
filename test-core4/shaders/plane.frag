#version 430 core

in vec2 vUV;
in vec3 vNormal;
flat in int vMaterialLayer;

layout(binding = 1) uniform sampler2DArray uMaterialLayers;

out vec4 fragColor;

void main() {
    vec3 color = texture(uMaterialLayers, vec3(vUV, float(vMaterialLayer))).rgb;
    vec3 lightDirection = normalize(vec3(0.4, 1.0, 0.3));
    float light = 0.3 + 0.7 * max(dot(normalize(vNormal), lightDirection), 0.0);
    fragColor = vec4(color * light, 1.0);
}
