#include "ParticleRenderer.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <cstring>
#include <cstdlib>

#pragma comment(lib, "d3dcompiler.lib")

bool ParticleRenderer::Initialize(ID3D11Device* device) {
    m_device = device;
    
    // Compile vertex shader
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    
    const char* vsSource = R"(
        cbuffer ConstantBuffer : register(b0) {
            matrix World;
            matrix View;
            matrix Projection;
            float4 CameraRight;
            float4 CameraUp;
            float Time;
            float3 Padding;
        }
        struct VS_INPUT {
            float3 pos : POSITION;
            float2 texCoord : TEXCOORD;
            float4 color : COLOR;
            float size : SIZE;
        };
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 texCoord : TEXCOORD;
            float4 color : COLOR;
        };
        PS_INPUT main(VS_INPUT input) {
            PS_INPUT output;
            float3 worldPos = input.pos;
            worldPos += CameraRight.xyz * (input.texCoord.x - 0.5) * input.size;
            worldPos += CameraUp.xyz * (input.texCoord.y - 0.5) * input.size;
            float4 viewPos = mul(float4(worldPos, 1.0f), View);
            output.pos = mul(viewPos, Projection);
            output.texCoord = input.texCoord;
            output.color = input.color;
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
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "SIZE", 0, DXGI_FORMAT_R32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;
    
    // Compile pixel shader
    ID3DBlob* psBlob = nullptr;
    const char* psSource = R"(
        Texture2D tex : register(t0);
        SamplerState samp : register(s0);
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 texCoord : TEXCOORD;
            float4 color : COLOR;
        };
        float4 main(PS_INPUT input) : SV_TARGET {
            return tex.Sample(samp, input.texCoord) * input.color;
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
    cbDesc.ByteWidth = sizeof(ParticleCB);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);
    if (FAILED(hr)) return false;
    
    // Create blend state for additive blending
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    hr = device->CreateBlendState(&blendDesc, &m_blendState);
    if (FAILED(hr)) return false;
    
    return true;
}

void ParticleRenderer::Shutdown() {
    if (m_blendState) { m_blendState->Release(); m_blendState = nullptr; }
    if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = nullptr; }
    if (m_constantBuffer) { m_constantBuffer->Release(); m_constantBuffer = nullptr; }
    if (m_inputLayout) { m_inputLayout->Release(); m_inputLayout = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }
}

void ParticleRenderer::Update(float deltaTime) {
    // Update particle lifetimes and positions
    for (auto& p : m_particles) {
        if (p.active) {
            p.age += deltaTime;
            p.position.x += p.velocity.x * deltaTime;
            p.position.y += p.velocity.y * deltaTime;
            p.position.z += p.velocity.z * deltaTime;
            
            if (p.age >= p.lifetime) {
                p.active = false;
            }
        }
    }
}

void ParticleRenderer::Render(ID3D11DeviceContext* context, const C3Model::ParticleSystem& ps,
    const XMMATRIX& view, const XMMATRIX& projection, const XMFLOAT3& cameraPos) {
    
    if (!m_vertexShader || !m_pixelShader) return;
    
    // Emit new particles
    static float emitTimer = 0.0f;
    emitTimer += 0.016f;
    while (emitTimer >= (1.0f / ps.emitRate) && m_particles.size() < ps.maxParticles) {
        EmitParticle(ps);
        emitTimer -= (1.0f / ps.emitRate);
    }
    
    // Build vertex buffer from active particles
    std::vector<ParticleVertex> vertices;
    vertices.reserve(m_particles.size() * 4);
    
    XMMATRIX viewInv = XMMatrixInverse(nullptr, view);
    XMFLOAT3 cameraRight, cameraUp;
    XMStoreFloat3(&cameraRight, viewInv.r[0]);
    XMStoreFloat3(&cameraUp, viewInv.r[1]);
    
    for (const auto& p : m_particles) {
        if (!p.active) continue;
        
        float lifeRatio = p.age / p.lifetime;
        XMFLOAT4 color;
        XMStoreFloat4(&color, XMVectorLerp(
            XMLoadFloat4(&ps.startColor),
            XMLoadFloat4(&ps.endColor),
            lifeRatio
        ));
        
        float size = p.size * (1.0f - lifeRatio * 0.5f);
        
        // Create billboard quad
        ParticleVertex v[4];
        v[0].pos = XMFLOAT3(p.position.x - size, p.position.y - size, p.position.z);
        v[0].texCoord = XMFLOAT2(0, 1);
        v[0].color = color;
        v[0].size = size;
        
        v[1].pos = XMFLOAT3(p.position.x + size, p.position.y - size, p.position.z);
        v[1].texCoord = XMFLOAT2(1, 1);
        v[1].color = color;
        v[1].size = size;
        
        v[2].pos = XMFLOAT3(p.position.x - size, p.position.y + size, p.position.z);
        v[2].texCoord = XMFLOAT2(0, 0);
        v[2].color = color;
        v[2].size = size;
        
        v[3].pos = XMFLOAT3(p.position.x + size, p.position.y + size, p.position.z);
        v[3].texCoord = XMFLOAT2(1, 0);
        v[3].color = color;
        v[3].size = size;
        
        vertices.insert(vertices.end(), v, v + 4);
    }
    
    if (vertices.empty()) return;
    
    // Create/update vertex buffer
    if (!m_vertexBuffer || m_lastVertexCount < vertices.size()) {
        if (m_vertexBuffer) m_vertexBuffer->Release();
        
        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = static_cast<UINT>(sizeof(ParticleVertex) * vertices.size());
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
            memcpy(mapped.pData, vertices.data(), sizeof(ParticleVertex) * vertices.size());
            context->Unmap(m_vertexBuffer, 0);
        }
    }
    
    // Update constant buffer
    ParticleCB cb;
    cb.world = XMMatrixTranspose(XMMatrixIdentity());
    cb.view = XMMatrixTranspose(view);
    cb.projection = XMMatrixTranspose(projection);
    cb.cameraRight = XMFLOAT4(cameraRight.x, cameraRight.y, cameraRight.z, 0);
    cb.cameraUp = XMFLOAT4(cameraUp.x, cameraUp.y, cameraUp.z, 0);
    cb.time = 0.0f;
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &cb, sizeof(ParticleCB));
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
    
    // Draw particles
    UINT stride = sizeof(ParticleVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    
    // Draw as triangle strips (2 triangles per particle = 4 vertices)
    for (size_t i = 0; i < vertices.size(); i += 4) {
        context->Draw(4, static_cast<UINT>(i));
    }
}

void ParticleRenderer::EmitParticle(const C3Model::ParticleSystem& ps) {
    Particle p;
    p.position = ps.emitterPos;
    p.velocity = XMFLOAT3(
        (rand() % 100 - 50) / 50.0f * ps.speed,
        (rand() % 100 - 50) / 50.0f * ps.speed,
        (rand() % 100 - 50) / 50.0f * ps.speed
    );
    p.color = ps.startColor;
    p.size = ps.size.x;
    p.lifetime = ps.lifetime;
    p.age = 0.0f;
    p.active = true;
    
    m_particles.push_back(p);
    
    // Remove old particles
    if (m_particles.size() > ps.maxParticles) {
        m_particles.erase(
            std::remove_if(m_particles.begin(), m_particles.end(),
                [](const Particle& p) { return !p.active; }),
            m_particles.end()
        );
    }
}