#include "incl/samplers.incl"

cbuffer CameraBuffer
{
    float4x4 matViewProj;
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

VSOut SkyboxVS(uint vertexId : SV_VertexID)
{
    VSOut vsOut = (VSOut) 0;
    vsOut.worldPos = VERTICES[vertexId];
    vsOut.position = mul(matViewProj, vsOut.worldPos + float4(cameraPosition, 0));
    return vsOut;
}


TextureCube environmentMap;
float roughness;
SamplerState cubemapSampler;

#define PI 3.14159265359f
// ----------------------------------------------------------------------------
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = (denom * denom) * PI;

    return nom / denom;
}
// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;
	
    float phi = 2.0 * Xi.x * PI;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
    float3 up = abs(N.z) < 0.9999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
	
    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}
// ----------------------------------------------------------------------------
float4 SkyboxPS(VSOut vs) : SV_Target
{
    //return float4(roughness, 0, roughness, 1);
    float3 N = normalize(vs.worldPos.xyz);
    
    // make the simplifying assumption that V equals R equals the normal 
    float3 R = N;
    float3 V = R;

    const uint SAMPLE_COUNT = 1024;
    float3 prefilteredColor = 0.0.xxx;
    float totalWeight = 0.0001;
    
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = saturate(dot(N, L));
        if (NdotL > 0.0)
        {
            // sample from the environment's mip level based on roughness/pdf
            float D = DistributionGGX(N, H, roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            float resolution = 512.0; // resolution of source cubemap (per face)
            float saTexel = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
            
            prefilteredColor += environmentMap.SampleLevel(blitSamplerState, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight ;

    return float4(prefilteredColor, 1.0);
}

technique11 prefilter_SpecularIBL
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, SkyboxVS()));
        SetPixelShader(CompileShader(ps_5_0, SkyboxPS()));
        SetGeometryShader(NULL);
    }
}