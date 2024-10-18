#include "incl/samplers.incl"
#include "incl/cbuffers.incl"

static float4 VERTICES[36] =
{
    float4(-1, -1, +1, 1), float4(-1, -1, -1, 1), float4(-1, +1, +1, 1),
  float4(-1, -1, -1, 1), float4(+1, +1, -1, 1), float4(-1, +1, -1, 1),
  float4(-1, -1, -1, 1), float4(+1, -1, +1, 1), float4(+1, -1, -1, 1),
  float4(+1, -1, -1, 1), float4(+1, +1, -1, 1), float4(-1, -1, -1, 1),
  float4(-1, +1, +1, 1), float4(-1, -1, -1, 1), float4(-1, +1, -1, 1),
  float4(-1, -1, +1, 1), float4(+1, -1, +1, 1), float4(-1, -1, -1, 1),
  float4(-1, -1, +1, 1), float4(-1, +1, +1, 1), float4(+1, -1, +1, 1),
  float4(+1, -1, -1, 1), float4(+1, +1, +1, 1), float4(+1, +1, -1, 1),
  float4(+1, +1, +1, 1), float4(+1, -1, -1, 1), float4(+1, -1, +1, 1),
  float4(+1, +1, -1, 1), float4(+1, +1, +1, 1), float4(-1, +1, -1, 1),
  float4(-1, +1, -1, 1), float4(+1, +1, +1, 1), float4(-1, +1, +1, 1),
  float4(-1, +1, +1, 1), float4(+1, +1, +1, 1), float4(+1, -1, +1, 1),
};

Texture2D mat_hdr : register(t0);

struct VSOut
{
    float4 position : SV_Position;
    float4 worldPos : TEXCOORD0;
};

VSOut EquiprojVS(uint vertexId : SV_VertexID)
{
    VSOut vsOut = (VSOut) 0;
    vsOut.worldPos = VERTICES[vertexId];
    vsOut.position = mul(ViewProj, vsOut.worldPos);//    +float4(cameraPosition, 0));
    return vsOut;
}

float2 SampleSphericalMap(float3 normal)
{
    normal = normalize(normal);
    float u = 0.5 + atan2(normal.z, normal.x) / (2.0 * 3.14159265359);
    float v = 0.5 - asin(normal.y) / 3.14159265359;
    return float2(u, v);
}

float4 EquiprojFS(VSOut vso) : SV_Target
{
    // Compute normal from position, assuming vpos is in clip space and needs to be transformed to world space
    float3 normal = normalize(vso.worldPos.xyz);

    // Sample spherical map to get UV coordinates
    float2 uv = SampleSphericalMap(normal);

    // Sample the texture using the spherical UV coordinates
    float3 color = mat_hdr.Sample(blitSamplerState, uv).rgb;

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