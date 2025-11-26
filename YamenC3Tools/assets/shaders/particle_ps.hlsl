struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // Radial gradient for soft particles
    float2 center = input.texCoord - 0.5;
    float dist = length(center);
    float alpha = 1.0 - smoothstep(0.0, 0.5, dist);
    
    float4 color = input.color;
    color.a *= alpha;
    
    return color;
}