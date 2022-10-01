#version 450 core

layout (location = 0) in vec3 FragmentPositionWS;
layout (location = 1) in vec3 FragmentPositionVS;
layout (location = 2) in vec3 FragmentNormalWS;
layout (location = 3) in vec4 FragmentTangentWS;
layout (location = 4) in vec3 FragmentUV;
layout (location = 5) in vec3 ViewPositionWS;

layout (set = 1, binding = 1) uniform texture2D DiffuseTexture;
layout (set = 1, binding = 2) uniform texture2D SpecularTexture;
layout (set = 1, binding = 3) uniform texture2D NormalTexture;
layout (set = 1, binding = 4) uniform texture2D GlossTexture;
layout (set = 1, binding = 5) uniform texture2D AOTexture;
layout (set = 1, binding = 6) uniform sampler AnisotropicSampler;
layout (set = 1, binding = 7) uniform sampler LinearSampler;

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

vec3 EvaluateFresnelReflectance(float CosineOfViewAndNormal, vec3 SpecularReflectanceNormalIncidence)
{
    return SpecularReflectanceNormalIncidence + (1.0f - SpecularReflectanceNormalIncidence) * pow(1.0f - CosineOfViewAndNormal, 5.0f);
}

float EvaluateBeckmannDistribution(vec3 MicroNormalWS, vec3 MacroNormalWS, float Roughness)
{
    const float kPi = 3.1415926f;

    float RoughnessSquared = Roughness * Roughness;

    float CosineOfMicrofacetAngleSN = dot(MicroNormalWS, MacroNormalWS);
    float CosineOfMicrofacetAngleSNSquared = CosineOfMicrofacetAngleSN * CosineOfMicrofacetAngleSN;

    float TangentOfMicrofacetAngleSquared = (CosineOfMicrofacetAngleSNSquared - 1.0f) / (CosineOfMicrofacetAngleSNSquared * RoughnessSquared);

    return (max(CosineOfMicrofacetAngleSN, 0.0f) / (kPi * CosineOfMicrofacetAngleSNSquared * CosineOfMicrofacetAngleSNSquared * RoughnessSquared)) * exp(TangentOfMicrofacetAngleSquared);
}

float EvaluateGeometricFactor(float CosineOfDirectionSN, float Roughness)
{
    float Alpha = CosineOfDirectionSN / (Roughness * sqrt(1.0f - CosineOfDirectionSN * CosineOfDirectionSN));
    float AlphaSquared = Alpha * Alpha;

    return Alpha < 1.6f 
           ? (1.0f - Alpha * 1.259f + 0.396f * AlphaSquared) / (3.535f * Alpha + 2.181f * AlphaSquared)
           : 0.0f;
}

vec3 EvaluateTorranceSparrowBRDF(MaterialInputs Material, vec3 MicroNormalWS, vec3 LightDirectionWS, vec3 ViewDirectionWS, float CosineOfLightAngleSN, float CosineOfViewAngleFN, vec3 FresnelReflectance)
{
    float CosineOfViewAngleSN = abs(Material.MacroNormalWSAndCosineOfViewAngleSN.w) + 0.0001f;
    float CosineOfLightAngleFN = max(dot(MicroNormalWS, LightDirectionWS), 0.0f);

    float BeckmannDistribution = EvaluateBeckmannDistribution(MicroNormalWS, Material.MacroNormalWSAndCosineOfViewAngleSN.xyz, Material.SpecularReflectanceAndRoughness.w);
    float GeometricFactor = (CosineOfLightAngleFN * CosineOfViewAngleFN) 
                          / (1.0f + EvaluateGeometricFactor(CosineOfLightAngleSN, Material.SpecularReflectanceAndRoughness.w) 
                                  + EvaluateGeometricFactor(CosineOfViewAngleSN, Material.SpecularReflectanceAndRoughness.w));

    return GeometricFactor * BeckmannDistribution * FresnelReflectance / (4.0f * abs(CosineOfLightAngleSN) * CosineOfViewAngleSN);
}

