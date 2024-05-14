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
#define INV_MAX_KERNEL_SIZE_F 1.f / MAX_KERNEL_SIZE

///////////////////////////////////////////////////////////////////////////////////////


    
float4 u_kernel[MAX_KERNEL_SIZE];
float u_sampleRad;
Texture2D blueNoise;


///////////////////////////////////////////////////////////////////////////////////////

float3 calcViewPosition(float2 uv)
{
    float fragmentDepth = depthBuffer.Sample(blitSamplerState, uv).r;
    float4 ndc = float4(
          uv.x * 2.0 - 1.0,
          uv.y * 2.0 - 1.0,
          fragmentDepth * 2.0 - 1.0,
          1.0
        );
    float4 vs_pos = mul(InverseProj, ndc);
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
    float3 viewPos = calcViewPosition(vs.texCoord);
    float3 viewNormal = cross(ddy(viewPos.xyz), ddx(viewPos.xyz));
    viewNormal = normalize(viewNormal);

    
    float3 randomVec = blueNoise.Sample(MeshTextureSampler, vs.texCoord).xyz;
    
    float3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    float3 bitangent = cross(viewNormal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, viewNormal);
   
    float occlusion_factor = 0.0;
    [loop]
    for (int i = 0; i < MAX_KERNEL_SIZE; i++)
    {
        float3 samplePos = mul(TBN, u_kernel[i].xyz);
        samplePos = viewPos + samplePos * u_sampleRad;
        float4 offset = float4(samplePos, 1.0);
        offset = mul(Proj, offset);
        offset.xy /= offset.w;
        offset.xy = offset.xy * float2(.5, .5) + float2(.5, .5);
        float geometryDepth = calcViewPosition(offset.xy).z;
        float rangeCheck = smoothstep(0.0, 1.0, u_sampleRad / abs(viewPos.z - geometryDepth));
        occlusion_factor += float(geometryDepth >= samplePos.z + 0.0001) * rangeCheck;
    }

    float average_occlusion_factor = occlusion_factor * INV_MAX_KERNEL_SIZE_F;
        
    float visibility_factor = 1.0 - average_occlusion_factor;

    visibility_factor = pow(visibility_factor, 2.0);
    return float4(visibility_factor.xxx, 1);

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