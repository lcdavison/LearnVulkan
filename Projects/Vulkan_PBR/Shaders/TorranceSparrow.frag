#version 450

layout (location = 1) in vec3 FragmentNormalWS;
layout (location = 2) in vec3 FragmentPositionWS;
layout (location = 3) in vec3 FragmentUV;
layout (location = 4) in vec3 ViewPositionWS;

layout (set = 0, binding = 2) uniform texture2D DiffuseTexture;
layout (set = 0, binding = 3) uniform texture2D AOTexture;
layout (set = 0, binding = 4) uniform texture2D SpecularTexture;
layout (set = 0, binding = 5) uniform texture2D GlossTexture;
layout (set = 0, binding = 6) uniform sampler AnisotropicSampler;
layout (set = 0, binding = 7) uniform sampler LinearSampler;

layout (location = 0) out vec4 FragmentColour;

 /*
        SN = Surface Normal
        FN = Facet Normal
*/

struct MaterialData
{
    vec4 SpecularReflectanceAndRoughness;
    vec3 DiffuseReflectance;
    vec3 MicroNormalWS;
    vec3 MacroNormalWS;

    float CosineOfViewAngleSN;
    float CosineOfViewAngleFN;

    float CosineOfLightAngleSN;
    float CosineOfLightAngleFN;
};

vec3 EvaluateDiffuseBRDF(vec3 Albedo)
{
    const float kInversePI = 1.0f / 3.1415926f;
    return Albedo * kInversePI;
}

float EvaluateFresnelReflectance(float CosineOfViewAndNormal)
{
    const float AirIOR = 1.0f;
    const float LowIOR = 1.5f;

    float IORFactor = (AirIOR - LowIOR) / (AirIOR + LowIOR);
    IORFactor *= IORFactor;

    return IORFactor + (1.0f - IORFactor) * pow((1.0f - CosineOfViewAndNormal), 5.0f);
}

float EvaluateBeckmannDistribution(vec3 MicroNormalWS, vec3 MacroNormalWS, float Roughness)
{
    const float kPi = 3.1415926f;

    float RootMeanSlope = sqrt(2.0f) * Roughness;

    float CosineOfMicrofacetAngleSN = dot(MicroNormalWS, MacroNormalWS);
    float CosineOfMicrofacetAngleSNSquared = CosineOfMicrofacetAngleSN * CosineOfMicrofacetAngleSN;

    float TangentOfMicrofacetAngleSquared = (CosineOfMicrofacetAngleSNSquared - 1.0f) / CosineOfMicrofacetAngleSNSquared;

    float InverseRootMeanSquaredSlope = 1.0f / (RootMeanSlope * RootMeanSlope);

    return (max(CosineOfMicrofacetAngleSN, 0.0f) / (kPi * CosineOfMicrofacetAngleSNSquared * CosineOfMicrofacetAngleSNSquared)) * exp(TangentOfMicrofacetAngleSquared * InverseRootMeanSquaredSlope) * InverseRootMeanSquaredSlope;
}

float EvaluateGeometricFactor(float CosineOfDirectionSN, float Roughness)
{
    float Alpha = CosineOfDirectionSN / (Roughness * sqrt(1.0f - CosineOfDirectionSN * CosineOfDirectionSN));

    return Alpha < 1.6f 
           ? (1.0f - Alpha * 1.259f + 0.396f * Alpha * Alpha) / (3.535f * Alpha + 2.181f * Alpha * Alpha)
           : 0.0f;
}

vec3 EvaluateTorranceSparrowBRDF(vec3 SpecularReflectance, vec3 MacroNormal, vec3 MicroNormal, vec3 LightDirection, float CosineOfLightAngleSN, vec3 ViewDirection, float FresnelReflectance, float Roughness)
{
    float CosineOfViewAngleSN = max(dot(MacroNormal, ViewDirection), 0.0f);
    float CosineOfViewAngleFN = max(dot(MicroNormal, ViewDirection), 0.0f);

    float CosineOfLightAngleFN = max(dot(MicroNormal, LightDirection), 0.0f);

    float BeckmannDistribution = EvaluateBeckmannDistribution(MicroNormal, MacroNormal, Roughness);
    float GeometricFactor = (CosineOfLightAngleFN * CosineOfViewAngleFN) 
                          / (1.0f + EvaluateGeometricFactor(CosineOfLightAngleSN, Roughness) + EvaluateGeometricFactor(CosineOfViewAngleSN, Roughness));

    return SpecularReflectance * BeckmannDistribution * GeometricFactor * FresnelReflectance / (4.0f * abs(dot(MacroNormal, LightDirection)) * abs(dot(MacroNormal, ViewDirection)));
}

void main()
{
    /* TODO: Put BRDF inputs into a struct */

    // Specular Map = Specular Reflectance
    // Gloss Map = Roughness
    // No Metallic map
    // Metallic 0.0f

    const vec3 kLightPositionWS = vec3(25.0f, 100.0f, 200.0f);
    vec3 LightDirectionWS = kLightPositionWS - FragmentPositionWS;
    vec3 LightRadiance = 5000.0f * vec3(1.0f, 1.0f, 1.0f) * 64.0f / dot(LightDirectionWS, LightDirectionWS);

    LightDirectionWS = normalize(LightDirectionWS);

    vec3 MacroNormalWS = normalize(FragmentNormalWS);

    float CosineOfLightAngleSN = max(dot(LightDirectionWS, MacroNormalWS), 0.0f);

    vec3 DiffuseReflectance = pow(texture(sampler2D(DiffuseTexture, AnisotropicSampler), FragmentUV.xy).rgb, vec3(2.2f));
    float AmbientOcclusion = texture(sampler2D(AOTexture, LinearSampler), FragmentUV.xy).r;
    vec3 LinearRGB = EvaluateDiffuseBRDF(DiffuseReflectance) * AmbientOcclusion;

    if (CosineOfLightAngleSN >= 0.001f)
    {
        vec3 SpecularReflectance = pow(texture(sampler2D(SpecularTexture, AnisotropicSampler), FragmentUV.xy).rgb, vec3(2.2f));
        float Roughness = 1.0f - texture(sampler2D(GlossTexture, LinearSampler), FragmentUV.xy).r;

        vec3 ViewDirectionWS = normalize(ViewPositionWS - FragmentPositionWS);
        vec3 MicroNormalWS = normalize(LightDirectionWS + ViewDirectionWS);

        float CosineOfViewAngleFN = max(dot(MicroNormalWS, ViewDirectionWS), 0.0f);
        float CosineOfLightAngleFN = max(dot(MicroNormalWS, LightDirectionWS), 0.0f);

        float FresnelLightAngle = EvaluateFresnelReflectance(CosineOfLightAngleFN);
        float FresnelViewAngle = EvaluateFresnelReflectance(CosineOfViewAngleFN);

        vec3 DiffuseBRDF = (1.0f - FresnelLightAngle) * (1.0f - FresnelViewAngle) * EvaluateDiffuseBRDF(DiffuseReflectance);
        vec3 SpecularBRDF = EvaluateTorranceSparrowBRDF(SpecularReflectance, MacroNormalWS, MicroNormalWS, LightDirectionWS, CosineOfLightAngleSN, ViewDirectionWS, FresnelLightAngle, Roughness * Roughness);

        LinearRGB += LightRadiance * CosineOfLightAngleFN * (DiffuseBRDF + SpecularBRDF);
    }

    FragmentColour = vec4(LinearRGB, 1.0f);
}