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
#define RSM_SAMPLE_COUNT_SQRD RSM_SAMPLE_COUNT * RSM_SAMPLE_COUNT
#define PI 3.141592653
#define TWO_PI (2.0 * PI)

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
float u_NormalComparisonThreshold = 0.95;
float2 u_FullTextureDimensions = float2(512, 512);
float u_Rmax; // max radius of sampling
float u_normalizationFactor; // max radius of sampling

static const uint LOW_RES_PASS_ID = 0;
static const uint MAX_PASS_COUNT = 4;

uint u_CurrentPassID = 0; // < 0 will 

//////////////////////////////////////////////////////////////////////////////////////////////////

// https://www.shadertoy.com/view/7ssfWN

// reverses the bits of the input
uint MyBitfieldReverse(uint i)
{
    uint b = (uint(i) << 16u) | (uint(i) >> 16u);
    b = (b & 0x55555555u) << 1u | (b & 0xAAAAAAAAu) >> 1u;
    b = (b & 0x33333333u) << 2u | (b & 0xCCCCCCCCu) >> 2u;
    b = (b & 0x0F0F0F0Fu) << 4u | (b & 0xF0F0F0F0u) >> 4u;
    b = (b & 0x00FF00FFu) << 8u | (b & 0xFF00FF00u) >> 8u;
    return b;
}

float2 Hammersley(uint i, uint N)
{
    return float2(
    float(i) / float(N),
    float(MyBitfieldReverse(i)) * 2.3283064365386963e-10
  );
}

float2 WorldPosToUVs(float3 world_position, float4x4 view_proj)
{
    float4 lightspace_position = mul(float4(world_position, 1), view_proj);
    lightspace_position /= lightspace_position.w;
    float2 uv = lightspace_position.xy * .5f + .5f;
    uv = float2(uv.x, 1 - uv.y);
    return uv;
}

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
    float3 p_flux           = RSM_Flux.Sample(blitSamplerState, float3(p_screenSpace, DEBUG_CONST_INDEX_REMOVE_THIS)).rgb;

    // -- Apply equation 1
    float a = max(0, dot(p_normal, (x_worldposition - p_worldPosition)));
    float b = max(0, dot(x_normal, (p_worldPosition - x_worldposition)));
    float3 num = p_flux * a * b;
    
    float dist = max(distance(x_worldposition, p_worldPosition), 0.01);
    float dist2 = dist * dist;
    float inv_denom = 1.F / (dist2);
    
    float3 irradiance = num * inv_denom;
    return irradiance;
}

// -- Given a pixel that the player's camera sees in the lightspace uv, compute the irradiance based on the surrounding pixels
float3 ComputeIndirectIlluminationForUVs(float2 lightspace_uv, float3 x_worldPos, float3 x_normal)
{
    float3 indirect_illumination = float3(0, 0, 0);

    [loop]
    for (int i = 0; i < RSM_SAMPLE_COUNT_SQRD; i++)
    {
        float2 xi = Hammersley(i, RSM_SAMPLE_COUNT_SQRD);
        
        float r = xi.x;
        float theta = xi.y * TWO_PI;
        float2 offseted_uvs = lightspace_uv + float2(r * cos(theta), r * sin(theta)) * u_Rmax;
        float weight = xi.x;
        
        
        float3 radiance = evaluateRadianceInfluence(x_worldPos, x_normal, offseted_uvs);
        radiance *= weight;
        indirect_illumination += radiance;
    }
    indirect_illumination /= RSM_SAMPLE_COUNT_SQRD;
    return indirect_illumination;
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

float4 RSMComputeGITextureFS(VS_OUT psIn, float4 vpos : SV_Position) : SV_Target
{
    // -- Given a pixel, transform it into each light, and compute the irradiance
    float4x4 light_proj = CreateViewProjectionMatrixForLight_Perspective(sourceLight);   
    
    // -- Find the uv of the pixel in the light space
    float2 uv = WorldPosToUVs(psIn.worldPos, light_proj);
    float2 screenspace_uv = WorldPosToUVs(psIn.worldPos, transpose(ViewProj));
    

    // -- Now, sample a bunch of pixels around this one and evaluate them
    // -- First low res pass will compute all the pixels
    if (u_CurrentPassID == LOW_RES_PASS_ID)
    {
        return float4(ComputeIndirectIlluminationForUVs(uv, psIn.worldPos, psIn.normal), 1);
    }
    
    // -- Then subsequent passes will use the low res texture to interpolate if the nearby samples have close normals and positions
    else
    {
        float2 texelSize = 1.F / u_FullTextureDimensions;
        // Find the offset to base the interpolation check on (first pass is using corners, then we can use a cross)
        int2 offsets[4];
        if (u_CurrentPassID == 1)
        {
            offsets[0] = uint2(-1, +1);
            offsets[1] = uint2(+1, +1);
            offsets[2] = uint2(-1, -1);
            offsets[3] = uint2(+1, -1);
        }
        else if (u_CurrentPassID > 1)
        {
            offsets[0] = uint2(-1, 0);
            offsets[1] = uint2(+1, 0);
            offsets[2] = uint2(0, -1);
            offsets[3] = uint2(0, -1);
        }

         // compute weights for each
        float accum_weight = 0;
        float3 accum_color = float3(0,0,0);
        for (int i = 0; i < 4; i++)
        {
            float2 uv_offset = uv + offsets[i] * texelSize;
            
            if (uv_offset.x > 1.f || uv_offset.y > 1 || uv_offset.x < 0 || uv_offset.y < 0)
                continue;
            
            float3 offseted_pixel_normal    = RSM_Normals.Sample(blitSamplerState, float3(uv_offset, 0)).xyz;
            float3 offseted_pixel_worldPos  = RSM_WorldPositions.Sample(blitSamplerState, float3(uv_offset, 0)).xyz;
            
            float3 interpolated_color = LowResTexture.Sample(blitSamplerState, screenspace_uv + offsets[i] * 1/float2(64,64)).rgb;
            
            float3 dn = psIn.normal - offseted_pixel_normal;
            float3 dp = psIn.worldPos - offseted_pixel_worldPos;

            float n_weight = exp(-dot(dn, dn) / 1.0);
            float p_weight = exp(-dot(dp, dp) / 0.4);

            float weight = n_weight * p_weight;
            accum_color += interpolated_color * weight;
            accum_weight += weight;
        }

        if (accum_weight <= 1.0)
        {
            return float4(ComputeIndirectIlluminationForUVs(uv, psIn.worldPos, psIn.normal), 1);
            return float4(1, 0, 0, 1);
        }
        else
        {
            return float4(accum_color / accum_weight, 1);
        }
    }
    
    return float4(0,0.5,0.5, 1);
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