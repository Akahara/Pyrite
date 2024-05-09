cbuffer CameraBuffer
{
    float4x4 MVP;
    float4 cameraPosition;
};

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
    return float4(vsIn.uv, 0, 1);
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