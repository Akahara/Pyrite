cbuffer CameraBuffer
{
    float4x4 MVP;
    float4 cameraPosition;
};


float4 DefaultVS(float3 Pos : POSITION) : SV_Position
{
    return mul(MVP, float4(Pos, 1));
}

float4 DefaultPS() : SV_Target
{
    return float4(1,0,0,1);
}

technique11 MiniPhong
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, DefaultVS()));
        SetPixelShader(CompileShader(ps_5_0, DefaultPS()));
        SetGeometryShader(NULL);
    }
}