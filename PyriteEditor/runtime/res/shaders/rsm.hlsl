#include "incl/cbuffers.incl"
#include "incl/samplers.incl"
#include "incl/light_utils.incl"

//////////////////////////////////////////////////////////////////////////////////////////////////

struct VS_IN
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

struct VS_OUT
{
    float4 position : SV_Position;
    float3 worldPos : Position;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOOD0;
};

struct PS_OUT
{
    float4 WorldPosition    : SV_Target0 ; 
    float4 Normals          : SV_Target1 ; 
    float4 Flux             : SV_Target2 ;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer SingleLightBuffer
{
    Light sourceLight;
};

Texture2D albedoTexture;
float4 Ka;

//////////////////////////////////////////////////////////////////////////////////////////////////

VS_OUT RSMGenerationVS(VS_IN vsIn)
{
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    
    VS_OUT vso;
    vso.position = mul(MVP, vsIn.Pos);
    vso.worldPos = mul(ModelMatrix, vsIn.Pos);
    vso.uv = vsIn.uv;
    vso.normal = mul(ModelMatrix, normalize(float4(vsIn.Normal, 0))).xyz;
    vso.normal = normalize(vso.normal);
    return vso;
}

PS_OUT RSMGenerationFS(VS_OUT psIn)
{
    PS_OUT ps_out;
    ps_out.Normals = float4(psIn.normal, 1);
    ps_out.WorldPosition = float4(psIn.worldPos, 1);
    
    psIn.uv.y = 1 - psIn.uv.y;
    float3 albedo = Ka.rgb;
    float4 sampleAlbedo = albedoTexture.Sample(MeshTextureSampler, psIn.uv);
    albedo *= sampleAlbedo.xyz;
    
    float smallAngle = sourceLight.range.x;
    float largeAngle = smallAngle + sourceLight.fallOff;
    float pixelToSpotAngle = saturate(dot(
                    normalize(-sourceLight.position.xyz + psIn.worldPos.xyz),
                    normalize(sourceLight.direction.xyz)
            ));
    float t = smoothstep(0, 1, (pixelToSpotAngle - cos(largeAngle)) / (cos(smallAngle) - cos(largeAngle)));
    t *= (sourceLight.strength / 4.F);
    
    ps_out.Flux = float4(albedo * t, 1);
    
    return ps_out;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

technique11 RSMCompute
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, RSMGenerationVS()));
        SetPixelShader(CompileShader(ps_5_0, RSMGenerationFS()));
        SetGeometryShader(NULL);
    }
}