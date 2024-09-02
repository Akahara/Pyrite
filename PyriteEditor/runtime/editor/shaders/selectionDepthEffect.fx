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

struct VertexOut
{
    float4 Pos : SV_Position;
    float4 worldpos : POSITION;
    float2 uv : UV;
};



//======================================================================================================================//

float delinearize_depth(float d, float znear, float zfar)
{
    return znear * zfar / (zfar + d * (znear - zfar));
}


float gridSDF(float2 uv, float grid_size)
{
    float2 a = step(grid_size/2.F, frac(uv / grid_size) * grid_size);
    return step(0.99, a.x + a.y);
}

float2 rotateUvs(float2 uv, float angleDeg)
{
    return mul(uv, (float2x2(cos(angleDeg), sin(angleDeg), -sin(angleDeg), cos(angleDeg))));
}

//======================================================================================================================//

Texture2D depthBuffer;
float cameraZnear = 0.1f;
float cameraZfar = 10000.f;
float4 gridColor = float4(1, 0.45, 0, 1);

VertexOut DrawMesh(VertexInput vsIn)
{
    VertexOut vsOut;
    float4x4 MVP = mul(ViewProj, ModelMatrix);
    
    vsOut.Pos       = mul(MVP, vsIn.Pos);
    vsOut.worldpos  = mul(ModelMatrix, vsIn.Pos);
    vsOut.uv        = vsIn.uv;
    
    return vsOut;
}
float4 ColorID(VertexOut vsOut) : SV_Target
{
    float lin_depth = depthBuffer.Load(vsOut.Pos.xyz);
    float meshDepth = vsOut.Pos.z;
    
    float4 pixelColor = float4(1.f.xxx, 0);
    if (meshDepth > lin_depth)
    {
        float angle = radians(100);
        float2 rotatedUvs = rotateUvs(vsOut.Pos.xy, angle);
        float sdf = gridSDF(rotatedUvs, 3.F);
        pixelColor = gridColor * step(sdf, .5);
    }
    
   
    return pixelColor;
}

//======================================================================================================================//

technique11 SelectionOutline
{
    pass pass0
    {
        SetVertexShader(CompileShader(vs_5_0, DrawMesh()));
        SetPixelShader(CompileShader(ps_5_0, ColorID()));
        SetGeometryShader(NULL);
    }
}

