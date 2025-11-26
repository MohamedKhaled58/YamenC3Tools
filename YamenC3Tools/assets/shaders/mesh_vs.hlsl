cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 MorphWeights;
    float4 LightDir;
    float4 LightColor;
    float4 CameraPos;
    float Time;
    float3 Padding;
}

struct VS_INPUT
{
    float3 pos0 : POSITION0;
    float3 pos1 : POSITION1;
    float3 pos2 : POSITION2;
    float3 pos3 : POSITION3;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    uint2 boneIndices : BLENDINDICES;
    float2 boneWeights : BLENDWEIGHT;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    float3 worldPos : POSITION;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    
    // Morph target blending
    float3 morphedPos =
        input.pos0 * MorphWeights.x +
        input.pos1 * MorphWeights.y +
        input.pos2 * MorphWeights.z +
        input.pos3 * MorphWeights.w;
    
    // Calculate normal from morph target differences
    float3 tangent1 = input.pos1 - input.pos0;
    float3 tangent2 = input.pos2 - input.pos0;
    float3 localNormal = cross(tangent1, tangent2);
    
    if (length(localNormal) < 0.001)
    {
        localNormal = float3(0, 1, 0);
    }
    else
    {
        localNormal = normalize(localNormal);
    }
    
    // Transform to world space
    float4 worldPos = mul(float4(morphedPos, 1.0f), World);
    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul(localNormal, (float3x3) World));
    
    // View-projection transform
    output.pos = mul(worldPos, View);
    output.pos = mul(output.pos, Projection);
    
    output.texCoord = input.texCoord;
    output.color = input.color;
    
    return output;
}