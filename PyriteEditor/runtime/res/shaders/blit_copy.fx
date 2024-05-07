#include "blit.hlsl"

float4 BlitCopyPS(VSOut vs) : SV_Target
{
  return blitTexture.Sample(blitSamplerState, vs.texCoord);
}

technique11 BlitCopy
{
  pass pass0
  {
    SetVertexShader(CompileShader(vs_5_0, BlitVS()));
    SetPixelShader(CompileShader(ps_5_0, BlitCopyPS()));
    SetGeometryShader(NULL);
  }
}