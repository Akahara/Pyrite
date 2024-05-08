// todo add verrtex type in .incl file


struct Mesh_Vertex_type
{
    float3 Pos : POSITION;
    float2 uv : TEXCOORD;
};

float4 CubeVS(Mesh_Vertex_type vsIn) : SV_Position
{
    return float4(vsIn.Pos, 1);
}

float4 CubePS() : SV_Target
{
    return float4(1, 1, 0, 1);
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