#version 330

in vec3 fragNormal;
in vec2 fragTextCoord;

uniform sampler2D mainTexture;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out float textureInd;

void main() {
    vec4 texel = texture(mainTexture, fragTextCoord);
    FragColor = vec4(texel.rgb, 1.0);
    textureInd = texel.a;
}
