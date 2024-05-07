static const int MSAA_LEVEL = 8;

float4 sampleMSAA(Texture2DMS<float4> tex, float2 uv)
{
  float w, h, s;
  tex.GetDimensions(w, h, s);
  float4 c = float4(0, 0, 0, 0);
  for (int i = 0; i < MSAA_LEVEL; i++)
    c += tex.Load(uv * int2(w, h), i);
  return c / MSAA_LEVEL;
}

float sampleMSAA(Texture2DMS<float> tex, float2 uv)
{
  float w, h, s;
  tex.GetDimensions(w, h, s);
  float c = 0.f;
  for (int i = 0; i < MSAA_LEVEL; i++)
    c += tex.Load(uv * int2(w, h), i);
  return c / MSAA_LEVEL;
}
