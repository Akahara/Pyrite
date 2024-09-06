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

float4 hash(int id)
{
    int hashed = ((id + 173741789) * 507371178) % 1073741789;
    return float4((hashed & 2047) / 2047.F, ((hashed >> 11) & 2047) / 2047.F, (hashed >> 22) / 1023.0f, 1.f);

}
//float4 decode(int actorId)
//{
//    (n * 233233408 + 1073741789 - 173741789) % 1073741789
//
//}

//======================================================================================================================//

float4 EncodeActorID(uint id)
{
    float r = (float) (id & 0x7FF) / 2047.0f; // Red channel (11 bits)
    float g = (float) ((id >> 11) & 0x7FF) / 2047.0f; // Green channel (11 bits)
    float b = (float) ((id >> 22) & 0x3FF) / 1023.0f; // Blue channel (10 bits)
    return float4(r, g, b, 1.0f); // Alpha is not used
}

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