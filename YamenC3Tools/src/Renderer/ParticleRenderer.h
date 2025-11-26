#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "../Core/C3Model.h"

using namespace DirectX;

class ParticleRenderer {
public:
    bool Initialize(ID3D11Device* device);
    void Shutdown();
    void Update(float deltaTime);
    void Render(ID3D11DeviceContext* context, const C3Model::ParticleSystem& ps,
        const XMMATRIX& view, const XMMATRIX& projection, const XMFLOAT3& cameraPos);

private:
    struct Particle {
        XMFLOAT3 position;
        XMFLOAT3 velocity;
        XMFLOAT4 color;
        float size;
        float lifetime;
        float age;
        bool active;
    };

    struct ParticleVertex {
        XMFLOAT3 pos;
        XMFLOAT2 texCoord;
        XMFLOAT4 color;
        float size;
    };

    struct ParticleCB {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMFLOAT4 cameraRight;
        XMFLOAT4 cameraUp;
        float time;
        float padding[3];
    };

    std::vector<Particle> m_particles;
    ID3D11Device* m_device = nullptr;
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11BlendState* m_blendState = nullptr;
    size_t m_lastVertexCount = 0;

    void EmitParticle(const C3Model::ParticleSystem& ps);
};