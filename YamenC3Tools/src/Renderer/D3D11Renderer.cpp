#include "D3D11Renderer.h"
#include <d3dcompiler.h>
#include <algorithm>

#pragma comment(lib, "d3dcompiler.lib")

D3D11Renderer::D3D11Renderer() {}

D3D11Renderer::~D3D11Renderer() {
    Shutdown();
}

bool D3D11Renderer::Initialize(HWND hwnd, int width, int height) {
    m_hwnd = hwnd;
    m_width = width;
    m_height = height;

    if (!CreateDeviceAndSwapChain()) return false;
    if (!CreateRenderTargets()) return false;
    if (!CreateDepthStencil()) return false;
    if (!CompileShaders()) return false;
    if (!CreateStates()) return false;

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);

    return true;
}

void D3D11Renderer::Shutdown() {
    CleanupMeshBuffers();

    if (m_depthStateReadOnly) { m_depthStateReadOnly->Release(); m_depthStateReadOnly = nullptr; }
    if (m_depthStateReadWrite) { m_depthStateReadWrite->Release(); m_depthStateReadWrite = nullptr; }
    if (m_blendStateAlpha) { m_blendStateAlpha->Release(); m_blendStateAlpha = nullptr; }
    if (m_blendStateOpaque) { m_blendStateOpaque->Release(); m_blendStateOpaque = nullptr; }
    if (m_rasterizerStateWireframe) { m_rasterizerStateWireframe->Release(); m_rasterizerStateWireframe = nullptr; }
    if (m_rasterizerStateSolid) { m_rasterizerStateSolid->Release(); m_rasterizerStateSolid = nullptr; }
    if (m_constantBuffer) { m_constantBuffer->Release(); m_constantBuffer = nullptr; }
    if (m_inputLayout) { m_inputLayout->Release(); m_inputLayout = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }
    if (m_depthStencilBuffer) { m_depthStencilBuffer->Release(); m_depthStencilBuffer = nullptr; }
    if (m_depthStencilView) { m_depthStencilView->Release(); m_depthStencilView = nullptr; }
    if (m_renderTargetView) { m_renderTargetView->Release(); m_renderTargetView = nullptr; }
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
}

bool D3D11Renderer::CreateDeviceAndSwapChain() {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;  // Changed from 2 to 1 for simplicity
    scd.BufferDesc.Width = m_width;
    scd.BufferDesc.Height = m_height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = m_hwnd;
    scd.SampleDesc.Count = 1;  // Changed from 4 to 1 (no MSAA for simplicity)
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_DEBUG,  // Debug flag in CORRECT position
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &scd,
        &m_swapChain,
        &m_device,
        &featureLevel,
        &m_context
    );

    return SUCCEEDED(hr);
}

bool D3D11Renderer::CreateRenderTargets() {
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) return false;

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
    backBuffer->Release();

    return SUCCEEDED(hr);
}

bool D3D11Renderer::CreateDepthStencil() {
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = m_width;
    depthDesc.Height = m_height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;  // Must match swap chain sample count
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    HRESULT hr = m_device->CreateTexture2D(&depthDesc, nullptr, &m_depthStencilBuffer);
    if (FAILED(hr)) return false;

    hr = m_device->CreateDepthStencilView(m_depthStencilBuffer, nullptr, &m_depthStencilView);
    if (FAILED(hr)) return false;

    m_context->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
    return true;
}

void D3D11Renderer::Resize(int width, int height) {
    if (!m_swapChain) return;

    m_width = width;
    m_height = height;

    m_context->OMSetRenderTargets(0, nullptr, nullptr);

    if (m_renderTargetView) {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }

    if (m_depthStencilView) {
        m_depthStencilView->Release();
        m_depthStencilView = nullptr;
    }

    if (m_depthStencilBuffer) {
        m_depthStencilBuffer->Release();
        m_depthStencilBuffer = nullptr;
    }

    m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    CreateRenderTargets();
    CreateDepthStencil();

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);
}

void D3D11Renderer::SetMorphWeights(float w0, float w1, float w2, float w3) {
    m_morphWeights[0] = w0;
    m_morphWeights[1] = w1;
    m_morphWeights[2] = w2;
    m_morphWeights[3] = w3;
}

