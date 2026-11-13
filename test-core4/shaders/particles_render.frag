#version 430 core

in vec4 vColor;
out vec4 fragColor;

void main() {
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    float radiusSquared = dot(p, p);
    if (radiusSquared >= 1.0 || vColor.a <= 0.0) {
        discard;
    }
    float alpha = vColor.a * (1.0 - smoothstep(0.1, 1.0, radiusSquared));
    fragColor = vec4(vColor.rgb, alpha * 0.6);
}
