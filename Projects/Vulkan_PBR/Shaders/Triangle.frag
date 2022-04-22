#version 450

layout (location = 0) in vec3 FragmentColour;

layout (location = 0) out vec4 OutputFragmentColour;

mat3x3 RGBToXYZ = mat3x3(0.49000f, 0.31000f, 0.20000f,
                         0.17697f, 0.81240f, 0.01063f,
                         0.00000f, 0.01000f, 0.99000f);

mat3x3 XYZToRGB = mat3x3( 2.36461385f, -0.89654057f, -0.46807328f,
                         -0.51516621f,  1.42640810f,  0.08875810f,
                          0.00520370f, -0.01440816f,  1.00920446f);

const float kGammaCorrectionExponent = 1.0f / 2.4f;

/* 
*   There really isn't a need to do this (Vulkan implementations have *_SRGB surface formats).
*   Bit of fun though.
*/
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

float LinearRGBToRec709(float LinearValue)
{
    const float Rec709TransferConstant = 0.018f;

    return LinearValue < 0.018f ?
           4.500f * LinearValue :
           1.099f * pow(LinearValue, 0.45f) - 0.099f;
}

vec3 LinearRGBToRec709(vec3 LinearRGB)
{
    vec3 Rec709 = vec3(0.0f);

    Rec709.r = LinearRGBToRec709(LinearRGB.r);
    Rec709.g = LinearRGBToRec709(LinearRGB.g);
    Rec709.b = LinearRGBToRec709(LinearRGB.b);

    return Rec709;
}

vec3 ToneMap(vec3 LinearRGB)
{
    vec3 CIEXYZ = RGBToXYZ * LinearRGB;

    float ColourScaleFactor = 1.0f / (CIEXYZ.x + CIEXYZ.y + CIEXYZ.z + 1.0f);

    vec3 CIExyY = vec3(CIEXYZ.xy * ColourScaleFactor, CIEXYZ.y);
    
    float ToneMappedLuminance = CIExyY.z / (CIExyY.z + 1.0f);

    CIEXYZ.x = (ToneMappedLuminance * CIEXYZ.x) / CIEXYZ.y;
    CIEXYZ.y = ToneMappedLuminance;
    CIEXYZ.z = (ToneMappedLuminance * (1.0f - CIEXYZ.x - CIEXYZ.y)) / CIEXYZ.y;

    return XYZToRGB * CIEXYZ;
}

void main()
{
    vec3 LinearRGB = FragmentColour.rgb;
    //LinearRGB = ToneMap(LinearRGB);

    vec3 OutputColour = LinearRGBToSRGB(LinearRGB);

    OutputFragmentColour = vec4(OutputColour, 1.0f);
}