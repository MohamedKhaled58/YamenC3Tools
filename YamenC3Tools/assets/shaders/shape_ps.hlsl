cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 TrailColor;
    float Time;
    float3 Padding;
}

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float alpha : ALPHA;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // Simple gradient for trail
    float gradient = 1.0 - input.texCoord.x;
    
    float4 color = TrailColor;
    color.a *= gradient * input.alpha;
    
    return color;
}