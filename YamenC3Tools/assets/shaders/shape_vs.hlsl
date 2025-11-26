cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 TrailColor;
    float Time;
    float3 Padding;
}

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float alpha : ALPHA;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float alpha : ALPHA;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 worldPos = mul(float4(input.pos, 1.0f), World);
    output.pos = mul(worldPos, View);
    output.pos = mul(output.pos, Projection);
    
    output.texCoord = input.texCoord;
    output.alpha = input.alpha;
    
    return output;
}