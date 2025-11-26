#include "ShapeRenderer.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

bool ShapeRenderer::Initialize(ID3D11Device* device) {
    m_device = device;
    
    // Compile vertex shader
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    
    const char* vsSource = R"(
        cbuffer ConstantBuffer : register(b0) {
            matrix World;
            matrix View;
            matrix Projection;
            float4 TrailColor;
            float Time;
            float3 Padding;
        }
        struct VS_INPUT {
            float3 pos : POSITION;
            float2 texCoord : TEXCOORD;
            float alpha : ALPHA;
        };
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 texCoord : TEXCOORD;
            float alpha : ALPHA;
        };
        PS_INPUT main(VS_INPUT input) {
            PS_INPUT output;
            float4 worldPos = mul(float4(input.pos, 1.0f), World);
            output.pos = mul(worldPos, View);
            output.pos = mul(output.pos, Projection);
            output.texCoord = input.texCoord;
            output.alpha = input.alpha;
            return output;
        }
    )";
    
    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) errorBlob->Release();
        return false;
    }
    
    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    if (FAILED(hr)) {
        vsBlob->Release();
        return false;
    }
    
    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "ALPHA", 0, DXGI_FORMAT_R32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;
    
    // Compile pixel shader
    ID3DBlob* psBlob = nullptr;
    const char* psSource = R"(
        Texture2D tex : register(t0);
        SamplerState samp : register(s0);
        cbuffer ConstantBuffer : register(b0) {
            float4 TrailColor;
        };
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 texCoord : TEXCOORD;
            float alpha : ALPHA;
        };
        float4 main(PS_INPUT input) : SV_TARGET {
            float4 texColor = tex.Sample(samp, input.texCoord);
            return float4(TrailColor.rgb * texColor.rgb, TrailColor.a * texColor.a * input.alpha);
        }
    )";
    
    hr = D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) errorBlob->Release();
        return false;
    }
    
    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
    psBlob->Release();
    if (FAILED(hr)) return false;
    
    // Create constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(ShapeCB);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);
    if (FAILED(hr)) return false;
    
    // Create blend state for alpha blending
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    hr = device->CreateBlendState(&blendDesc, &m_blendState);
    if (FAILED(hr)) return false;
    
    return true;
}

void ShapeRenderer::Shutdown() {
    if (m_blendState) { m_blendState->Release(); m_blendState = nullptr; }
    if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = nullptr; }
    if (m_constantBuffer) { m_constantBuffer->Release(); m_constantBuffer = nullptr; }
    if (m_inputLayout) { m_inputLayout->Release(); m_inputLayout = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }
}

void ShapeRenderer::Render(ID3D11DeviceContext* context, const C3Model::ShapeData& shape,
    const XMMATRIX& view, const XMMATRIX& projection) {
    
    if (!m_vertexShader || !m_pixelShader || shape.lines.empty()) return;
    
    // Build vertex buffer from shape lines (trail effect)
    std::vector<ShapeVertex> vertices;
    
    // For each line, create a ribbon mesh
    for (const auto& line : shape.lines) {
        if (line.points.size() < 2) continue;
        
        // Create trail segments
        uint32_t segmentCount = shape.segmentCount > 0 ? shape.segmentCount : 20;
        uint32_t pointsToUse = std::min(static_cast<uint32_t>(line.points.size()), segmentCount);
        
        for (uint32_t i = 0; i < pointsToUse - 1; i++) {
            const XMFLOAT3& p0 = line.points[i];
            const XMFLOAT3& p1 = line.points[i + 1];
            
            // Calculate perpendicular direction for trail width
            XMVECTOR dir = XMVectorSubtract(XMLoadFloat3(&p1), XMLoadFloat3(&p0));
            XMVECTOR perp = XMVector3Cross(dir, XMVectorSet(0, 1, 0, 0));
            perp = XMVector3Normalize(perp);
            XMVECTOR width = XMVectorScale(perp, 0.1f);
            
            XMFLOAT3 width3;
            XMStoreFloat3(&width3, width);
            
            float alpha = 1.0f - (float)i / (float)pointsToUse;
            
            // Create quad for this segment
            ShapeVertex v[6];
            
            v[0].pos = XMFLOAT3(p0.x - width3.x, p0.y - width3.y, p0.z - width3.z);
            v[0].texCoord = XMFLOAT2(0, alpha);
            v[0].alpha = alpha;
            
            v[1].pos = XMFLOAT3(p0.x + width3.x, p0.y + width3.y, p0.z + width3.z);
            v[1].texCoord = XMFLOAT2(1, alpha);
            v[1].alpha = alpha;
            
            v[2].pos = XMFLOAT3(p1.x - width3.x, p1.y - width3.y, p1.z - width3.z);
            v[2].texCoord = XMFLOAT2(0, alpha - 1.0f / pointsToUse);
            v[2].alpha = alpha - 1.0f / pointsToUse;
            
            v[3].pos = XMFLOAT3(p1.x - width3.x, p1.y - width3.y, p1.z - width3.z);
            v[3].texCoord = XMFLOAT2(0, alpha - 1.0f / pointsToUse);
            v[3].alpha = alpha - 1.0f / pointsToUse;
            
            v[4].pos = XMFLOAT3(p0.x + width3.x, p0.y + width3.y, p0.z + width3.z);
            v[4].texCoord = XMFLOAT2(1, alpha);
            v[4].alpha = alpha;
            
            v[5].pos = XMFLOAT3(p1.x + width3.x, p1.y + width3.y, p1.z + width3.z);
            v[5].texCoord = XMFLOAT2(1, alpha - 1.0f / pointsToUse);
            v[5].alpha = alpha - 1.0f / pointsToUse;
            
            vertices.insert(vertices.end(), v, v + 6);
        }
    }
    
    if (vertices.empty()) return;
    
    // Create/update vertex buffer
    if (!m_vertexBuffer || m_lastVertexCount < vertices.size()) {
        if (m_vertexBuffer) m_vertexBuffer->Release();
        
        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = static_cast<UINT>(sizeof(ShapeVertex) * vertices.size());
        vbd.Usage = D3D11_USAGE_DYNAMIC;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        D3D11_SUBRESOURCE_DATA vData = {};
        vData.pSysMem = vertices.data();
        
        if (FAILED(m_device->CreateBuffer(&vbd, &vData, &m_vertexBuffer))) {
            return;
        }
        m_lastVertexCount = vertices.size();
    } else {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            memcpy(mapped.pData, vertices.data(), sizeof(ShapeVertex) * vertices.size());
            context->Unmap(m_vertexBuffer, 0);
        }
    }
    
    // Update constant buffer
    ShapeCB cb;
    cb.world = XMMatrixTranspose(XMMatrixIdentity());
    cb.view = XMMatrixTranspose(view);
    cb.projection = XMMatrixTranspose(projection);
    cb.trailColor = XMFLOAT4(1, 1, 1, 1);
    cb.time = 0.0f;
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &cb, sizeof(ShapeCB));
        context->Unmap(m_constantBuffer, 0);
    }
    
    // Set render state
    context->VSSetShader(m_vertexShader, nullptr, 0);
    context->PSSetShader(m_pixelShader, nullptr, 0);
    context->IASetInputLayout(m_inputLayout);
    context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    context->PSSetConstantBuffers(0, 1, &m_constantBuffer);
    
    float blendFactor[4] = { 1, 1, 1, 1 };
    context->OMSetBlendState(m_blendState, blendFactor, 0xFFFFFFFF);
    
    // Draw shape
    UINT stride = sizeof(ShapeVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(static_cast<UINT>(vertices.size()), 0);
}