//======================================================================================================================//

#include "../../res/shaders/incl/cbuffers.incl"
#include "../../res/shaders/incl/samplers.incl"
#include "../../res/shaders/incl/transform_utils.incl"

//======================================================================================================================//

cbuffer ActorPickerIDBuffer
{
    uint actorId;
};

#ifdef USE_BILLBOARDS
cbuffer BillboardPickerIDBuffer
{
    uint billboardsID[16];
};
#endif

//======================================================================================================================//

// -- Quick hacky stuff to ensure default value being use_mesh
#ifdef USE_MESH
struct VertexInput
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : UV;
};
#else
#ifdef USE_BILLBOARDS
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

struct VertexOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    uint texId : TEXCOORD1;
    uint billboardActorId : TEXCOORD2;
};

static const int AUTO_FACING = 0;
Texture2D textures[16];
#endif
#endif

//======================================================================================================================//


#ifdef USE_BILLBOARDS
static float4 VERTICES[6] =
{
    float4(-0.5, -0.5, 0, 0),
    float4(-0.5, +0.5, 0, 1),
    float4(+0.5, -0.5, 1, 0),
                              
    float4(-0.5, +0.5, 0, 1),
    float4(+0.5, +0.5, 1, 1),
    float4(+0.5, -0.5, 1, 0),
};
#endif


//======================================================================================================================//

#ifdef USE_MESH
float4 PickerVertexShader(VertexInput meshVSin) : SV_Position
{
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    return mul(MVP, meshVSin.Pos);
}
#else
#ifdef USE_BILLBOARDS
VertexOutput PickerVertexShader(VertexInput bbVSin, uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID)
{
    VertexOutput vso;

    float4 position = bbVSin.instanceTransform3;
    float4x4 M = transpose(float4x4(
            bbVSin.instanceTransform0,
            bbVSin.instanceTransform1,
            bbVSin.instanceTransform2,
            position
    ));
    
    if (bbVSin.billboardType == AUTO_FACING) // HUD (autofacing + no depth)
    {
        float3 facingNormal = normalize(cameraPosition - position.xyz);
        M = RotateModelToFaceNormal(M, -facingNormal);
    }
    
    float4x4 MVP = mul(ViewProj, M);
    
    uint currentID = vertexID % 6;
    float4 vertex = VERTICES[currentID];
    float4 vertexLocalPos = float4(vertex.xy, 0, 1);
    vso.pos = mul(MVP, vertexLocalPos);
    vso.billboardActorId = billboardsID[instanceID];

    vso.uv = vertex.zw * bbVSin.instanceUVs.zw + bbVSin.instanceUVs.xy;
    vso.uv.x = 1 - vso.uv.x;
    vso.uv.y = 1 - vso.uv.y;

    vso.texId = (uint) bbVSin.texId;
    return vso;

}
#endif
#endif

#ifdef USE_BILLBOARDS
float ColorID(VertexOutput vsIn) : SV_Target
{
    float4 texSample = float4(0, 0, 0, 1);
    
    for (uint i = 0; i < 16; i++)
    {
        if (i == vsIn.texId)
        {
            texSample = textures[i].Sample(MeshTextureSampler, vsIn.uv);
        }
    
    }
    if (texSample.a <= 0.9999) discard;

    return !!texSample.a * float(vsIn.billboardActorId);
}
#else
float ColorID() : SV_Target
{
    return float(actorId);
}
#endif
    

//======================================================================================================================//

technique11 PickerShader
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, PickerVertexShader()));
        SetPixelShader(CompileShader(ps_5_0, ColorID()));
        SetGeometryShader(NULL);
    }
}

