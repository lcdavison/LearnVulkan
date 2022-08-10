#version 450

layout (location = 0) in vec3 FragmentColour;
layout (location = 1) in vec3 FragmentNormal;
layout (location = 2) in vec3 FragmentWorldPosition;
layout (location = 3) in vec3 FragmentUV;
layout (location = 4) in vec3 ViewPositionWorldSpace;

layout (set = 0, binding = 2) uniform texture2D DiffuseTexture;
layout (set = 0, binding = 3) uniform sampler DiffuseTextureSampler;

layout (location = 0) out vec4 OutputFragmentColour;

/* Using Z-Up */
vec3 GetHemisphereAmbientColour(vec3 Normal, vec3 SkyColour, vec3 GroundColour)
{
    float NormalizedZ = (0.5f * Normal.z) + 0.5f;

    return (1.0f - NormalizedZ) * GroundColour + NormalizedZ * SkyColour;
}

const vec3 kLightRadiance = vec3(1.0f, 1.0f, 1.0f);
const vec3 kLightPosition = vec3(0.0f, 100.0f, 100.0f);

vec3 GetDiffuseReflection(vec3 SurfaceReflectance)
{
    const float kInversePI = 1.0f / 3.1415926f;
    return SurfaceReflectance * kInversePI;
}

vec3 GetSpecularReflection(vec3 LightDirection, vec3 SurfaceNormal, vec3 SurfaceToView)
{
    const float kGlossiness = 100.0f;

    vec3 MicroSurfaceNormal = normalize(LightDirection + SurfaceToView);

    return ((kGlossiness + 2.0f) / 8.0f) * vec3(pow(max(dot(SurfaceNormal, MicroSurfaceNormal), 0.0f), kGlossiness));
}

float EvaluateFresnel(float CosineOfViewAndNormal, float IndexOfRefraction)
{
    float IORFactor = (1.0f - IndexOfRefraction) / (1.0f + IndexOfRefraction);
    IORFactor *= IORFactor;

    return IORFactor + (1.0f - IORFactor) * pow((1.0f - CosineOfViewAndNormal), 5.0f);
}

void main()
{
    vec3 SurfaceNormal = normalize(FragmentNormal);
    vec3 SurfaceToView = normalize(ViewPositionWorldSpace - FragmentWorldPosition);

    vec3 SurfaceToLight = kLightPosition - FragmentWorldPosition;
    vec3 LightDirection = normalize(SurfaceToLight);

    const float SourceRadius = 2.0f;
    vec3 IncomingRadiance = 10000.0f * SourceRadius * SourceRadius * (kLightRadiance / dot(SurfaceToLight, SurfaceToLight));

    float CosineOfLightAngle = max(dot(LightDirection, SurfaceNormal), 0.0f);
    float CosineOfViewAngle = max(dot(SurfaceToView, SurfaceNormal), 0.0f);

    vec3 DiffuseReflectance = texture(sampler2D(DiffuseTexture, DiffuseTextureSampler), FragmentUV.xy).rgb;

    vec3 LinearRGB = 0.1f * DiffuseReflectance;//0.1f * GetHemisphereAmbientColour(SurfaceNormal, FragmentColour, vec3(0.0f, 0.3f, 0.7f));

    float Fresnel = EvaluateFresnel(CosineOfViewAngle, 1.5f);
    float DiffuseFresnel = EvaluateFresnel(CosineOfLightAngle, 1.5f);
    LinearRGB += IncomingRadiance * CosineOfLightAngle * (((1.0f - Fresnel) * GetDiffuseReflection(DiffuseReflectance)) + Fresnel * GetSpecularReflection(LightDirection, SurfaceNormal, SurfaceToView));

    OutputFragmentColour = vec4(LinearRGB, 1.0f);
}