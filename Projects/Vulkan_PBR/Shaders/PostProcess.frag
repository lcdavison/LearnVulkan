#version 450 core

layout (input_attachment_index = 0u, set = 0u, binding = 0u) uniform subpassInput SceneColour;

layout (location = 0u) out vec4 OutputColour;

mat3x3 SRGBToXYZ = mat3x3(0.412391f, 0.357584f, 0.180481f,
                          0.212639f, 0.715169f, 0.072192f,
                          0.019331f, 0.119195f, 0.950532f);

mat3x3 XYZToSRGB = mat3x3( 3.240970f, -1.537383f, -0.498611f,
                          -0.969244f,  1.875968f,  0.041555f,
                           0.055630f, -0.203977f,  1.056972f);

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

    vec3 LinearXYZ = SRGBToXYZ * LinearRGB;
    vec2 Chromaticity = vec2(LinearXYZ.xy) / (LinearXYZ.x + LinearXYZ.y + LinearXYZ.z);

    float ToneMappedLuminance = 1.0f - exp(-LinearXYZ.y * 0.5f);
    //float ToneMappedLuminance = LinearXYZ.y / (LinearXYZ.y + 1.0f);

    LinearXYZ.x = ToneMappedLuminance * (Chromaticity.x / Chromaticity.y);
    LinearXYZ.y = ToneMappedLuminance;
    LinearXYZ.z = ToneMappedLuminance * (1.0f - Chromaticity.x - Chromaticity.y) / Chromaticity.y;

    LinearRGB = XYZToSRGB * LinearXYZ;

    vec3 OutputRGB = LinearRGBToSRGB(LinearRGB);

    OutputColour = vec4(OutputRGB, 1.0f);
}