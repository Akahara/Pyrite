#include "incl/cbuffers.incl"

//////////////////////////////////////////////////////////////////////////////////////////////////

struct VertexInput
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

struct VertexOut
{
    float4 position : SV_Position;
    float4 worldPos : Position;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

VertexOut DepthVS(VertexInput vsIn) 
{
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    
    VertexOut vso;
    vso.position = mul(MVP, vsIn.Pos);
    vso.worldPos = mul(ModelMatrix, vsIn.Pos);
    return vso;
}

#ifdef LINEARIZE_DEPTH
float u_CameraFarPlane; // < would be great to have this stored in the cameraBuffer....
float3 u_sourcePosition; 

float LinearDepthFS(VertexOut vsIn) : SV_Target
{
    // get distance between fragment and light source
    float lightDistance = length(vsIn.worldPos.xyz - u_sourcePosition);
    
    return lightDistance;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

technique11 DepthOnly
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, DepthVS()));
#ifdef LINEARIZE_DEPTH
        SetPixelShader(CompileShader(ps_5_0, LinearDepthFS()));
#else
        SetPixelShader(NULL);
#endif
        SetGeometryShader(NULL);
    }
}