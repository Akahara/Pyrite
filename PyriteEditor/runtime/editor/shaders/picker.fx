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

#define RANDOM_ODD_NUMBER 347631
#define RANDOM_NUMBER 98

float4 hash(int actorId)
{
    int hashed = ((actorId + 173741789) * 507371178) % 1073741789;
    return float4((hashed & 2047) / 2047.F, ((hashed >> 11) & 2047) / 2047.F, (hashed >> 22) / 2047.F, 1.f);

}

//float4 decode(int actorId)
//{
//    (n * 233233408 + 1073741789 - 173741789) % 1073741789
//
//}

//======================================================================================================================//

float4 DrawMesh(VertexInput vsIn) : SV_Position
{
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    return mul(MVP, vsIn.Pos);
}
float4 ColorID() : SV_Target
{
    return hash(actorId);
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