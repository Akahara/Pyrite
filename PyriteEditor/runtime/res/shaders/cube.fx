cbuffer cbCube
{
  float4x4 vMatViewProj;
  float4x4 transformMat;
  float4 vCubeColor;
};

static float3 VERTICES[36] =
{
  float3(-1, -1, -1), float3(-1, -1, +1), float3(-1, +1, +1),
  float3(+1, +1, -1), float3(-1, -1, -1), float3(-1, +1, -1),
  float3(+1, -1, +1), float3(-1, -1, -1), float3(+1, -1, -1),
  float3(+1, +1, -1), float3(+1, -1, -1), float3(-1, -1, -1),
  float3(-1, -1, -1), float3(-1, +1, +1), float3(-1, +1, -1),
  float3(+1, -1, +1), float3(-1, -1, +1), float3(-1, -1, -1),
  float3(-1, +1, +1), float3(-1, -1, +1), float3(+1, -1, +1),
  float3(+1, +1, +1), float3(+1, -1, -1), float3(+1, +1, -1),
  float3(+1, -1, -1), float3(+1, +1, +1), float3(+1, -1, +1),
  float3(+1, +1, +1), float3(+1, +1, -1), float3(-1, +1, -1),
  float3(+1, +1, +1), float3(-1, +1, -1), float3(-1, +1, +1),
  float3(+1, +1, +1), float3(-1, +1, +1), float3(+1, -1, +1),
};

float4 CubeVS(uint vertexId : SV_VertexID) : SV_Position
{
    return mul(vMatViewProj, mul(transformMat, float4(VERTICES[vertexId], 1)));
}

float4 CubePS() : SV_Target
{
  return vCubeColor;
}

technique11 MiniPhong
{
  pass pass0
  {
    SetVertexShader(CompileShader(vs_5_0, CubeVS()));
    SetPixelShader(CompileShader(ps_5_0, CubePS()));
    SetGeometryShader(NULL);
  }
}