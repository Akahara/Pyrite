#include "incl/samplers.incl"
#include "incl/cbuffers.incl"

cbuffer CameraBuffer
{
    float4x4 MVP;
    float3 cameraPosition;
};

cbuffer ColorBuffer
{
    float4 colorShift;
};

cbuffer UnusedBuffer
{
    float4 foo;
    float4x4 bar;
};

float u_blue = 0;
Texture2D tex_breadbug;

struct VertexInput
{
    float4 Pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct VertexOut
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

VertexOut CubeVS(VertexInput vsIn)
{
    VertexOut vso;
    vso.pos = mul(MVP, vsIn.Pos);
    vso.uv = vsIn.uv;
    return vso;
}

float4 CubePS(VertexOut vsIn) : SV_Target
{
    return tex_breadbug.Sample(MeshTextureSampler, vsIn.uv) * colorShift + importedValue;

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