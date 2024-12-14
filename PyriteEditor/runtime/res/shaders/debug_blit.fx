#include "blit.hlsl"

int u_BufferArrayVisualisedIndex;
Texture2DArray BufferArray;
Texture2D VisualisedBuffer;

float4 BlitDebugPS(VSOut vs) : SV_Target
{
    if (u_BufferArrayVisualisedIndex < 0)
    {
        return VisualisedBuffer.Sample(blitSamplerState, vs.texCoord);
    }
    else
    {
        return BufferArray.Sample(blitSamplerState, float3(vs.texCoord, u_BufferArrayVisualisedIndex));
    }
}

technique11 BlitDebug
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, BlitVS()));
        SetPixelShader(CompileShader(ps_5_0, BlitDebugPS()));
        SetGeometryShader(NULL);
    }
}