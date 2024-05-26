#include "incl/samplers.incl"
#include "incl/cbuffers.incl"

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

cbuffer ActorMaterials
{
    float4 Ka; // color
    float4 Ks; // specular
    float4 Ke; // emissive
    float Ns; // specular exponent
    float Ni; // optical density 
    float d; // transparency
};

cbuffer CameraBuffer
{
    float4x4 ViewProj;
    float3 cameraPosition;
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
    
    //float N = DistributionGGX();
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
    float NdotH = dot(normal, halfway);
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
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
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


Texture2D mat_albedo : register(t0);
Texture2D mat_normal : register(t1);
Texture2D mat_ao : register(t2);
Texture2D mat_roughness : register(t3);
Texture2D mat_metalness : register(t4);
Texture2D mat_height : register(t5);

float3 sunPos = float3(0, 10, 0);

struct VertexInput
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

struct VertexOut
{
    float4 pos : SV_Position;
    float4 worldpos : POSITION;
    float4 norm : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

VertexOut GGXVertexShader(VertexInput vsIn)
{
    VertexOut vso;
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    vso.pos = mul(MVP, float4(vsIn.Pos, 1));
    vso.worldpos = mul(ModelMatrix, float4(vsIn.Pos, 1));
    vso.uv = vsIn.uv;
    vso.norm = mul(ModelMatrix, normalize(float4(vsIn.Normal, 0)));
    vso.norm = normalize(vso.norm);
    return vso;
}

float3 sampleFromTexture(Texture2D source, float2 uv)
{
    return source.Sample(MeshTextureSampler, uv).xyz;

}

float2 ParallaxMapping(float2 texCoords, float3 viewDir, Texture2D heightmap)
{
    float height = heightmap.Sample(MeshTextureSampler, texCoords).r;
    float2 p = viewDir.xy / viewDir.z * (height * 0.0002);
    return texCoords - p;
}

#define LIGHT_COUNT 1
#define PI 3.14159

float metallic = 0.2;
float roughness = 0.1;
float ao = 0;

float4 GGXPixelShader(VertexOut vsIn) : SV_Target
{
    
    // Get light direction for this fragment
    float3 lightDir = normalize(sunPos - vsIn.worldpos.xyz);

    // Note: Non-uniform scaling not supported
    float diffuseLighting = saturate(dot(vsIn.norm.xyz, -lightDir)); // per pixel diffuse lighting

    // Introduce fall-off of light intensity
    diffuseLighting *= ((length(lightDir) * length(lightDir)) / dot(sunPos - vsIn.worldpos.xyz, sunPos - vsIn.worldpos.xyz));

    // Using Blinn half angle modification for performance over correctness
    float3 h = normalize(normalize(cameraPosition - vsIn.norm.xyz) - lightDir); // why is this norm
    float specLighting = pow(saturate(dot(h, vsIn.norm.xyz)), 2.0f);

    return saturate(float4(.1, 0, 0, 1) + (float4(0,1, 0, 1) * diffuseLighting * 0.6f) + (specLighting * 0.5f));

    //////////////////////////////////
    
    float3 albedo = Ka; // pow(Ka, 2.2);
    float3 pixelNormal = normalize(vsIn.norm.xyz);
    
    float3 V = normalize(cameraPosition - vsIn.worldpos.xyz);
    
    // Go ggx !!
    
    float3 F0 = Ni.xxx;
    F0 = lerp(F0, Ka.xyz, metallic);
    
    float NDF = DistributionGGX(pixelNormal, h, roughness);
    float G = GeometrySmith(pixelNormal, V, lightDir, roughness);
    float3 F = fresnelSchlick(saturate(dot(h, V)), F0);
    
    
    //return float4(F, 1);
    return float4(G.xxx * step(-dot(lightDir, pixelNormal), 0), 1);
    return float4(NDF.xxx, 1);

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