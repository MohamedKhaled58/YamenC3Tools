#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include "../Core/C3Model.h"
#include "Camera.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

class D3D11Renderer {
public:
    struct ConstantBuffer {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMFLOAT4 morphWeights;
        XMFLOAT4 lightDir;
        XMFLOAT4 lightColor;
        XMFLOAT4 cameraPos;
        float time;
        float padding[3];
    };

    struct RenderVertex {
        XMFLOAT3 pos0, pos1, pos2, pos3;
        XMFLOAT2 texCoord;
        XMFLOAT4 color;
        XMUINT2 boneIndices;
        XMFLOAT2 boneWeights;
    };

    D3D11Renderer();
    ~D3D11Renderer();

    bool Initialize(HWND hwnd, int width, int height);
    void Shutdown();
    void Resize(int width, int height);

    bool LoadModel(const C3Model& model);
    void Render(Camera& camera);

    void SetMorphWeights(float w0, float w1, float w2, float w3);
    void SetWireframe(bool enabled);
    void SetShowNormals(bool enabled);

public:
    // Add these getters
    ID3D11DeviceContext* GetContext() const { return m_context; }
    ID3D11RenderTargetView* GetRenderTargetView() const { return m_renderTargetView; }
    IDXGISwapChain* GetSwapChain() const { return m_swapChain; }

    ID3D11Device* GetDevice() { return m_device; }
    ID3D11DeviceContext* GetContext() { return m_context; }

private:
    HWND m_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
    ID3D11DepthStencilView* m_depthStencilView = nullptr;
    ID3D11Texture2D* m_depthStencilBuffer = nullptr;

    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;

    ID3D11RasterizerState* m_rasterizerStateSolid = nullptr;
    ID3D11RasterizerState* m_rasterizerStateWireframe = nullptr;
    ID3D11BlendState* m_blendStateOpaque = nullptr;
    ID3D11BlendState* m_blendStateAlpha = nullptr;
    ID3D11DepthStencilState* m_depthStateReadWrite = nullptr;
    ID3D11DepthStencilState* m_depthStateReadOnly = nullptr;

    struct MeshBuffer {
        ID3D11Buffer* vertexBuffer = nullptr;
        ID3D11Buffer* normalIndexBuffer = nullptr;
        ID3D11Buffer* alphaIndexBuffer = nullptr;
        uint32_t normalIndexCount = 0;
        uint32_t alphaIndexCount = 0;
    };
    std::vector<MeshBuffer> m_meshBuffers;

    float m_morphWeights[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    float m_time = 0.0f;
    bool m_wireframe = false;

    bool CreateDeviceAndSwapChain();
    bool CreateRenderTargets();
    bool CreateDepthStencil();
    bool CompileShaders();
    bool CreateStates();
    void CleanupMeshBuffers();
};