void D3D11Renderer::SetWireframe(bool enabled) {
    m_wireframe = enabled;
}

void D3D11Renderer::CleanupMeshBuffers() {
    for (auto& mb : m_meshBuffers) {
        if (mb.vertexBuffer) mb.vertexBuffer->Release();
        if (mb.normalIndexBuffer) mb.normalIndexBuffer->Release();
        if (mb.alphaIndexBuffer) mb.alphaIndexBuffer->Release();
    }
    m_meshBuffers.clear();
}

bool D3D11Renderer::CompileShaders() {
    // Vertex Shader Source
    const char* vsSource = R"(
        cbuffer ConstantBuffer : register(b0) {
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

        struct VS_INPUT {
            float3 pos0 : POSITION0;
            float3 pos1 : POSITION1;
            float3 pos2 : POSITION2;
            float3 pos3 : POSITION3;
            float2 texCoord : TEXCOORD;
            float4 color : COLOR;
            uint2 boneIndices : BLENDINDICES;
            float2 boneWeights : BLENDWEIGHT;
        };

        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float3 normal : NORMAL;
            float2 texCoord : TEXCOORD;
            float4 color : COLOR;
            float3 worldPos : POSITION;
        };

        PS_INPUT main(VS_INPUT input) {
            PS_INPUT output;
            
            float3 morphedPos = 
                input.pos0 * MorphWeights.x +
                input.pos1 * MorphWeights.y +
                input.pos2 * MorphWeights.z +
                input.pos3 * MorphWeights.w;
            
            float3 tangent1 = input.pos1 - input.pos0;
            float3 tangent2 = input.pos2 - input.pos0;
            float3 localNormal = cross(tangent1, tangent2);
            
            if (length(localNormal) < 0.001) {
                localNormal = float3(0, 1, 0);
            } else {
                localNormal = normalize(localNormal);
            }
            
            float4 worldPos = mul(float4(morphedPos, 1.0f), World);
            output.worldPos = worldPos.xyz;
            output.normal = normalize(mul(localNormal, (float3x3)World));
            output.pos = mul(worldPos, View);
            output.pos = mul(output.pos, Projection);
            output.texCoord = input.texCoord;
            output.color = input.color;
            
            return output;
        }
    )";

    // Pixel Shader Source
    const char* psSource = R"(
        cbuffer ConstantBuffer : register(b0) {
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

        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float3 normal : NORMAL;
            float2 texCoord : TEXCOORD;
            float4 color : COLOR;
            float3 worldPos : POSITION;
        };

        float4 main(PS_INPUT input) : SV_TARGET {
            float3 normal = normalize(input.normal);
            float3 lightDir = normalize(-LightDir.xyz);
            float3 viewDir = normalize(CameraPos.xyz - input.worldPos);
            
            float3 ambient = float3(0.5, 0.5, 0.55);
            float diff = max(dot(normal, lightDir), 0.0);
            float3 diffuse = diff * LightColor.rgb;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
            float3 specular = spec * LightColor.rgb * 0.5;
            
            float rim = 1.0 - max(dot(viewDir, normal), 0.0);
            rim = pow(rim, 3.0) * 0.3;
            
            float3 baseColor = input.color.rgb;
            float3 finalColor = (ambient + diffuse + specular + rim) * baseColor;
            
            return float4(finalColor, input.color.a);
        }
    )";

    // Compile Vertex Shader
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(
        vsSource, strlen(vsSource),
        nullptr, nullptr, nullptr,
        "main", "vs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &vsBlob, &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return false;
    }

    hr = m_device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &m_vertexShader
    );

    if (FAILED(hr)) {
        vsBlob->Release();
        return false;
    }

    // Compile Pixel Shader (separate blob!)
    ID3DBlob* psBlob = nullptr;

    hr = D3DCompile(
        psSource, strlen(psSource),
        nullptr, nullptr, nullptr,
        "main", "ps_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &psBlob, &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        vsBlob->Release();
        return false;
    }

    hr = m_device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &m_pixelShader
    );

    psBlob->Release();

    if (FAILED(hr)) {
        vsBlob->Release();
        return false;
    }

    // Create Input Layout (matches RenderVertex structure)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 3, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32_UINT, 0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 80, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = m_device->CreateInputLayout(
        layout, 8,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &m_inputLayout
    );

    // ERROR CHECK ADDED!
    if (FAILED(hr)) {
        vsBlob->Release();
        return false;
    }

    vsBlob->Release();

    // Create Constant Buffer
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = sizeof(ConstantBuffer);
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = m_device->CreateBuffer(&cbd, nullptr, &m_constantBuffer);

    return SUCCEEDED(hr);
}

