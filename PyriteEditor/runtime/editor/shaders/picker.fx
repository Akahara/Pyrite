//======================================================================================================================//

#include "../../res/shaders/incl/cbuffers.incl"
cbuffer ActorPickerIDBuffer
{
    uint actorId;
};

//======================================================================================================================//

struct VertexInput
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

//======================================================================================================================//

float4 DrawMesh(VertexInput vsIn) : SV_Position
{
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    return mul(MVP, vsIn.Pos);
}
float ColorID() : SV_Target
{
    return float(actorId);
}

//======================================================================================================================//

technique11 PickerShader
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, DrawMesh()));
        SetPixelShader(CompileShader(ps_5_0, ColorID()));
        SetGeometryShader(NULL);
    }
}