vec3 EvaluateLighting(vec3 LinearRGB, vec3 LightDirectionWS, vec3 ViewDirectionWS, vec3 LightRadiance, MaterialInputs Material)
{
    float CosineOfLightAngleSN = dot(LightDirectionWS, Material.MacroNormalWSAndCosineOfViewAngleSN.xyz);

    if (CosineOfLightAngleSN >= 0.001f)
    {
        vec3 MicroNormalWS = normalize(LightDirectionWS + ViewDirectionWS);

        float CosineOfViewAngleFN = abs(dot(MicroNormalWS, ViewDirectionWS));
        float CosineOfLightAngleFN = max(dot(MicroNormalWS, LightDirectionWS), 0.0f);

        vec3 FresnelReflectance = EvaluateFresnelReflectance(CosineOfViewAngleFN, Material.SpecularReflectanceAndRoughness.xyz);

        vec3 DiffuseBRDF = Material.DiffuseReflectanceAndAmbientOcclusion.xyz;
        vec3 SpecularBRDF = EvaluateTorranceSparrowBRDF(Material, MicroNormalWS, LightDirectionWS, ViewDirectionWS, CosineOfLightAngleSN, CosineOfViewAngleFN, FresnelReflectance);

        LinearRGB += LightRadiance * CosineOfLightAngleSN * (DiffuseBRDF + SpecularBRDF);
    }

    return LinearRGB;
}

vec3 EvaluateNormal()
{
    vec3 NormalWS = normalize(FragmentNormalWS);

    // Make sure that the tangent is perpendicular
    vec3 TangentWS = normalize(FragmentTangentWS.xyz - dot(FragmentTangentWS.xyz, NormalWS) * NormalWS);

    vec3 BitangentWS = FragmentTangentWS.w * cross(NormalWS, TangentWS);

    vec3 SampledNormal = texture(sampler2D(NormalTexture, LinearSampler), FragmentUV.xy).xyz * 2.0f - vec3(1.0f);
    return normalize(SampledNormal.x * TangentWS + SampledNormal.y * BitangentWS + SampledNormal.z * NormalWS);
}

void main()
{
    vec3 ViewDirectionWS = normalize(ViewPositionWS - FragmentPositionWS);

    MaterialInputs Material;
    Material.SpecularReflectanceAndRoughness.xyz = SRGBToLinearRGBApproximate(texture(sampler2D(SpecularTexture, AnisotropicSampler), FragmentUV.xy).rgb);
    Material.SpecularReflectanceAndRoughness.w = 1.0f - texture(sampler2D(GlossTexture, LinearSampler), FragmentUV.xy).r;
    Material.SpecularReflectanceAndRoughness.w = max(Material.SpecularReflectanceAndRoughness.w, 0.04f);

    Material.DiffuseReflectanceAndAmbientOcclusion.xyz = SRGBToLinearRGBApproximate(texture(sampler2D(DiffuseTexture, AnisotropicSampler), FragmentUV.xy).rgb);
    Material.DiffuseReflectanceAndAmbientOcclusion.w = texture(sampler2D(AOTexture, LinearSampler), FragmentUV.xy).r;

    Material.MacroNormalWSAndCosineOfViewAngleSN.xyz = EvaluateNormal();
    Material.MacroNormalWSAndCosineOfViewAngleSN.w = dot(Material.MacroNormalWSAndCosineOfViewAngleSN.xyz, ViewDirectionWS);

    const float kPointLightRadius = 2.0f;
    const vec3 kPointLightRadiance = vec3(250.0f);
    const vec3 kPointLightPositionWS = vec3(25.0f, 100.0f, 200.0f);
    vec3 PointLightDirectionWS = kPointLightPositionWS - FragmentPositionWS;
    vec3 PointLightRadiance = kPointLightRadiance * kPointLightRadius / (dot(PointLightDirectionWS, PointLightDirectionWS) + 0.001f);
    PointLightDirectionWS = normalize(PointLightDirectionWS);

    const vec3 kSunDirectionWS = normalize(-1.0f * vec3(-1.0f, 1.0f, -1.0f));
    const vec3 kSunRadiance = vec3(4.0f);

    vec3 LinearRGB = vec3(Material.DiffuseReflectanceAndAmbientOcclusion.xyz * 0.1f);
    LinearRGB = EvaluateLighting(LinearRGB, PointLightDirectionWS, ViewDirectionWS, PointLightRadiance, Material);
    LinearRGB = EvaluateLighting(LinearRGB, kSunDirectionWS, ViewDirectionWS, kSunRadiance, Material);
    LinearRGB *= Material.DiffuseReflectanceAndAmbientOcclusion.w;

    FragmentColour = vec4(LinearRGB, 1.0f);
}