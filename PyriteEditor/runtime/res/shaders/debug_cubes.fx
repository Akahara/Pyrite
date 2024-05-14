cbuffer CameraBuffer {
  float4x4 matViewProj;
};

struct VSIn {
  float4 position : POSITION;
  float4 instanceTransform0 : INSTANCE_TRANSFORM0;
  float4 instanceTransform1 : INSTANCE_TRANSFORM1;
  float4 instanceTransform2 : INSTANCE_TRANSFORM2;
  float4 instanceTransform3 : INSTANCE_TRANSFORM3;
  float4 color : INSTANCE_COLOR;
};

struct VSOut {
  float4 position : SV_Position;
  float4 color : TEXCOORD0;
};

VSOut SpriteVS(VSIn vsIn)
{
  VSOut vsOut = (VSOut)0;
  vsOut.position = mul(matViewProj, mul(transpose(float4x4(
    vsIn.instanceTransform0,
    vsIn.instanceTransform1,
    vsIn.instanceTransform2,
    vsIn.instanceTransform3
  )), vsIn.position));
  vsOut.color = vsIn.color;
  return vsOut;
}

float4 SpritePS(VSOut vs) : SV_Target
{
  return vs.color;
}

technique11 MiniPhong {
  pass pass0 {
    SetVertexShader(CompileShader(vs_5_0, SpriteVS()));
    SetPixelShader(CompileShader(ps_5_0, SpritePS()));
    SetGeometryShader(NULL);
  }
}