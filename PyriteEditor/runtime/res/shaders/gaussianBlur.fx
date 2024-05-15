Texture2D sourceTexture;

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

float u_blurStrength = 0.05;
int sampleCount = 5;

/////////////////////////////////////////////////////////////////////////////////////////////

float gaussian(float2 i)
{
    return exp(-.5 * dot(i /= (sampleCount * .25), i)) / sqrt(6.28 * (sampleCount * .25) * (sampleCount * .25));
}


float4 blur(Texture2D tex, float2 uv, float scale)
{
    float4 outBlur = float4(0, 0, 0, 0);
    
    for (int i = 0; i < sampleCount * sampleCount; i++)
    {
        float2 offset = float2(i % sampleCount, i / sampleCount) - sampleCount / 2.f;
        offset /= float2(1540, 845);
        //O += gaussian(d) * sp.Load(int3(U + scale * d, 0));
        outBlur += gaussian(offset) * float4(tex.Sample(blitSamplerState, uv + offset * scale).rgb, 1.0f);
    }
    
    return outBlur / outBlur.a;
}


/////////////////////////////////////////////////////////////////////////////////////////////

struct VSOut
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

VSOut GaussianBlurVS(uint vertexId : SV_VertexID)
{
    float4 vertex = VERTICES[vertexId];
    VSOut vsOut = (VSOut) 0;
    vsOut.position = vertex;
    vsOut.texCoord = vertex.xy * float2(.5, -.5) + .5;
    return vsOut;
}

float4 GaussianBlurPS(VSOut vs) : SV_Target
{
    //return sourceTexture.Sample(blitSamplerState, vs.texCoord);
    return blur(sourceTexture, vs.texCoord, u_blurStrength);
}

technique11 BlitCopy
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, GaussianBlurVS()));
        SetPixelShader(CompileShader(ps_5_0, GaussianBlurPS()));
        SetGeometryShader(NULL);
    }
}