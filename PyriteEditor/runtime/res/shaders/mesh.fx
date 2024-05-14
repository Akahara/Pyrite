#include "incl/samplers.incl"
#include "incl/cbuffers.incl"

cbuffer CameraBuffer
{
    float4x4 ViewProj;
    float3 cameraPosition;
};

cbuffer ColorBuffer
{
    float4 colorShift;
};

Texture2D mat_albedo;
Texture2D mat_normal;

struct VertexInput
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

struct VertexOut
{
    float4 pos : SV_Position;
    float4 norm : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

float3 sunPos = float3(0, 100, 100);

VertexOut CubeVS(VertexInput vsIn)
{
    VertexOut vso;
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    vso.pos = mul(MVP, vsIn.Pos);
    vso.uv = vsIn.uv;
    vso.norm = float4(vsIn.Normal, 0);
    return vso;
}

float4 CubePS(VertexOut vsIn) : SV_Target
{
    float3 dirToSun = normalize(sunPos - vsIn.pos.xyz);
    float diffuseDot = saturate(dot(dirToSun, vsIn.norm.xyz));
    
    float4 sample = mat_albedo.Sample(MeshTextureSampler, vsIn.uv);
    float3 color = sample.xyz *  lerp(0.5, 1 , diffuseDot);
    return float4(color, sample.a);
}

technique11 MiniPhong
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, CubeVS()));
        SetPixelShader(CompileShader(ps_5_0, CubePS()));
        SetGeometryShader(NULL);
    }
}