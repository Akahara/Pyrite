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

Texture2D sceneTexture;
Texture2D selectedMeshesDepth;
Texture2D sceneDepth;

float outlineScale = 1.F;
float4 outlineColor = float4(1, 0.45, 0, 1);

//======================================================================================================================//

const int2 texAddrOffsets[8] =
{
    int2(-1, -1),
    int2(0, -1),
    int2(1, -1),
    int2(-1, 0),
    int2(1, 0),
    int2(-1, 1),
    int2(0, 1),
    int2(1, 1),
};

float SobelSDF_2(float2 uv, float2 screenSize)
{
    float lum[8];
    int i;

    float3 LuminanceConv = { 0.2125f, 0.7154f, 0.0721f };

    uint width, height, levels;

    for (i = 0; i < 8; i++)
    {
        float2 uv_offset = uv + (texAddrOffsets[i] / screenSize) * outlineScale;
        float d = selectedMeshesDepth.Sample(blitSamplerState, uv_offset).r;
        lum[i] = step(0.0001, 1 - d);
    }

    float x = lum[0] + 2 * lum[3] + lum[5] - lum[2] - 2 * lum[4] - lum[7];
    float y = lum[0] + 2 * lum[1] + lum[2] - lum[5] - 2 * lum[6] - lum[7];
    float edge = sqrt(x * x + y * y);
    return edge;
}

//======================================================================================================================//

VSOut  BlitVS(uint vertexId : SV_VertexID)
{
    float4 vertex = VERTICES[vertexId];
    VSOut vsOut = (VSOut) 0;
    vsOut.position = vertex;
    vsOut.texCoord = vertex.xy * float2(.5, -.5) + .5;
    return vsOut;
}

float4 OutlinePS(VSOut vs) : SV_Target
{
    float depth = selectedMeshesDepth.Sample(blitSamplerState, vs.texCoord).r;
    
    uint width, height;
    selectedMeshesDepth.GetDimensions(width, height);
    
    float sobelSDF = SobelSDF_2(vs.texCoord, float2(width, height));
    return sobelSDF * outlineColor;
}

//======================================================================================================================//

technique11 selectionOutlineTechnique
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, BlitVS()));
        SetPixelShader(CompileShader(ps_5_0, OutlinePS()));
        SetGeometryShader(NULL);
    }
}