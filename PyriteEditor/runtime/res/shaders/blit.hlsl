Texture2D blitTexture;
SamplerState blitSamplerState
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Clamp;
  AddressV = Clamp;
};

static float4 VERTICES[3] =
{
  float4(-1, -1, 0, 1),
  float4(-1, +3, 0, 1),
  float4(+3, -1, 0, 1),
};

struct VSOut
{
  float4 position : SV_Position;
  float2 texCoord : TEXCOORD0;
};

VSOut BlitVS(uint vertexId : SV_VertexID)
{
  float4 vertex = VERTICES[vertexId];
  VSOut vsOut = (VSOut) 0;
  vsOut.position = vertex;
  vsOut.texCoord = vertex.xy * float2(.5, -.5) + .5;
  return vsOut;
}