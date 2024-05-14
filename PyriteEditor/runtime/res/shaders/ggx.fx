#include "incl/samplers.incl"
#include "incl/cbuffers.incl"

cbuffer CameraBuffer
{
    float4x4 ViewProj;
    float3 cameraPosition;
};


Texture2D tex : register(t0);
Texture2D texNormal : register(t1);

// struct to hold materials and sutff should come here

// Explanation and goal of this file : build the Render equation using GGX BRDF.
// GGX Brdf is a brdf defined as
//
//      BRDF(x,w_i,w_o) = Kd * BRDF_lambert + Ks * BRDF_cook_torrance
//  
//      where Kd is the amount of light absorbed and ks reflected (kd + ks = 1 for energy conservation)
//
//
// Lambertian bdrf is straightforward c/pi 
// But what is BRDF_cook_torrance ?
//
//      BRDF_ct = (D * F * G) / 4*(w_o.dot(n))*(w_i.dot(n))   
//      
//      where D, F and G are functions
//
//
//  D is Normal Distribution , which basically relates to how smooth a surface is. D has multiple implementation possible.
//  F is Fresnel, probably using schlick approximation
//  G is the geometry , which is the self shadowing term. Basically ao
//
// Finally, we do an approximation of the rendering equation using a riemann sum, and i think thats it for GGX pbs

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// -- Definition of a material

struct Material
{
    float Kd;
    float Ks;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// -- Compute light using riemann sum approximation

float4 ComputeLightForPixel(float3 position, float2 uv, float3 cameraPos)
{
    int steps = 100;
    float sum = 0.0f;
    //float3 Wo = ;
    //float3 normal = texNormal.Sample(PointClampSampler, uv);
    float dW = 1.0f / steps;
    for (int i = 0; i < steps; ++i)
    {
        //float3 incoming = getNextIncomingLightDir(i);
        //sum += Fr(P, Wi, Wo) * L(P, Wi) * dot(N, Wi) * dW;
    }
    return float4(0, 0, 0, 0);
}

// -- Compute BRDF

float4 BRDF(float3 x, float3 incoming, float3 outgoing)
{
    return float4(0, 0, 0, 0);
}

// -- Compute Cook-Torrance BRDF

float4 BRDF_CookTorrance(float3 x, float3 normal, float3 incoming, float3 outgoing)
{
    return float4(0, 0, 0, 0);
    //return (DistributionGGX(normal, ) * GeometrySchlickGGX() * fresnelSchlick()) / (4 * dot(incoming, normal) * dot(outgoing, normal));

}

// -- Compute Lambertian BRDF
float4 BRDF_Lambertian(float3 x, float3 incoming, float3 outgoing)
{
    return float4(0, 0, 0, 0);
}

// -- Normal distribution function 
float DistributionGGX(float3 normal, float3 halfway, float roughness)
{
    float r2 = roughness * roughness;
    float NdotH = max(dot(normal, halfway), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (r2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;

    return r2 / denom;
}

// -- Geometry function

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float nom = NdotV;
    float denom = NdotV * (1.0 - roughness) + roughness;
	
    return nom / denom;
}
  
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
// -- Fresnel

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


Texture2D mat_albedo;
Texture2D mat_normal;
float3 sunPos = float3(0, 100, 100);

struct VertexInput
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VertexOut
{
    float4 pos : SV_Position;
    float4 norm : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

VertexOut GGXVertexShader(VertexInput vsIn)
{
    VertexOut vso;
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    vso.pos = mul(MVP, float4(vsIn.Pos, 1));
    vso.uv = vsIn.uv;
    vso.norm = float4(vsIn.Normal, 0);
    return vso;
}

float4 GGXPixelShader(VertexOut vsIn) : SV_Target
{
    float3 dirToSun = normalize(sunPos - vsIn.pos.xyz);
    float diffuseDot = saturate(dot(dirToSun, vsIn.norm.xyz));
    
    float4 sample = mat_albedo.Sample(MeshTextureSampler, vsIn.uv);
    float3 color = sample.xyz * lerp(0.5, 1, diffuseDot);
    return float4(color, sample.a);
}

technique11 GGX
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, GGXVertexShader()));
        SetPixelShader(CompileShader(ps_5_0, GGXPixelShader()));
        SetGeometryShader(NULL);
    }
}