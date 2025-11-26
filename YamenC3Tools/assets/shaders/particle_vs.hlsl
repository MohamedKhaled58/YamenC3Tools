cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 CameraRight;
    float4 CameraUp;
    float Time;
    float3 Padding;
}

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    float size : SIZE;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    
    // Billboard quad facing camera
    float3 worldPos = input.pos;
    worldPos += CameraRight.xyz * (input.texCoord.x - 0.5) * input.size;
    worldPos += CameraUp.xyz * (input.texCoord.y - 0.5) * input.size;
    
    float4 viewPos = mul(float4(worldPos, 1.0f), View);
    output.pos = mul(viewPos, Projection);
    
    output.texCoord = input.texCoord;
    output.color = input.color;
    
    return output;
}