#version 430 core

layout(quads, fractional_even_spacing, ccw) in;
layout(binding = 0) uniform sampler2DArray uHeightLayers;

uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vUV;
out vec3 vNormal;
flat out int vMaterialLayer;

float heightAt(vec2 uv) {
    float h = textureLod(uHeightLayers, vec3(uv, float(gl_PrimitiveID)), 0.0).r * 3.0;

    vec2 edge = min(uv, vec2(1.0) - uv);
    vec2 fade = smoothstep(vec2(0.0), vec2(0.15), edge);
    return h * fade.x * fade.y;
}

void main() {
    vec2 t = gl_TessCoord.xy;
    vec3 position = mix(mix(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, t.x),
                        mix(gl_in[3].gl_Position.xyz, gl_in[2].gl_Position.xyz, t.x), t.y);

    position.y = heightAt(t);
    gl_Position = uProjection * uView * vec4(position, 1.0);
    vUV = t.xy;
    vMaterialLayer = gl_PrimitiveID;
    vec2 stepUV = 1.0 / vec2(textureSize(uHeightLayers, 0).xy);
    float hLeft = heightAt(t - vec2(stepUV.x, 0.0));
    float hRight = heightAt(t + vec2(stepUV.x, 0.0));
    float hMinusV = heightAt(t - vec2(0.0, stepUV.y));
    float hPlusV = heightAt(t + vec2(0.0, stepUV.y));
    vec2 patchSize = vec2(length(gl_in[1].gl_Position.xz - gl_in[0].gl_Position.xz),
                          length(gl_in[3].gl_Position.xz - gl_in[0].gl_Position.xz));
    float slopeX = (hRight - hLeft) / (2.0 * stepUV.x * patchSize.x);
    float slopeV = (hPlusV - hMinusV) / (2.0 * stepUV.y * patchSize.y);
    vNormal = normalize(vec3(-slopeX, 1.0, slopeV));
}
