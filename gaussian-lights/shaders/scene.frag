#version 330 core

in vec3 cameraSpacePosition;
in vec3 cameraSpaceNormal;
out vec4 outputColor;

layout(std140) uniform Material {
    vec4 diffuseColor;
    vec4 specularColor;
    float specularRoughness;
} Mtl;

struct PointLight {
    vec4 cameraSpacePosition;
    vec4 intensity;
};

layout(std140) uniform Lights {
    vec4 ambientIntensity;
    float attenuationFactor;
    PointLight lights[3];
} Lgt;

vec3 CalculateLight(PointLight curLight) {
    vec3 normal = normalize(cameraSpaceNormal);
    vec3 lightDirNotNorm = curLight.cameraSpacePosition.xyz - cameraSpacePosition;
    float lengthSquared = dot(lightDirNotNorm, lightDirNotNorm);

    vec3 lightDir = normalize(lightDirNotNorm);

    float diffuseFactor = max(dot(lightDir, normal), 0.0f);
    vec3 diffuseLight = diffuseFactor * Mtl.diffuseColor.rgb;

    float attenuation = 1.0f / (1.0f + Lgt.attenuationFactor * lengthSquared);

    vec3 cameraDir = normalize(vec3(0.0f) - cameraSpacePosition);
    vec3 halfVec = normalize(lightDir + cameraDir);

    float specularFactor = 0.0f;
    if (diffuseFactor > 0.0f) {
        float angle = acos(clamp(dot(normal, halfVec), -1.0f, 1.0f));
        float ex = angle / Mtl.specularRoughness;

        specularFactor = exp(-(ex * ex));
    }

    vec3 specularLight = specularFactor * Mtl.specularColor.rgb;

    vec3 finalColor = attenuation * curLight.intensity.rgb * (diffuseLight + specularLight);

    return finalColor;
}

void main() {
    vec3 hdrColor = Lgt.ambientIntensity.rgb * Mtl.diffuseColor.rgb;
    for (int i = 0; i < 3; ++i) {
        hdrColor += CalculateLight(Lgt.lights[i]);
    }

    outputColor = vec4(hdrColor / (hdrColor + vec3(1.0f)), 1.0f);
    outputColor = sqrt(outputColor);
}
