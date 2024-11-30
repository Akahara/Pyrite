//======================================================================================================================//
// -- INCLUDES

#include "incl/samplers.incl"
#include "incl/cbuffers.incl"
#include "incl/shadow_utils.incl"
#include "incl/light_utils.incl"
#include "incl/ibl.incl"
#include "incl/ggx.incl"

//======================================================================================================================//

static float4 VERTICES[3] =
{
    float4(-1, -1, 0, 1),
    float4(-1, +3, 0, 1),
    float4(+3, -1, 0, 1),
};

struct VS_OUT
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

//======================================================================================================================//

static const int SLICE_ALBEDO   = 0;
static const int SLICE_NORMAL   = 1;
static const int SLICE_WORLDPOS = 2;
static const int SLICE_ARM      = 3;

Texture2DArray G_Buffer;
Texture2D      DepthBuffer;

cbuffer lightsBuffer
{
    Light lights[16];
};

//======================================================================================================================//
// -- Optional stuff

Texture2D ssaoTexture;
Texture2D GI_CompositeTexture;

//======================================================================================================================//

VS_OUT BlitVS(uint vertexId : SV_VertexID)
{
    float4 vertex = VERTICES[vertexId];
    VS_OUT vsOut = (VS_OUT) 0;
    vsOut.position = vertex;
    vsOut.texCoord = vertex.xy * float2(.5, -.5) + .5;
    return vsOut;
}

//======================================================================================================================//

float4 DeferredLightPassFS(VS_OUT vsIn) : SV_Target
{
    // -- First, discard pixel if the depth-pre pass did not render anything
    
    float depth = DepthBuffer.Sample(blitSamplerState, vsIn.texCoord).r;
    if (depth == 1.00f) discard;
    
    // -- Fetch the pixel data
    
    float3 albedo   = G_Buffer.Sample(blitSamplerState, float3(vsIn.texCoord, SLICE_ALBEDO))  .xyz;
    float3 normal   = G_Buffer.Sample(blitSamplerState, float3(vsIn.texCoord, SLICE_NORMAL))  .xyz;
    float3 worldPos = G_Buffer.Sample(blitSamplerState, float3(vsIn.texCoord, SLICE_WORLDPOS)).xyz;
    float4 armn     = G_Buffer.Sample(blitSamplerState, float3(vsIn.texCoord, SLICE_ARM))         ;
    
    float ao        = armn.r;
    float roughness = armn.g;
    float metallic  = armn.b;
    float Ni        = armn.a;
    
    // -- Compute lighting (GGX)
    
    float3 V = normalize(cameraPosition - worldPos);
    float3 F0 = Ni.xxx;
    F0 = lerp(F0, albedo.xyz, metallic);
    float3 Lo = float3(0, 0, 0);
    
    // -- For each light, compute the specular 
    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        Light light = lights[i];
        if (light.isOn < 1.0f)
            continue;
        
        // -- Compute how much the light participates in the radiance, taking shadows into account
        float shadow_attenuation = 1.0f - ShadowFactor(light, worldPos, normal);
        float3 radiance = Flux(light, worldPos);
        radiance *= shadow_attenuation;
        
        // -- BRDF
        float3 L = GetLightVector(light, worldPos);
        Lo += BRDF_CookTorrance(radiance, albedo, L, V, normal, roughness, metallic, F0);
    }
    
     // -- IBL Environnement sampling for reflections
    float3 F = fresnelSchlick(saturate(dot(normal, V)), F0);
    float3 environnementReflection = GetEnvironnementReflection(normal, V, F, roughness);
    float3 environnementIrradiance = GetAmbiantColoring(normal);
    
    // -- SSAO
    ao *= ssaoTexture.Sample(blitSamplerState, vsIn.texCoord).r;
    float3 GI = GI_CompositeTexture.Sample(blitSamplerState, vsIn.texCoord);
    
    // -- Compute the color (ambiant + specular essentially)
    float3 diffuse = (environnementIrradiance + GI) * (albedo);
    float3 ambient = (GetIBLDiffuse(F, metallic) * diffuse) + environnementReflection;
    float3 OutColor = (ambient + Lo) * ao;

    // -- Tonemapping 
    OutColor = OutColor / (OutColor + float3(1, 1, 1));
    OutColor = pow(OutColor, 0.4545.xxx);
    
    return float4(OutColor, 1);
}

//======================================================================================================================//

technique11 DeferredLightPass
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, BlitVS()));
        SetPixelShader(CompileShader(ps_5_0, DeferredLightPassFS()));
        SetGeometryShader(NULL);
    }
}