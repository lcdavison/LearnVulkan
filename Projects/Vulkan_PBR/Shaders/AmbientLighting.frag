#version 450

layout (location = 0) in vec3 FragmentColour;
layout (location = 1) in vec3 FragmentNormal;
layout (location = 2) in float FragmentBrightness;

layout (location = 0) out vec4 OutputFragmentColour;

mat3x3 RGBToXYZ = mat3x3(0.49000f, 0.31000f, 0.20000f,
                         0.17697f, 0.81240f, 0.01063f,
                         0.00000f, 0.01000f, 0.99000f);

mat3x3 XYZToRGB = mat3x3( 2.36461385f, -0.89654057f, -0.46807328f,
                         -0.51516621f,  1.42640810f,  0.08875810f,
                          0.00520370f, -0.01440816f,  1.00920446f);

const float kGammaCorrectionExponent = 1.0f / 2.4f;

float LinearRGBToSRGB(float LinearValue)
{
    const float SRGBTransferConstant = 0.0031308f;

    return LinearValue > SRGBTransferConstant ?
           1.055f * pow(LinearValue, kGammaCorrectionExponent) - 0.055f :
           12.92f * LinearValue;
}

vec3 LinearRGBToSRGB(vec3 LinearRGB)
{
    vec3 SRGB = vec3(0.0f);

    SRGB.r = LinearRGBToSRGB(LinearRGB.r);
    SRGB.g = LinearRGBToSRGB(LinearRGB.g);
    SRGB.b = LinearRGBToSRGB(LinearRGB.b);

    return SRGB;
}

/* Using Z-Up */
vec3 GetHemisphereAmbientColour(vec3 Normal)
{
    const vec3 SkyColour = vec3(0.0f, 0.3f, 0.7f);
    const vec3 GroundColour = vec3(0.3f, 0.7f, 0.0f);

    float NormalizedZ = (0.5f * Normal.z) + 0.5f;

    return (1.0f - NormalizedZ) * GroundColour + NormalizedZ * SkyColour;
}

vec3 GetDiffuseReflection(vec3 DiffuseAlbedo, vec3 Normal)
{
    const float kPI = 3.1415f;

    vec3 BRDF = DiffuseAlbedo / kPI;
    vec3 IncomingRadiance = vec3(1.0f, 0.0f, 0.0f);
    vec3 LightDirection = normalize(vec3(-0.5f, 0.0f, -0.5f));

    return IncomingRadiance * BRDF * dot(Normal, -LightDirection);
}

void main()
{
    vec3 LinearRGB = FragmentColour.rgb;
    LinearRGB = 0.25f * GetHemisphereAmbientColour(FragmentNormal)
    + GetDiffuseReflection(FragmentColour, FragmentNormal);

    vec3 OutputColour = LinearRGBToSRGB(LinearRGB);
    //vec3 OutputColour = LinearRGBToRec709(LinearRGB);

    //OutputColour += FragmentBrightness;

    OutputFragmentColour = vec4(OutputColour, 1.0f);
}