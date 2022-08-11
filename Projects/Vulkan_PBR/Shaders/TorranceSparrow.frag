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

struct MaterialInputs
{
    vec4 SpecularReflectanceAndRoughness;
    vec4 DiffuseReflectanceAndAmbientOcclusion;
    vec4 MacroNormalWSAndCosineOfViewAngleSN;
};

vec3 SRGBToLinearRGBApproximate(vec3 SRGB)
{
    return pow(SRGB, vec3(2.2f));
}

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

vec3 EvaluateTorranceSparrowBRDF(MaterialInputs Material, vec3 MicroNormalWS, vec3 LightDirectionWS, vec3 ViewDirectionWS, float CosineOfLightAngleSN, float FresnelReflectance)
{
    float CosineOfViewAngleSN = max(Material.MacroNormalWSAndCosineOfViewAngleSN.w, 0.0f);
    float CosineOfViewAngleFN = max(dot(MicroNormalWS, ViewDirectionWS), 0.0f);

    float CosineOfLightAngleFN = max(dot(MicroNormalWS, LightDirectionWS), 0.0f);

    float BeckmannDistribution = EvaluateBeckmannDistribution(MicroNormalWS, Material.MacroNormalWSAndCosineOfViewAngleSN.xyz, Material.SpecularReflectanceAndRoughness.w);
    float GeometricFactor = (CosineOfLightAngleFN * CosineOfViewAngleFN) 
                          / (1.0f + EvaluateGeometricFactor(CosineOfLightAngleSN, Material.SpecularReflectanceAndRoughness.w) 
                                  + EvaluateGeometricFactor(CosineOfViewAngleSN, Material.SpecularReflectanceAndRoughness.w));

    return Material.SpecularReflectanceAndRoughness.xyz * BeckmannDistribution * GeometricFactor * FresnelReflectance / (4.0f * abs(dot(Material.MacroNormalWSAndCosineOfViewAngleSN.xyz, LightDirectionWS)) * abs(Material.MacroNormalWSAndCosineOfViewAngleSN.w));
}

vec3 EvaluateLighting(vec3 LinearRGB, vec3 LightDirectionWS, vec3 ViewDirectionWS, vec3 LightRadiance, MaterialInputs Material)
{
    float CosineOfLightAngleSN = dot(LightDirectionWS, Material.MacroNormalWSAndCosineOfViewAngleSN.xyz);

    if (CosineOfLightAngleSN >= 0.001f)
    {
        vec3 MicroNormalWS = normalize(LightDirectionWS + ViewDirectionWS);

        float CosineOfViewAngleFN = max(dot(MicroNormalWS, ViewDirectionWS), 0.0f);
        float CosineOfLightAngleFN = max(dot(MicroNormalWS, LightDirectionWS), 0.0f);

        float FresnelLightAngle = EvaluateFresnelReflectance(CosineOfLightAngleFN);
        float FresnelViewAngle = EvaluateFresnelReflectance(CosineOfViewAngleFN);

        vec3 DiffuseBRDF = (1.0f - FresnelLightAngle) * (1.0f - FresnelViewAngle) * EvaluateDiffuseBRDF(Material.DiffuseReflectanceAndAmbientOcclusion.xyz);
        vec3 SpecularBRDF = EvaluateTorranceSparrowBRDF(Material, MicroNormalWS, LightDirectionWS, ViewDirectionWS, CosineOfLightAngleSN, FresnelLightAngle);

        LinearRGB += LightRadiance * CosineOfLightAngleSN * (DiffuseBRDF + SpecularBRDF);
    }

    return LinearRGB;
}

void main()
{
    vec3 ViewDirectionWS = normalize(ViewPositionWS - FragmentPositionWS);

    MaterialInputs Material;
    Material.SpecularReflectanceAndRoughness.xyz = SRGBToLinearRGBApproximate(texture(sampler2D(SpecularTexture, AnisotropicSampler), FragmentUV.xy).rgb);
    Material.SpecularReflectanceAndRoughness.w = 1.0f - texture(sampler2D(GlossTexture, LinearSampler), FragmentUV.xy).r;
    Material.SpecularReflectanceAndRoughness.w *= Material.SpecularReflectanceAndRoughness.w;

    Material.DiffuseReflectanceAndAmbientOcclusion.xyz = SRGBToLinearRGBApproximate(texture(sampler2D(DiffuseTexture, AnisotropicSampler), FragmentUV.xy).rgb);
    Material.DiffuseReflectanceAndAmbientOcclusion.w = texture(sampler2D(AOTexture, LinearSampler), FragmentUV.xy).r;

    Material.MacroNormalWSAndCosineOfViewAngleSN.xyz = normalize(FragmentNormalWS);
    Material.MacroNormalWSAndCosineOfViewAngleSN.w = dot(Material.MacroNormalWSAndCosineOfViewAngleSN.xyz, ViewDirectionWS);

    const vec3 kPointLightPositionWS = vec3(25.0f, 100.0f, 200.0f);
    vec3 PointLightDirectionWS = kPointLightPositionWS - FragmentPositionWS;
    vec3 PointLightRadiance = 5000.0f * vec3(1.0f, 1.0f, 1.0f) * 64.0f / dot(PointLightDirectionWS, PointLightDirectionWS);
    PointLightDirectionWS = normalize(PointLightDirectionWS);

    const vec3 kSunDirectionWS = -1.0f * vec3(-1.0f, -1.0f, 0.0f);

    vec3 LinearRGB = 0.25f * Material.DiffuseReflectanceAndAmbientOcclusion.w * EvaluateDiffuseBRDF(Material.DiffuseReflectanceAndAmbientOcclusion.xyz);
    LinearRGB = EvaluateLighting(LinearRGB, PointLightDirectionWS, ViewDirectionWS, PointLightRadiance, Material);
    LinearRGB = EvaluateLighting(LinearRGB, kSunDirectionWS, ViewDirectionWS, vec3(8.0f), Material);

    FragmentColour = vec4(LinearRGB, 1.0f);
}