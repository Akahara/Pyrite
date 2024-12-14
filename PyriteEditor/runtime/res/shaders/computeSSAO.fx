#include "incl/cbuffers.incl"
#include "incl/samplers.incl"

Texture2D depthBuffer;

//  https://betterprogramming.pub/depth-only-ssao-for-forward-renderers-1a3dcfa1873a


static float4 VERTICES[3] =
{
  float4(-1, -1, 0, 1),
  float4(-1, +3, 0, 1),
  float4(+3, -1, 0, 1),
};

struct VSOut
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////////////

#define MAX_KERNEL_SIZE 64
#define INV_MAX_KERNEL_SIZE_F (1.f / MAX_KERNEL_SIZE)

///////////////////////////////////////////////////////////////////////////////////////

    
float4 u_kernel[MAX_KERNEL_SIZE];
float u_sampleRad = 1.5;
float u_bias = 0.0001f;
float u_tolerancy = -0.85f;
float u_noiseScale = 10.f;
float u_alpha = 0.005f;
Texture2D blueNoise;

static const int SLICE_NORMAL = 1;
Texture2DArray G_Buffer;

///////////////////////////////////////////////////////////////////////////////////////

float3 calcWorldPos(float2 uv)
{
    float fragmentDepth = depthBuffer.Sample(blitSamplerState, uv).r;
    
    float4 ndc = float4(
          uv.x * 2.0 - 1.0,
          (1 - uv.y) * 2.0 - 1.0,
          fragmentDepth,
          1.0
        );
     
    float4 vs_pos = mul(InverseViewProj, ndc);
    vs_pos.xyz = vs_pos.xyz / vs_pos.w;
          
    return vs_pos.xyz;
}

///////////////////////////////////////////////////////////////////////////////////////

VSOut vs(uint vertexId : SV_VertexID)
{
    float4 vertex = VERTICES[vertexId];
    VSOut vsOut = (VSOut) 0;
    vsOut.position = vertex;
    vsOut.texCoord = vertex.xy * float2(.5, -.5) + .5;
    return vsOut;
}

float4 ps(VSOut vs) : SV_Target
{
    float3 worldPos = calcWorldPos(vs.texCoord);
    
    float3 normal = cross(ddy_fine(worldPos.xyz), ddx_fine(worldPos.xyz));
    normal = G_Buffer.Sample(blitSamplerState, float3(vs.texCoord, SLICE_NORMAL)).xyz;
    normal = normalize(normal);
    
    float localDepth = depthBuffer.Sample(blitSamplerState, vs.texCoord).r;

    float4 whiteNoiseSample = blueNoise.Sample(MeshTextureSampler, vs.texCoord * u_noiseScale);
    float3 randomVec = whiteNoiseSample.xyz;
    
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    
    float occlusion_factor = 0.f;
    
    [loop]
    for (int i = 0; i < MAX_KERNEL_SIZE; i++)
    {
        
        // This is an attempt at fixing the bading issues ...
        float angle = dot(normalize(u_kernel[i].xyz), normal);
        normal = normal * (step(angle, 0) * 2 - 1);
        if (dot(normalize(u_kernel[i].xyz), normal) < u_tolerancy)
            continue;
        
        float3 samplePos = mul(TBN, u_kernel[i].xyz);
        samplePos = worldPos + samplePos * u_sampleRad * (1 + whiteNoiseSample.a / u_alpha);
        
        float4 offset = float4(samplePos, 1.0);
        offset = mul(ViewProj, offset);
        
        offset.xy /= offset.w;
        offset.xy = offset.xy * .5 + .5;
        offset.y = 1 - offset.y;
        
        float geometryDepth = depthBuffer.Sample(blitSamplerState, offset.xy).r;
        float deltaDepth = abs(localDepth - geometryDepth);
        float rangeCheck = smoothstep(0.0, 1.0, u_sampleRad / deltaDepth);
        occlusion_factor += step(geometryDepth, localDepth + .001 * deltaDepth) * rangeCheck;
    }

    float average_occlusion_factor = occlusion_factor * INV_MAX_KERNEL_SIZE_F;
        
    float visibility_factor = 1.0 - average_occlusion_factor;

    return float4(visibility_factor.xxx, 1);
    visibility_factor = pow(visibility_factor, 2.0);

}

technique11 MiniPhong
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, vs()));
        SetPixelShader(CompileShader(ps_5_0, ps()));
        SetGeometryShader(NULL);
    }
}