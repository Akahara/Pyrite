#include "incl/samplers.incl"
#include "incl/cbuffers.incl"

cbuffer CameraBuffer
{
    float4x4 ViewProj;
    float3 cameraPosition;
};


Texture2D mat_hdr : register(t0);

struct VertexInput
{
    float4 Pos : POSITION;
};

float4 EquiprojVS(VertexInput vsIn) : SV_Position
{
    return mul(ViewProj, vsIn.Pos);
}

float2 invAtan = float2(0.1591, 0.3183);
float2 SampleSphericalMap(float3 v)
{
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

float4 EquiprojFS(float4 vpos : SV_Position) : SV_Target
{
    float2 uv = SampleSphericalMap(normalize(vpos.xyz)); // make sure to normalize localPos
    float3 color = mat_hdr.Sample(PointTextureSampler, uv).rgb;
    return float4(color, 1);
}

technique11 GGX
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, EquiprojVS()));
        SetPixelShader(CompileShader(ps_5_0, EquiprojFS()));
        SetGeometryShader(NULL);
    }
}