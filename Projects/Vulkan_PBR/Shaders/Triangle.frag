#version 450

layout (location = 0) in vec3 FragmentColour;

layout (location = 0) out vec4 OutputFragmentColour;

mat3x3 RGBToXYZ = mat3x3(0.49000f, 0.31000f, 0.20000f,
                         0.17697f, 0.81240f, 0.01063f,
                         0.00000f, 0.01000f, 0.99000f);

mat3x3 XYZToRGB = mat3x3( 2.36461385f, -0.89654057f, -0.46807328f,
                         -0.51516621f,  1.42640810f,  0.08875810f,
                          0.00520370f, -0.01440816f,  1.00920446f);

const float kGammaCorrectionExponent = 1.0f / 2.2f;

/* 
*   There really isn't a need to do this (Vulkan implementations have *_SRGB surface formats).
*   Bit of fun though.
*/
vec3 LinearRGBToSRGB(vec3 LinearRGB)
{
    const float SRGBTransferConstant = 0.0031308f;

    vec3 SRGB = vec3(0.0f);
    SRGB.r = LinearRGB.r > SRGBTransferConstant ? 
                           1.055f * pow(LinearRGB.r, kGammaCorrectionExponent) - 0.055f :
                           12.92f * LinearRGB.r;

    SRGB.g = LinearRGB.g > SRGBTransferConstant ? 
                           1.055f * pow(LinearRGB.g, kGammaCorrectionExponent) - 0.055f :
                           12.92f * LinearRGB.g;

    SRGB.b = LinearRGB.b > SRGBTransferConstant ? 
                           1.055f * pow(LinearRGB.b, kGammaCorrectionExponent) - 0.055f :
                           12.92f * LinearRGB.b;

    return SRGB;
}

void main()
{
    vec3 LinearRGB = FragmentColour.rgb;
    vec3 CIEXYZ = RGBToXYZ * LinearRGB;

    float ColourScaleFactor = 1.0f / (CIEXYZ.x + CIEXYZ.y + CIEXYZ.z + 1.0f);

    vec3 CIExyY = vec3(CIEXYZ.xy * ColourScaleFactor, CIEXYZ.y);
    
    float ToneMappedLuminance = CIExyY.z / (CIExyY.z + 1.0f);

    CIEXYZ.x = (ToneMappedLuminance * CIEXYZ.x) / CIEXYZ.y;
    CIEXYZ.y = ToneMappedLuminance;
    CIEXYZ.z = (ToneMappedLuminance * (1.0f - CIEXYZ.x - CIEXYZ.y)) / CIEXYZ.y;

    LinearRGB = XYZToRGB * CIEXYZ;

    vec3 SRGBOutput = LinearRGBToSRGB(LinearRGB);

    OutputFragmentColour = vec4(SRGBOutput, 1.0f);
}