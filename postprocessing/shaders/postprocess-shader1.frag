#version 330 core

in vec2 SurfaceTextPos;

uniform sampler2D framebufferColorTexture;
uniform sampler2D framebufferIndTexture;

out vec4 FragColor;

uint decodeId(float encodedId) {
    return uint(round(encodedId * 255.0));
}

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(framebufferIndTexture, 0));

    uint center = decodeId(texture(framebufferIndTexture, SurfaceTextPos).r);
    uint left = decodeId(texture(framebufferIndTexture, SurfaceTextPos + 2 * vec2(-texelSize.x, 0.0)).r);
    uint right = decodeId(texture(framebufferIndTexture, SurfaceTextPos + 2 * vec2(texelSize.x, 0.0)).r);
    uint up = decodeId(texture(framebufferIndTexture, SurfaceTextPos + 2 * vec2(0.0, texelSize.y)).r);
    uint down = decodeId(texture(framebufferIndTexture, SurfaceTextPos + 2 * vec2(0.0, -texelSize.y)).r);

    bool edge = center != left || center != right || center != up || center != down;

    vec4 sceneColor = texture(framebufferColorTexture, SurfaceTextPos);

    if (edge) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = sceneColor;
    }
}
