//======================================================================================================================//

#include "../../res/shaders/incl/cbuffers.incl"
#include "../../res/shaders/incl/samplers.incl"

//======================================================================================================================//

struct VertexInput
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

struct VSOut
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

//======================================================================================================================//

static float4 VERTICES[3] =
{
  float4(-1, +3, 0, 1),
  float4(-1, -1, 0, 1),
  float4(+3, -1, 0, 1),
};

//======================================================================================================================//

Texture2D outlineTexture;
Texture2D depthGridTexture;

//======================================================================================================================//

VSOut BlitVS(uint vertexId : SV_VertexID)
{
    float4 vertex = VERTICES[vertexId];
    VSOut vsOut = (VSOut) 0;
    vsOut.position = vertex;
    vsOut.texCoord = vertex.xy * float2(.5, -.5) + .5;
    return vsOut;
}

float4 OutputPicker(VSOut vs) : SV_Target
{
    float2 uvs = vs.texCoord;
    
    float4 outline      = outlineTexture.Sample(blitSamplerState, uvs);
    float4 depthGrid    = depthGridTexture.Sample(blitSamplerState, uvs);
    
    return (outline * outline.a) + (depthGrid * depthGrid.a);
    
}

//======================================================================================================================//

technique11 selectionOutlineTechnique
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, BlitVS()));
        SetPixelShader(CompileShader(ps_5_0, OutputPicker()));
        SetGeometryShader(NULL);
    }
}