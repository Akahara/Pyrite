//======================================================================================================================//
// -- INCLUDES

#include "incl/samplers.incl"
#include "incl/cbuffers.incl"
#include "incl/shadow_utils.incl"
#include "incl/light_utils.incl"

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
//      BRDF_ct = (D * F * G) / (4*(w_o.dot(n))*(w_i.dot(n)))   
//      
//      where D, F and G are functions
//
//
//  D is Normal Distribution , which basically relates to how smooth a surface is. D has multiple implementation possible.
//  F is Fresnel, probably using schlick approximation
//  G is the geometry , which is the self shadowing term. Basically ao
//
// Finally, we do an approximation of the rendering equation using a riemann sum, and i think thats it for GGX pbs

//======================================================================================================================//
// -- DEFINES

#define LIGHT_COUNT 16
#define PI 3.14159

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

// -- Normal distribution function 
float DistributionGGX(float3 normal, float3 halfway, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = dot(normal, halfway);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;

    return a2 / denom;
}

// -- Geometry function

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
  
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 fresnelSchlick(float3 H, float3 V, float3 F0)
{
    float cosTheta = dot(H, V);
    return fresnelSchlick(cosTheta, F0);
}

float3 sampleFromTexture(Texture2D source, float2 uv)
{
    return source.Sample(MeshTextureSampler, uv).xyz;
}

//======================================================================================================================//


Texture2D mat_albedo;
Texture2D mat_normal ;
Texture2D mat_ao ;
Texture2D mat_roughness ;
Texture2D mat_metalness;
Texture2D mat_height ;
Texture2D ssaoTexture;
TextureCube irrandiance_map;
Texture2D brdfLUT;
Texture2D dummyTexture;
TextureCube prefilterMap;

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
    
    float computed_metallic = metallic; 
    computed_metallic *= sampleFromTexture(mat_metalness, vsIn.uv).b;
    float computed_roughness = roughness;
    computed_roughness *= sampleFromTexture(mat_roughness, vsIn.uv).g;
    
    float3 R = reflect(-V, pixelNormal);
    
    // Go ggx !!
    float3 F0 = Ni.xxx; 
    F0 = lerp(F0, albedo.xyz, computed_metallic);
    float3 Lo = float3(0,0,0);
    float shadow_attenuation = 1.0f;
    // -- For each light, compute the specular --//
    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        Light light = lights[i];
        if (light.isOn < 1.0f)
            continue;
        
        float3 radiance = 0.0.xxx;
        float3 L = normalize(1.0.xxx);
        bool bShouldCastShadows = light.shadowType == 1;
        
        if (light.type == DIRECTIONAL_LIGHT_TYPE) // dir
        {
            if (bShouldCastShadows)
            {
                shadow_attenuation = 1.f - getShadowFactor_2D_Ortho(vsIn.worldpos.xyz, vsIn.norm.xyz, light.lightmap_index, CreateViewProjectionMatrixForLight_Ortho(light));
            }
            
            L = normalize(-light.direction);
            radiance = light.diffuse.xyz * light.strength * 5.f;
            
        }
        if (light.type == POINT_LIGHT_TYPE) // point
        {

            if (bShouldCastShadows)
            {
                shadow_attenuation = 1.f - getShadowFactor_3D(vsIn.worldpos.xyz, vsIn.norm.xyz, light.lightmap_index - 16, light.position.xyz);
            }
            
            float LightToPixel = light.position.xyz - vsIn.worldpos.xyz;
            float dist = length(LightToPixel);
            float attenuation = 1.0 / (light.range.y + light.range.z * dist + light.range.w * (dist * dist));
            L = normalize(light.position.xyz - vsIn.worldpos.xyz);
            radiance = light.diffuse.xyz * attenuation * light.specularFactor;

        }
        if (light.type == SPOT_LIGHT_TYPE) // spot
        {
            if (bShouldCastShadows)
            {
                shadow_attenuation = 1.f - getShadowFactor_2D_Perspective(
                    vsIn.worldpos.xyz, vsIn.norm.xyz,
                    light.lightmap_index, CreateViewProjectionMatrixForLight_Perspective(light), light.position.xyz);
            }
            
            L = normalize(light.position.xyz - vsIn.worldpos.xyz);
            float smallAngle = light.range.x;
            float largeAngle = smallAngle + light.fallOff;
            
            float pixelToSpotAngle = saturate(dot(
                    normalize(-light.position.xyz + vsIn.worldpos.xyz),
                    normalize(light.direction.xyz)
            ));
            float t = smoothstep(0, 1, (pixelToSpotAngle - cos(largeAngle)) / (cos(smallAngle) - cos(largeAngle)));
            radiance = light.diffuse.rgb * lights[i].strength * t;
        }
        
        radiance *= shadow_attenuation;
        float3 H = normalize(L + V);
        float NDF = DistributionGGX(pixelNormal, H, computed_roughness);
        float G = GeometrySmith(pixelNormal, V, L, computed_roughness);
        float3 F = fresnelSchlick(H, V, F0);
        float3 kS = F;
        float3 kD = float3(1,1,1) - kS;
        kD *= 1.f - computed_metallic;
        float NDotL = saturate(dot(pixelNormal, L));
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * saturate(dot(V, pixelNormal)) * NDotL;
        float3 specular = numerator / (denominator + 0.0001);
        Lo += ((kD * albedo / PI) + specular) * radiance * NDotL;
    }
    
    // -- Ambiant coloring -- //
    float3 F = fresnelSchlick(saturate(dot(pixelNormal, V)), F0);
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    float3 irradiance = irrandiance_map.Sample(blitSamplerState, pixelNormal).rgb;
    float3 diffuse = irradiance * albedo;
    
    const float MAX_REFLECTION_LOD = 4.0;
    float3 prefilteredColor = prefilterMap.SampleLevel(blitSamplerState, R, roughness * MAX_REFLECTION_LOD).rgb;
    float2 envBRDF = brdfLUT.Sample(blitSamplerState, float2(saturate(dot(pixelNormal, V)), roughness)).rg;
    float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y) + 0.001;
    
    float matOcclusion = sampleFromTexture(mat_ao, vsIn.uv).r;
    
    float occlusion = matOcclusion;
    
    int w, h;
    ssaoTexture.GetDimensions(w, h);
    occlusion *= ssaoTexture.Load(float3(vpos.x % w, vpos.y % h, vpos.z));
    float3 ambient = (kD * diffuse + specular) * occlusion;
    
    // -- Tonemapping -- //
    float3 OutColor = (ambient + Lo * occlusion);
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