//======================================================================================================================//
// -- INCLUDES

#include "incl/samplers.incl"
#include "incl/cbuffers.incl"
#include "incl/shadow_utils.incl"
#include "incl/light_utils.incl"
#include "incl/ggx.incl"
#include "incl/ibl.incl"

//======================================================================================================================//

// -- Definition of a material

cbuffer ActorMaterials
{
    float4 Ka;          // color
    float4 Ks;          // specular
    float4 Ke;          // emissive
    float roughness;    // specular exponent
    float metallic;     // specular exponent
    float Ni;           // optical density 
    float d;            // transparency < todo 
};

cbuffer lightsBuffer
{
    Light lights[16];
};

//======================================================================================================================//

float3 sampleFromTexture(Texture2D source, float2 uv)
{
    return source.Sample(MeshTextureSampler, uv).xyz;
}

//======================================================================================================================//

Texture2D GI_CompositeTexture;
Texture2D mat_albedo;
Texture2D mat_normal ;
Texture2D mat_ao ;
Texture2D mat_roughness ;
Texture2D mat_metalness;
Texture2D mat_height ;
Texture2D ssaoTexture;

struct VertexInput
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

struct VertexOut
{
    float4 pos : SV_Position;
    float4 worldpos : POSITION;
    float3 norm : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

//======================================================================================================================//

VertexOut GGXVertexShader(VertexInput vsIn)
{
    VertexOut vso;
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    vso.pos = mul(MVP, vsIn.Pos);
    
    vso.uv = vsIn.uv;
    vso.norm = mul(ModelMatrix, normalize(float4(vsIn.Normal, 0))).xyz;
    vso.norm = normalize(vso.norm);
    
    vso.worldpos = mul(ModelMatrix, vsIn.Pos);
    
    return vso;
}

//======================================================================================================================//

float4 GGXPixelShader(VertexOut vsIn, float4 vpos : SV_Position) : SV_Target
{
    // -- Sampling stuff
    float3 V = normalize(cameraPosition - vsIn.worldpos.xyz);
    vsIn.uv.y = 1 - vsIn.uv.y;
    float3 albedo = Ka.rgb;
    float4 sampleAlbedo = mat_albedo.Sample(MeshTextureSampler, vsIn.uv);
    albedo *= sampleAlbedo.xyz;
    albedo = pow(albedo, 2.2);
    
    float4 sampleNormal = mat_normal.Sample(MeshTextureSampler, vsIn.uv);
    sampleNormal.y *= -1;
    float3 pixelNormal = vsIn.norm.xyz;
    pixelNormal = normalize(pixelNormal);
    
    float computed_roughness = roughness;
    computed_roughness *= sampleFromTexture(mat_roughness, vsIn.uv).g;
    
    float computed_metallic = metallic; 
    computed_metallic *= sampleFromTexture(mat_metalness, vsIn.uv).b;
    
    // -- Go ggx !!
    float3 F0 = Ni.xxx; 
    F0 = lerp(F0, albedo.xyz, computed_metallic);
    float3 Lo = float3(0,0,0);
    
    // -- For each light, compute the specular 
    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        Light light = lights[i];
        if (light.isOn < 1.0f) continue;
        
        // -- Compute how much the light participates in the radiance, taking shadows into account
        float shadow_attenuation = 1.0f - ShadowFactor(light, vsIn.worldpos.xyz, vsIn.norm.xyz);
        float3 radiance = Flux(light, vsIn.worldpos.xyz);
        radiance *= shadow_attenuation;
        
        // -- BRDF
        float3 L = GetLightVector(light, vsIn.worldpos.xyz);
        Lo += BRDF_CookTorrance(radiance, albedo, L, V, vsIn.norm.xyz, computed_roughness, computed_metallic, F0);
    }
    
    // -- Wip GI composite
    int w, h;
    ssaoTexture.GetDimensions(w, h);
    int gi_w, gi_h;
    GI_CompositeTexture.GetDimensions(gi_w, gi_h);
    float4 pos = mul(ViewProj, float4(vsIn.worldpos.xyz, 1));
    pos /= pos.w;
    float2 x_ss_uv = pos.xy * .5f + .5f;
    x_ss_uv = float2(x_ss_uv.x, 1 - x_ss_uv.y);
    float3 GI = GI_CompositeTexture.Sample(MeshTextureSampler, x_ss_uv);
    
    // -- IBL Environnement sampling for reflections
    float3 F = fresnelSchlick(saturate(dot(pixelNormal, V)), F0);
    float3 environnementReflection = GetEnvironnementReflection(pixelNormal, V, F, computed_roughness);
    float3 environnementIrradiance = GetAmbiantColoring(pixelNormal);
    
    // -- SSAO
    float matOcclusion = sampleFromTexture(mat_ao, vsIn.uv).r;
    float occlusion = matOcclusion;
    occlusion *= ssaoTexture.Load(float3(vpos.x % w, vpos.y % h, vpos.z));
    
    // -- Compute the color (ambiant + specular essentially)
    float3 diffuse = (environnementIrradiance + GI) * (albedo);
    float3 ambient = (GetIBLDiffuse(F, computed_metallic) * diffuse) + environnementReflection;
    float3 OutColor = (ambient + Lo) * occlusion;

    // -- Tonemapping 
    OutColor = OutColor / (OutColor + float3(1, 1, 1));
    OutColor = pow(OutColor, 0.4545.xxx);
    
    return float4(OutColor, 1);
}

//======================================================================================================================//

technique11 GGX
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, GGXVertexShader()));
        SetPixelShader(CompileShader(ps_5_0, GGXPixelShader()));
        SetGeometryShader(NULL);
    }
}