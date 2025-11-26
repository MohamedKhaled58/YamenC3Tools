#pragma once
#include "C3Types.h"
#include <memory>

class C3Model {
public:
    struct MeshPart {
        std::string name;
        std::vector<PhyVertex> vertices;
        std::vector<uint16_t> normalIndices;
        std::vector<uint16_t> alphaIndices;
        std::string textureName;
        XMFLOAT3 bboxMin{}, bboxMax{};
        XMFLOAT4X4 initialMatrix{};
        uint32_t textureRow = 0;
        uint32_t blendCount = 0;
        std::vector<C3KeyFrame> alphaKeyframes;
        std::vector<C3KeyFrame> drawKeyframes;
    };

    struct ShapeData {
        std::string name;
        struct Line {
            std::vector<XMFLOAT3> points;
        };
        std::vector<Line> lines;
        std::string textureName;
        uint32_t segmentCount = 0;
    };

    struct ParticleSystem {
        std::string name;
        XMFLOAT3 emitterPos{};
        float emitRate = 10.0f;
        float lifetime = 5.0f;
        float speed = 1.0f;
        XMFLOAT3 size{ 1,1,1 };
        XMFLOAT4 startColor{ 1,1,1,1 };
        XMFLOAT4 endColor{ 1,1,1,0 };
        std::string textureName;
        uint32_t maxParticles = 1000;
    };

    C3Model() = default;
    ~C3Model() = default;

    bool LoadFromFile(const std::string& path);
    bool LoadFromMemory(const std::vector<uint8_t>& data);

    C3ChunkType GetType() const { return m_type; }
    const std::vector<MeshPart>& GetMeshes() const { return m_meshes; }
    const std::vector<ShapeData>& GetShapes() const { return m_shapes; }
    const std::vector<ParticleSystem>& GetParticles() const { return m_particles; }
    const std::string& GetError() const { return m_error; }

    XMFLOAT3 GetCenter() const { return m_center; }
    float GetRadius() const { return m_radius; }

private:
    C3ChunkType m_type = C3ChunkType::Unknown;
    std::vector<MeshPart> m_meshes;
    std::vector<ShapeData> m_shapes;
    std::vector<ParticleSystem> m_particles;
    std::string m_error;
    XMFLOAT3 m_center{};
    float m_radius = 1.0f;

    bool ParsePHY(const uint8_t* data, size_t offset, size_t chunkSize);
    bool ParseSMOT(const uint8_t* data, size_t offset, size_t chunkSize);
    bool ParsePTCL(const uint8_t* data, size_t offset, size_t chunkSize);
    void CalculateBounds();
};