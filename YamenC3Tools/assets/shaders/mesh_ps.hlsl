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

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    float3 worldPos : POSITION;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(-LightDir.xyz);
    float3 viewDir = normalize(CameraPos.xyz - input.worldPos);
    
    // Ambient
    float3 ambient = float3(0.5, 0.5, 0.55);
    
    // Diffuse (Lambert)
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * LightColor.rgb;
    
    // Specular (Blinn-Phong)
    float3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    float3 specular = spec * LightColor.rgb * 0.5;
    
    // Rim lighting for better silhouette
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = pow(rim, 3.0) * 0.3;
    
    // Combine lighting
    float3 baseColor = input.color.rgb;
    float3 finalColor = (ambient + diffuse + specular + rim) * baseColor;
    
    return float4(finalColor, input.color.a);
}