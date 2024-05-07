
TextureCube cubemap;
SamplerState cubemapSampler;

cbuffer cbWorld
{
  float4x4 matViewProj;
  float3 cameraPosition;
};

static float4 VERTICES[36] =
{
  float4(-1,-1,+1,1), float4(-1,-1,-1,1), float4(-1,+1,+1,1),
  float4(-1,-1,-1,1), float4(+1,+1,-1,1), float4(-1,+1,-1,1),
  float4(-1,-1,-1,1), float4(+1,-1,+1,1), float4(+1,-1,-1,1),
  float4(+1,-1,-1,1), float4(+1,+1,-1,1), float4(-1,-1,-1,1),
  float4(-1,+1,+1,1), float4(-1,-1,-1,1), float4(-1,+1,-1,1),
  float4(-1,-1,+1,1), float4(+1,-1,+1,1), float4(-1,-1,-1,1),
  float4(-1,-1,+1,1), float4(-1,+1,+1,1), float4(+1,-1,+1,1),
  float4(+1,-1,-1,1), float4(+1,+1,+1,1), float4(+1,+1,-1,1),
  float4(+1,+1,+1,1), float4(+1,-1,-1,1), float4(+1,-1,+1,1),
  float4(+1,+1,-1,1), float4(+1,+1,+1,1), float4(-1,+1,-1,1),
  float4(-1,+1,-1,1), float4(+1,+1,+1,1), float4(-1,+1,+1,1),
  float4(-1,+1,+1,1), float4(+1,+1,+1,1), float4(+1,-1,+1,1),
};

struct VSOut
{
  float4 position : SV_Position;
  float4 worldPos : TEXCOORD0;
};

VSOut SkyboxVS(uint vertexId : SV_VertexID)
{
  VSOut vsOut = (VSOut) 0;
  vsOut.worldPos = VERTICES[vertexId];
  vsOut.position = mul(matViewProj, vsOut.worldPos + float4(cameraPosition, 0));
  vsOut.position.z = vsOut.position.w; // force depth to be greater than every object in the scene
  return vsOut;
}

float4 SkyboxPS(VSOut vs) : SV_Target
{
  return cubemap.Sample(cubemapSampler, vs.worldPos.xyz);
}

technique11 MiniPhong
{
  pass pass0
  {
    SetVertexShader(CompileShader(vs_5_0, SkyboxVS()));
    SetPixelShader(CompileShader(ps_5_0, SkyboxPS()));
    SetGeometryShader(NULL);
  }
}