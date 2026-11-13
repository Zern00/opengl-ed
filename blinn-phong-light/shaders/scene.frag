#version 330 core

in vec3 cameraSpacePosition;
in vec3 cameraSpaceNormal;
out vec4 outputColor;

layout(std140) uniform Material {
    vec4 diffuseColor;
    vec4 specularColor;
    float specularShininess;
} Mtl;

uniform vec3 lightPositionCamera;
uniform vec3 lightIntensity;
uniform vec3 ambientIntensity;
uniform float attenuationFactor;

void main() {
    vec3 normal = normalize(cameraSpaceNormal);
    vec3 lightDirNotNormal = cameraSpacePosition - lightPositionCamera;
    float lengthSquared = dot(lightDirNotNormal, lightDirNotNormal);

    vec3 lightDir = normalize(lightDirNotNormal);

    float diffuseFactor = max(dot(normal, -lightDir), 0.0f);
    vec3 diffuseLight = Mtl.diffuseColor.rgb * diffuseFactor;

    float attenuation = 1.0f / (1.0f + attenuationFactor * lengthSquared);

    vec3 ViewVec = normalize(vec3(0.0f, 0.0f, 0.0f) - cameraSpacePosition);
    vec3 halfVec = normalize(ViewVec + (-lightDir));

    float specularFactor = (diffuseFactor > 0) ? (pow(max(dot(halfVec, normal), 0.0f), Mtl.specularShininess)) : 0.0f;

    vec3 specularLight = Mtl.specularColor.rgb * specularFactor;

    vec3 linearColor = Mtl.diffuseColor.rgb * ambientIntensity;

    vec3 preGamma = linearColor + attenuation * lightIntensity * (diffuseLight + specularLight);

    outputColor = sqrt(vec4(preGamma, 1.0f));
}
