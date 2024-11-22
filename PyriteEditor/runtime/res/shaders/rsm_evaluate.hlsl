#include "incl/cbuffers.incl"
#include "incl/samplers.incl"
#include "incl/light_utils.incl"

//////////////////////////////////////////////////////////////////////////////////////////////////

struct VS_IN
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
};

struct VS_OUT
{
    float4 position : SV_Position;
    float3 worldPos : Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOOD0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

#define RSM_SAMPLE_COUNT 20

Texture2DArray RSM_WorldPositions;
Texture2DArray RSM_Normals;
Texture2DArray RSM_Flux;

Texture2DArray RSM_LowRes_WorldPositions;
Texture2DArray RSM_LowRes_Normals;
Texture2DArray RSM_LowRes_Flux;


cbuffer SingleLightBuffer
{
    Light sourceLight;
};

bool bFullTexture = false;
Texture2D LowResTexture;
float u_DistanceComparisonThreshold = 0.05;
float u_NormalComparisonThreshold = 0.05;
float2 u_FullTextureDimensions = float2(512, 512);

//////////////////////////////////////////////////////////////////////////////////////////////////

// see https://users.soe.ucsc.edu/~pang/160/s13/proposal/mijallen/proposal/media/p203-dachsbacher.pdf 
// equation (1) 

// -- We use the evaluated pixel transformed to light space uv's            
float3 evaluateRadianceInfluence(float3 x_worldposition, float3 x_normal, float2 p_screenSpace)
{
    const int DEBUG_CONST_INDEX_REMOVE_THIS = 0; 
    
    // -- Get the informations of p (the light pixel)
    float3 p_worldPosition  = RSM_WorldPositions.Sample(blitSamplerState, float3(p_screenSpace, DEBUG_CONST_INDEX_REMOVE_THIS)).rgb;
    float3 p_normal         = RSM_Normals       .Sample(blitSamplerState, float3(p_screenSpace, DEBUG_CONST_INDEX_REMOVE_THIS)).rgb;
    float3 p_flux           = RSM_Flux          .Sample(blitSamplerState, float3(p_screenSpace, DEBUG_CONST_INDEX_REMOVE_THIS)).rgb;

    // -- Apply equation 1
    float a = max(0, dot(p_normal, (x_worldposition - p_worldPosition)));
    float b = max(0, dot(x_normal, (p_worldPosition - x_worldposition)));
    float3 num = p_flux * a * b;
    
    float dist = length(x_worldposition - p_worldPosition);
    float dist2 = dist * dist;
    float inv_denom = 1.F / dist2;
    
    float3 irradiance = num * inv_denom;
    return irradiance;
}

// -- We use the evaluated pixel transformed to light space uv's            
float3 evaluateRadianceInfluence_LowRes(float3 x_worldposition, float3 x_normal, float2 p_screenSpace)
{
    const int DEBUG_CONST_INDEX_REMOVE_THIS = 0;
    
    // -- Get the informations of p (the light pixel)
    float3 p_worldPosition  =   RSM_LowRes_WorldPositions.Sample(blitSamplerState, float3(p_screenSpace, DEBUG_CONST_INDEX_REMOVE_THIS)).rgb;
    float3 p_normal         =   RSM_LowRes_Normals.Sample(blitSamplerState, float3(p_screenSpace, DEBUG_CONST_INDEX_REMOVE_THIS)).rgb;
    float3 p_flux           =   RSM_LowRes_Flux.Sample(blitSamplerState, float3(p_screenSpace, DEBUG_CONST_INDEX_REMOVE_THIS)).rgb;

    // -- Apply equation 1
    float a = max(0, dot(p_normal, (x_worldposition - p_worldPosition)));
    float b = max(0, dot(x_normal, (p_worldPosition - x_worldposition)));
    float3 num = p_flux * a * b;
    
    float dist = length(x_worldposition - p_worldPosition);
    float dist2 = dist * dist;
    float inv_denom = 1.F / dist2;
    
    float3 irradiance = num * inv_denom;
    return irradiance;
}


float random(float2 p)
{
    float2 K1 = float2(
        23.14069263277926, // e^pi (Gelfond's constant)
         2.665144142690225 // 2^sqrt(2) (Gelfond–Schneider constant)
    );
    return frac(cos(dot(p, K1)) * 12345.6789);
}

// see equation (3)
float2 PseudoRandomUVOffset(float2 uv)
{
    // Let eta1 and 2 be two uniformly distributed numbers.
    float eta_1 = random(uv);
    float eta_2 = random(uv * eta_1);

    return float2(
    eta_1 * sin(2 * 3.1415 * eta_2),
    eta_1 * cos(2 * 3.1415 * eta_2)
    );
    
}

float3 ComputeIndirectIlluminationForUVs(float2 uv, float3 x_worldPos, float3 x_normal)
{
    float3 indirect_illumination = float3(0, 0, 0);
    for (int x = 0; x < RSM_SAMPLE_COUNT; x++)
    {
        for (int y = 0; y < RSM_SAMPLE_COUNT; y++)
        {
            float2 offset = PseudoRandomUVOffset(float2(x, y));
            float2 pixel_vpl_uvs = uv + offset;
            float screenspace_distance = length(pixel_vpl_uvs - uv);
            
            float3 radiance = evaluateRadianceInfluence(x_worldPos, x_normal, pixel_vpl_uvs);
            radiance *= screenspace_distance;
            indirect_illumination += radiance;
        }
    }
    indirect_illumination /= RSM_SAMPLE_COUNT;
    return indirect_illumination;
}

float3 ComputeIndirectIlluminationForUVs_LowRes(float2 uv, float3 x_worldPos, float3 x_normal)
{
    float3 indirect_illumination = float3(0, 0, 0);
    for (int x = 0; x < RSM_SAMPLE_COUNT; x++)
    {
        for (int y = 0; y < RSM_SAMPLE_COUNT; y++)
        {
            float2 offset = PseudoRandomUVOffset(float2(x, y));
            float2 pixel_vpl_uvs = uv + offset;
            float screenspace_distance = length(pixel_vpl_uvs - uv);
            
            float3 radiance = evaluateRadianceInfluence_LowRes(x_worldPos, x_normal, pixel_vpl_uvs);
            radiance *= screenspace_distance;
            indirect_illumination += radiance;
        }
    }
    indirect_illumination /= RSM_SAMPLE_COUNT;
    return indirect_illumination;
}


float3 InterpolateIndirectIlluminationFromLowResolution(float2 uv)
{
    float3 res = float3(0, 0, 0);
    
    float2 offset;
    float2 texelSize = 1.F / float2(32, 32);
    
    offset = float2(1, -1);
    res += LowResTexture.Sample(blitSamplerState, uv + offset * texelSize);
    offset = float2(-1, -1);
    res += LowResTexture.Sample(blitSamplerState, uv + offset * texelSize);
    offset = float2(1, -1);
    res += LowResTexture.Sample(blitSamplerState, uv + offset * texelSize);
    offset = float2(-1, 1);
    res += LowResTexture.Sample(blitSamplerState, uv + offset * texelSize);
    
    
    return res / 4.F;
}

float LowResSampleComparison(float3 x_position, float3 p_position, float3 x_normal, float3 p_normal)
{
    bool isClose = length(x_position - p_position) < u_DistanceComparisonThreshold;
    bool isFlat = abs(dot(x_normal, p_normal)) < u_NormalComparisonThreshold;
    
    return isClose && isFlat;
}

bool IsSuitableForInterpolation(float3 x_position, float3 x_normal, float2 uv, float2 texelSize)
{
    float suitable = 0.0F;
    float2 offset; 

    offset = float2(1, -1);
    suitable += LowResSampleComparison(x_position, RSM_WorldPositions.Sample(blitSamplerState, float3(uv + offset * texelSize, 0)).xyz,
                                        x_normal, RSM_Normals.Sample(blitSamplerState, float3(uv + offset * texelSize, 0)).xyz);
    
    offset = float2(-1, -1);
    suitable += LowResSampleComparison(x_position, RSM_WorldPositions.Sample(blitSamplerState, float3(uv + offset * texelSize, 0)).xyz,
                                        x_normal, RSM_Normals.Sample(blitSamplerState, float3(uv + offset * texelSize, 0)).xyz);
    
    offset = float2(1, 1);
    suitable += LowResSampleComparison(x_position, RSM_WorldPositions.Sample(blitSamplerState, float3(uv + offset * texelSize, 0)).xyz,
                                        x_normal, RSM_Normals.Sample(blitSamplerState, float3(uv + offset * texelSize, 0)).xyz);
    
    offset = float2(-1, 1);
    suitable += LowResSampleComparison(x_position, RSM_WorldPositions.Sample(blitSamplerState, float3(uv + offset * texelSize, 0)).xyz,
                                        x_normal, RSM_Normals.Sample(blitSamplerState, float3(uv + offset * texelSize, 0)).xyz);
    
    
    return suitable > 2.9;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

VS_OUT RSMComputeGITextureVS(VS_IN vsIn)
{
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    
    VS_OUT vso;
    vso.position = mul(MVP, vsIn.Pos);
    vso.worldPos = mul(ModelMatrix, vsIn.Pos).xyz;
    vso.normal = mul(ModelMatrix, normalize(float4(vsIn.Normal, 0))).xyz;
    vso.normal = normalize(vso.normal);
    
    return vso;
}

float4 RSMComputeGITextureFS(VS_OUT psIn) : SV_Target
{
    // -- Given a pixel, transform it into each light, and compute the irradiance
    float4x4 light_proj = CreateViewProjectionMatrixForLight_Perspective(sourceLight);   
    
    // -- Find the uv of the pixel in the light space
    float4 x_lightposition = mul(float4(psIn.worldPos, 1), light_proj);
    x_lightposition /= x_lightposition.w;
    float2 uv = x_lightposition.xy * .5f + .5f;
    uv = float2(uv.x, 1 - uv.y);

    // -- Now, sample a bunch of pixels around this one and evaluate them
    bool bShouldBeInterpolated = IsSuitableForInterpolation(psIn.worldPos, psIn.normal, uv, 1.F / u_FullTextureDimensions);
    
    float3 indirect_illumination;
    if (bShouldBeInterpolated || !bFullTexture)
    {
        indirect_illumination = InterpolateIndirectIlluminationFromLowResolution(uv);
    }
    else
    {
        //indirect_illumination = float3(0, 0.4, 0);
        indirect_illumination = float3(1, 0, 0);
        indirect_illumination = ComputeIndirectIlluminationForUVs_LowRes(uv, psIn.worldPos, psIn.normal);

    }
    
    return float4(indirect_illumination, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

technique11 RSMCompute
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, RSMComputeGITextureVS()));
        SetPixelShader(CompileShader(ps_5_0, RSMComputeGITextureFS()));
        SetGeometryShader(NULL);
    }
}