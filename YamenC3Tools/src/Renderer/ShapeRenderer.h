#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "../Core/C3Model.h"

using namespace DirectX;

class ShapeRenderer {
public:
    bool Initialize(ID3D11Device* device);
    void Shutdown();
    void Render(ID3D11DeviceContext* context, const C3Model::ShapeData& shape,
        const XMMATRIX& view, const XMMATRIX& projection);

private:
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;
    ID3D11BlendState* m_blendState = nullptr;

    struct ShapeVertex {
        XMFLOAT3 pos;
        XMFLOAT2 texCoord;
        float alpha;
    };

    struct ShapeCB {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMFLOAT4 trailColor;
        float time;
        float padding[3];
    };
};