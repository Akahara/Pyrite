cbuffer cbCamera
{
  float4x4 matViewProj;
};

struct VSIn
{
  float4 position : POSITION;
  float4 color : COLOR;
};

struct VSOut
{
  float4 position : SV_Position;
  float4 color : TEXCOORD0;
};

VSOut SpriteVS(VSIn vsIn)
{
  VSOut vsOut = (VSOut) 0;
  vsOut.position = mul(matViewProj, vsIn.position);
  vsOut.color = vsIn.color;
  return vsOut;
}

float4 SpritePS(VSOut vs) : SV_Target
{
  return vs.color;
}

technique11 MiniPhong
{
  pass pass0
  {
    SetVertexShader(CompileShader(vs_5_0, SpriteVS()));
    SetPixelShader(CompileShader(ps_5_0, SpritePS()));
    SetGeometryShader(NULL);
  }
}