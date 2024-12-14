//======================================================================================================================//
// -- INCLUDES

#include "incl/samplers.incl"
#include "incl/cbuffers.incl"
#include "incl/shadow_utils.incl"
#include "incl/light_utils.incl"

//======================================================================================================================//

struct VS_IN
{
    float4 Pos      : POSITION;
    float3 Normal   : NORMAL;
    float2 uv       : UV;
};

struct VS_OUT
{
    float4 pos      : SV_Position;
    float4 worldpos : POSITION;
    float3 norm     : TEXCOORD0;
    float2 uv       : TEXCOORD1;
};

struct PS_OUT
{
    float4 albedo           : SV_Target0 ;
    float4 Normal           : SV_Target1 ;
    float4 WorldPosition    : SV_Target2 ;
    float4 ARM              : SV_Target3 ;
    float4 Unlit            : SV_Target4 ;
};

//======================================================================================================================//

Texture2D mat_albedo;
Texture2D mat_normal;
Texture2D mat_ao;
Texture2D mat_roughness;
Texture2D mat_metalness;
Texture2D mat_height;
Texture2D ssaoTexture;

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


//======================================================================================================================//

VS_OUT GBufferVS(VS_IN vsIn)
{
    VS_OUT vso;
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    vso.pos = mul(MVP, vsIn.Pos);
    
    vso.uv = vsIn.uv;
    vso.norm = mul(ModelMatrix, normalize(float4(vsIn.Normal, 0))).xyz;
    vso.norm = normalize(vso.norm);
    
    vso.worldpos = mul(ModelMatrix, vsIn.Pos);
    
    return vso;
}

//======================================================================================================================//

PS_OUT GBufferFS(VS_OUT vsIn)
{
    PS_OUT ps_out;
    
    float4 sampleNormal = mat_normal.Sample(MeshTextureSampler, vsIn.uv);
    float3 pixelNormal = vsIn.norm.xyz;
    pixelNormal = normalize(pixelNormal);
    
    ps_out.Normal        = float4(normalize(vsIn.norm.xyz * sampleNormal.xyz), 1);
    ps_out.WorldPosition = float4(vsIn.worldpos.xyz, 1);
    
    ps_out.albedo        = Ka * mat_albedo.Sample(MeshTextureSampler, vsIn.uv);
    
    ps_out.ARM.x         = mat_ao.Sample(MeshTextureSampler, vsIn.uv);
    ps_out.ARM.y         = roughness * mat_roughness.Sample(MeshTextureSampler, vsIn.uv);
    ps_out.ARM.z         = metallic * mat_metalness.Sample(MeshTextureSampler, vsIn.uv);
    ps_out.ARM.w         = Ni; 
    
    return ps_out;
}

//======================================================================================================================//

technique11 ComputeGBuffer
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, GBufferVS()));
        SetPixelShader(CompileShader(ps_5_0, GBufferFS()));
        SetGeometryShader(NULL);
    }
}