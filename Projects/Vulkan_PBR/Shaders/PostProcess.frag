#version 450 core

layout (input_attachment_index = 0u, set = 0u, binding = 0u) uniform subpassInput SceneColour;

layout (location = 0u) out vec4 OutputColour;

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

void main()
{
    vec3 LinearRGB = subpassLoad(SceneColour).rgb;

    vec3 LinearXYZ = RGBToXYZ * LinearRGB;
    vec2 Chromaticity = vec2(LinearXYZ.xy) / (LinearXYZ.x + LinearXYZ.y + LinearXYZ.z);

    float ToneMappedLuminance = 1.0f - exp(-LinearXYZ.y * 0.4f);
    //float ToneMappedLuminance = LinearXYZ.y / (LinearXYZ.y + 1.0f);

    LinearXYZ.x = ToneMappedLuminance * (Chromaticity.x / Chromaticity.y);
    LinearXYZ.y = ToneMappedLuminance;
    LinearXYZ.z = ToneMappedLuminance * (1.0f - Chromaticity.x - Chromaticity.y) / Chromaticity.y;

    LinearRGB = XYZToRGB * LinearXYZ;

    vec3 OutputRGB = LinearRGBToSRGB(LinearRGB);

    OutputColour = vec4(OutputRGB, 1.0f);
}