#include "incl/cbuffers.incl"

//////////////////////////////////////////////////////////////////////////////////////////////////

struct VertexInput
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

float4 TransformVS(VertexInput vsIn) : SV_Position
{
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    return mul(MVP, vsIn.Pos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

technique11 DepthOnly
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, TransformVS()));
        SetPixelShader(NULL);
        SetGeometryShader(NULL);
    }
}