#version 450

layout (location = 0) in vec3 FragmentColour;
layout (location = 1) in vec3 FragmentNormal;
layout (location = 2) in vec3 FragmentWorldPosition;

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
    const vec3 SkyColour = vec3(0.6f, 0.4f, 0.0f);
    const vec3 GroundColour = vec3(0.0f, 0.3f, 0.7f);

    float NormalizedZ = (0.5f * Normal.z) + 0.5f;

    return (1.0f - NormalizedZ) * GroundColour + NormalizedZ * SkyColour;
}

const vec3 kLightRadiance = vec3(10.0f, 10.0f, 10.0f);
const vec3 kLightPosition = vec3(-100.0f, 100.0f, 10.0f);

vec3 GetDiffuseReflection(vec3 DiffuseAlbedo, vec3 Normal)
{
    const float kPI = 3.1415f;

    vec3 BRDF = DiffuseAlbedo / kPI;
    //vec3 LightDirection = normalize(vec3(-0.5f, 0.0f, -0.5f));

    vec3 SurfaceToLight = kLightPosition - FragmentWorldPosition;
    vec3 LightDirection = normalize(SurfaceToLight);
    vec3 IncomingRadiance = 1000.0f * (kLightRadiance / dot(SurfaceToLight, SurfaceToLight));

    return IncomingRadiance * BRDF * max(dot(Normal, LightDirection), 0.0f);
}

vec3 GetSpecularReflection(vec3 Normal)
{
    const float kGlossiness = 50.0f;

    vec3 SurfaceToLight = kLightPosition - FragmentWorldPosition;
    vec3 LightDirection = normalize(SurfaceToLight);
    vec3 IncomingRadiance = 1000.0f * (kLightRadiance / dot(SurfaceToLight, SurfaceToLight));

    /* Weirdly reflect intrinsic needs incoming direction to be flipped */
    vec3 ReflectionDirection = normalize(reflect(-LightDirection, Normal));

    /* This needs to use the camera position */
    vec3 SurfaceToCamera = normalize(-FragmentWorldPosition);
    float BRDF = max(pow(dot(SurfaceToCamera, ReflectionDirection), kGlossiness), 0.0f);

    return IncomingRadiance * BRDF * max(dot(Normal, LightDirection), 0.0f);
}

void main()
{
    vec3 LinearRGB = FragmentColour.rgb;
    LinearRGB = 0.05f * GetHemisphereAmbientColour(FragmentNormal)
              + GetDiffuseReflection(FragmentColour, FragmentNormal)
              + GetSpecularReflection(FragmentNormal);

    vec3 OutputColour = LinearRGBToSRGB(LinearRGB);

    OutputFragmentColour = vec4(OutputColour, 1.0f);
}