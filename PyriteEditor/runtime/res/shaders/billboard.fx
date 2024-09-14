//======================================================================================================================//

#include "../../res/shaders/incl/cbuffers.incl"
#include "../../res/shaders/incl/samplers.incl"

//======================================================================================================================//

Texture2D textures[16];

struct VertexInput
{
    float4 instanceTransform0 : INSTANCE_TRANSFORM0;
    float4 instanceTransform1 : INSTANCE_TRANSFORM1;
    float4 instanceTransform2 : INSTANCE_TRANSFORM2;
    float4 instanceTransform3 : INSTANCE_TRANSFORM3;
    
    float4 instanceUVs   : INSTANCE_UV;
    float texId          : INSTANCE_TEXID;
    float billboardType  : INSTANCE_DATA;
};

struct VertexOut
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    uint texId : TEXCOORD1;
    float4 color : TEXCOORD2;
};

//======================================================================================================================//


static float4 VERTICES[6] =
{
    float4(-0.5, -0.5, 0, 0), 
    float4(-0.5, +0.5, 0, 1), 
    float4(+0.5, -0.5, 1, 0), 
                              
    float4(-0.5, +0.5, 0, 1), 
    float4(+0.5, +0.5, 1, 1), 
    float4(+0.5, -0.5, 1, 0), 
};                            

//======================================================================================================================//

float4x4 RotateModelToFaceNormal(float4x4 modelMatrix, float3 targetNormal)
{
    targetNormal = normalize(targetNormal);
    
    float3 worldUp = float3(0.0f, 1.0f, 0.0f);
    float3 right    = normalize(cross(targetNormal, worldUp)); 
    float3 front    = cross(right, targetNormal);
    
    float4x4 rotationMatrix = float4x4(
        float4(right, 0.0f),
        float4(front, 0.0f),
        float4(targetNormal, 0.0f),
        float4(0.0f, 0.0f, 0.0f, 1.0f)
    );
    
    return mul(transpose(rotationMatrix), modelMatrix);
}
//======================================================================================================================//

VertexOut billboardVS(VertexInput VSin, uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID)
{
    float4 position = VSin.instanceTransform3;
    float4x4 M = transpose(float4x4(
            VSin.instanceTransform0,
            VSin.instanceTransform1,
            VSin.instanceTransform2,
            position
    ));  
    
    
    if (VSin.billboardType == 0) // HUD (autofacing + no depth)
    {
        float3 facingNormal = normalize(cameraPosition - position.xyz);
        M = RotateModelToFaceNormal(M, -facingNormal);
    }
    
    if (VSin.billboardType == 1) // Depth write
    {
    }
    
    float4x4 MVP = mul(ViewProj, M);
    
    uint currentID = vertexID - (instanceID * 6);
    float4 vertex = VERTICES[currentID];
    
    VertexOut vso;
    float4 vertexLocalPos = float4(vertex.xy, 0, 1);
    vso.pos = mul(MVP, vertexLocalPos);
    vso.color = float4(1, 1, 1, 1);
    vso.texId = (uint) VSin.texId;
    vso.uv = vertex.zw * VSin.instanceUVs.zw + VSin.instanceUVs.xy;
    vso.uv.x = 1 - vso.uv.x;
    vso.uv.y = 1 - vso.uv.y;
   
    return vso;
}

//======================================================================================================================//

float4 billboardPS(VertexOut vsin) : SV_Target
{
    float4 texSample = float4(0, 0, 0, 1);
    
    for (uint i = 0; i < 16; i++)
    {
        if (i == vsin.texId)
        {
            texSample = textures[i].Sample(MeshTextureSampler, vsin.uv);
        }
    
    }
    return texSample * vsin.color;
}

//======================================================================================================================//

technique11 billboard
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, billboardVS()));
        SetPixelShader(CompileShader(ps_5_0, billboardPS()));
        SetGeometryShader(NULL);
    }
}