bool D3D11Renderer::CreateStates() {
    D3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_BACK;
    rastDesc.FrontCounterClockwise = FALSE;
    rastDesc.DepthClipEnable = TRUE;
    rastDesc.MultisampleEnable = FALSE;  // Changed to match sample count

    HRESULT hr = m_device->CreateRasterizerState(&rastDesc, &m_rasterizerStateSolid);
    if (FAILED(hr)) return false;

    rastDesc.FillMode = D3D11_FILL_WIREFRAME;
    hr = m_device->CreateRasterizerState(&rastDesc, &m_rasterizerStateWireframe);
    if (FAILED(hr)) return false;

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_device->CreateBlendState(&blendDesc, &m_blendStateOpaque);
    if (FAILED(hr)) return false;

    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

    hr = m_device->CreateBlendState(&blendDesc, &m_blendStateAlpha);
    if (FAILED(hr)) return false;

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

    hr = m_device->CreateDepthStencilState(&dsDesc, &m_depthStateReadWrite);
    if (FAILED(hr)) return false;

    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    hr = m_device->CreateDepthStencilState(&dsDesc, &m_depthStateReadOnly);

    return SUCCEEDED(hr);
}

bool D3D11Renderer::LoadModel(const C3Model& model) {
    CleanupMeshBuffers();

    const auto& meshes = model.GetMeshes();
    if (meshes.empty()) return false;

    XMFLOAT3 center = model.GetCenter();
    float radius = model.GetRadius();
    float scale = (radius > 0) ? (2.0f / radius) : 1.0f;

    for (const auto& mesh : meshes) {
        if (mesh.vertices.empty()) continue;

        std::vector<RenderVertex> vertices;
        vertices.reserve(mesh.vertices.size());

        for (const auto& v : mesh.vertices) {
            RenderVertex rv;

            // Set all 4 morph target positions (FIXED!)
            rv.pos0 = XMFLOAT3(
                (v.positions[0].x - center.x) * scale,
                (v.positions[0].y - center.y) * scale,
                (v.positions[0].z - center.z) * scale
            );
            rv.pos1 = XMFLOAT3(
                (v.positions[1].x - center.x) * scale,
                (v.positions[1].y - center.y) * scale,
                (v.positions[1].z - center.z) * scale
            );
            rv.pos2 = XMFLOAT3(
                (v.positions[2].x - center.x) * scale,
                (v.positions[2].y - center.y) * scale,
                (v.positions[2].z - center.z) * scale
            );
            rv.pos3 = XMFLOAT3(
                (v.positions[3].x - center.x) * scale,
                (v.positions[3].y - center.y) * scale,
                (v.positions[3].z - center.z) * scale
            );

            rv.texCoord = XMFLOAT2(v.u, v.v);

            // Convert ARGB to float4
            uint8_t a = (v.color >> 24) & 0xFF;
            uint8_t r = (v.color >> 16) & 0xFF;
            uint8_t g = (v.color >> 8) & 0xFF;
            uint8_t b = v.color & 0xFF;
            rv.color = XMFLOAT4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);

            rv.boneIndices = XMUINT2(v.boneIndices[0], v.boneIndices[1]);
            rv.boneWeights = XMFLOAT2(v.boneWeights[0], v.boneWeights[1]);

            vertices.push_back(rv);
        }

        MeshBuffer mb = {};

        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = static_cast<UINT>(sizeof(RenderVertex) * vertices.size());
        vbd.Usage = D3D11_USAGE_DEFAULT;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vData = {};
        vData.pSysMem = vertices.data();

        if (SUCCEEDED(m_device->CreateBuffer(&vbd, &vData, &mb.vertexBuffer))) {
            if (!mesh.normalIndices.empty()) {
                D3D11_BUFFER_DESC ibd = {};
                ibd.ByteWidth = static_cast<UINT>(sizeof(uint16_t) * mesh.normalIndices.size());
                ibd.Usage = D3D11_USAGE_DEFAULT;
                ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

                D3D11_SUBRESOURCE_DATA iData = {};
                iData.pSysMem = mesh.normalIndices.data();

                if (SUCCEEDED(m_device->CreateBuffer(&ibd, &iData, &mb.normalIndexBuffer))) {
                    mb.normalIndexCount = static_cast<uint32_t>(mesh.normalIndices.size());
                }
            }

            if (!mesh.alphaIndices.empty()) {
                D3D11_BUFFER_DESC ibd = {};
                ibd.ByteWidth = static_cast<UINT>(sizeof(uint16_t) * mesh.alphaIndices.size());
                ibd.Usage = D3D11_USAGE_DEFAULT;
                ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

                D3D11_SUBRESOURCE_DATA iData = {};
                iData.pSysMem = mesh.alphaIndices.data();

                if (SUCCEEDED(m_device->CreateBuffer(&ibd, &iData, &mb.alphaIndexBuffer))) {
                    mb.alphaIndexCount = static_cast<uint32_t>(mesh.alphaIndices.size());
                }
            }

            m_meshBuffers.push_back(mb);
        }
    }

    return !m_meshBuffers.empty();
}
/*
void D3D11Renderer::Render(Camera& camera) {
    // Clear to RED so we know this function is called
    float clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };  // BRIGHT RED
    m_context->ClearRenderTargetView(m_renderTargetView, clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Print to output window
    OutputDebugStringA("=== RENDER CALLED ===\n");

    // STOP HERE - don't draw anything yet
    // Just clear and return
}
*/
void D3D11Renderer::Render(Camera& camera) {
    // DEBUG: Print once to verify this is being called
    static bool printed = false;
    if (!printed) {
        char buf[256];
        sprintf_s(buf, "Render called! Mesh buffers: %zu\n", m_meshBuffers.size());
        OutputDebugStringA(buf);
        printed = true;
    }

    float clearColor[4] = { 0.1f, 0.1f, 0.15f, 1.0f };
    m_context->ClearRenderTargetView(m_renderTargetView, clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_time += 0.016f;

    ConstantBuffer cb;
    cb.world = XMMatrixTranspose(XMMatrixIdentity());
    cb.view = XMMatrixTranspose(camera.GetViewMatrix());
    cb.projection = XMMatrixTranspose(camera.GetProjectionMatrix());
    cb.morphWeights = XMFLOAT4(m_morphWeights[0], m_morphWeights[1], m_morphWeights[2], m_morphWeights[3]);
    cb.lightDir = XMFLOAT4(0.2f, -0.5f, 1.0f, 0.0f);
    cb.lightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    XMFLOAT3 camPos = camera.GetPosition();
    cb.cameraPos = XMFLOAT4(camPos.x, camPos.y, camPos.z, 0.0f);
    cb.time = m_time;

    m_context->UpdateSubresource(m_constantBuffer, 0, nullptr, &cb, 0, 0);

    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    m_context->PSSetConstantBuffers(0, 1, &m_constantBuffer);

    m_context->RSSetState(m_wireframe ? m_rasterizerStateWireframe : m_rasterizerStateSolid);

    UINT stride = sizeof(RenderVertex);
    UINT offset = 0;

    // Pass 1: Opaque
    m_context->OMSetBlendState(m_blendStateOpaque, nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_depthStateReadWrite, 0);

    for (const auto& mb : m_meshBuffers) {
        if (mb.normalIndexCount > 0) {
            m_context->IASetVertexBuffers(0, 1, &mb.vertexBuffer, &stride, &offset);
            m_context->IASetIndexBuffer(mb.normalIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_context->DrawIndexed(mb.normalIndexCount, 0, 0);
        }
    }

    // Pass 2: Transparent
    m_context->OMSetBlendState(m_blendStateAlpha, nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_depthStateReadOnly, 0);

    for (const auto& mb : m_meshBuffers) {
        if (mb.alphaIndexCount > 0) {
            m_context->IASetVertexBuffers(0, 1, &mb.vertexBuffer, &stride, &offset);
            m_context->IASetIndexBuffer(mb.alphaIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_context->DrawIndexed(mb.alphaIndexCount, 0, 0);
        }
    }
}

