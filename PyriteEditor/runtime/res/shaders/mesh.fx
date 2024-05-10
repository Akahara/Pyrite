#include "incl/samplers.incl"

cbuffer CameraBuffer
{
    float4x4 MVP;
    float3 cameraPosition;
};

float u_blue = 0;
Texture2D tex_breadbug;

struct VertexInput
{
    float3 Pos : POSITION;
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
    vso.pos = mul(MVP, float4(vsIn.Pos, 1));
    vso.uv = vsIn.uv;
    return vso;
}

float4 CubePS(VertexOut vsIn) : SV_Target
{
    return tex_breadbug.Sample(MeshTextureSampler, vsIn.uv);

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