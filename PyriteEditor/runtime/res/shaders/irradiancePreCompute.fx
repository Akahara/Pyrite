#include "incl/samplers.incl"


TextureCube cubemapHDR;
SamplerState cubemapSampler;

cbuffer CameraBuffer
{
    float4x4 ViewProj;
    float3 cameraPosition;
};

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

struct VSOut
{
    float4 position : SV_Position;
    float4 worldPos : TEXCOORD0;
};

VSOut IrradianceVS(uint vertexId : SV_VertexID)
{
    VSOut vsOut = (VSOut) 0;
    vsOut.worldPos = VERTICES[vertexId];
    vsOut.position = mul(ViewProj, vsOut.worldPos); 
    return vsOut;
}
#define PI 3.141592
#define LOD 1

float4 IrradiancePS(VSOut vs) : SV_Target
{
    float3 normal = vs.worldPos.xyz;
    float3 irradiance = 0.0f.xxx;

    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    float sampleDelta = 0.025 * LOD;
    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
        // spherical to cartesian (in tangent space)
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
        // tangent space to world
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
            irradiance += cubemapHDR.Sample(blitSamplerState, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    return float4(irradiance, 1);
}

technique11 MiniPhong
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0,   IrradianceVS()));
        SetPixelShader(CompileShader(ps_5_0, IrradiancePS()));
        SetGeometryShader(NULL);
    }
}