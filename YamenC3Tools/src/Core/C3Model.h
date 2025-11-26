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

    struct Bone {
        XMFLOAT4X4 bindMatrix;
        XMFLOAT4X4 invBindMatrix;
        std::string name;
        int parentIndex = -1;
    };

    struct Animation {
        std::string name;
        uint32_t boneCount = 0;
        uint32_t frameCount = 0;
        uint32_t keyFrameCount = 0;
        struct KeyFrame {
            uint32_t frame;
            std::vector<XMFLOAT4X4> boneMatrices; // Per-bone transform matrices
        };
        std::vector<KeyFrame> keyFrames;
        std::vector<float> morphWeights; // Morph target weights per frame
        uint32_t morphCount = 0;
    };

    C3Model() = default;
    ~C3Model() = default;

    bool LoadFromFile(const std::string& path);
    bool LoadFromMemory(const std::vector<uint8_t>& data);
    bool MergeFromFile(const std::string& path); // Merge additional C3 file data
    bool MergeFromMemory(const std::vector<uint8_t>& data);

    C3ChunkType GetType() const { return m_type; }
    const std::vector<MeshPart>& GetMeshes() const { return m_meshes; }
    std::vector<MeshPart>& GetMeshes() { return m_meshes; }
    const std::vector<ShapeData>& GetShapes() const { return m_shapes; }
    std::vector<ShapeData>& GetShapes() { return m_shapes; }
    const std::vector<ParticleSystem>& GetParticles() const { return m_particles; }
    std::vector<ParticleSystem>& GetParticles() { return m_particles; }
    const std::vector<Bone>& GetBones() const { return m_bones; }
    std::vector<Bone>& GetBones() { return m_bones; }
    const std::vector<Animation>& GetAnimations() const { return m_animations; }
    std::vector<Animation>& GetAnimations() { return m_animations; }
    const std::string& GetError() const { return m_error; }

    XMFLOAT3 GetCenter() const { return m_center; }
    float GetRadius() const { return m_radius; }
    
    // Animation helpers
    void SetAnimationFrame(uint32_t animIndex, uint32_t frame);
    void GetBoneMatrix(uint32_t boneIndex, uint32_t animIndex, uint32_t frame, XMFLOAT4X4& outMatrix);

private:
    C3ChunkType m_type = C3ChunkType::Unknown;
    std::vector<MeshPart> m_meshes;
    std::vector<ShapeData> m_shapes;
    std::vector<ParticleSystem> m_particles;
    std::vector<Bone> m_bones;
    std::vector<Animation> m_animations;
    std::string m_error;
    XMFLOAT3 m_center{};
    float m_radius = 1.0f;
    
    uint32_t m_currentAnimIndex = 0;
    uint32_t m_currentFrame = 0;

    bool ParsePHY(const uint8_t* data, size_t offset, size_t chunkSize);
    bool ParseSMOT(const uint8_t* data, size_t offset, size_t chunkSize);
    bool ParsePTCL(const uint8_t* data, size_t offset, size_t chunkSize);
    bool ParseMOTI(const uint8_t* data, size_t offset, size_t chunkSize);
    bool ParsePHYS(const uint8_t* data, size_t offset, size_t chunkSize); // Physics chunk with bones
    void CalculateBounds();
    void InterpolateKeyFrames(const Animation& anim, uint32_t frame, std::vector<XMFLOAT4X4>& outMatrices);
};