#pragma once
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <cstdint>

using namespace DirectX;

enum class C3ChunkType {
    Unknown,
    PHY, PHY3, PHY4,
    SMOT, SHAP,
    PTCL,
    SCNE,
    ANIM,
    SKEL
};

#pragma pack(push, 1)
struct PhyVertex {
    XMFLOAT3 positions[4];
    float u, v;
    uint32_t color;
    uint32_t boneIndices[2];
    float boneWeights[2];
};

struct C3FileHeader {
    char magic[16];
    char physicsType[4];
};
#pragma pack(pop)

static_assert(sizeof(PhyVertex) == 76, "PhyVertex must be 76 bytes");

struct C3KeyFrame {
    uint32_t frame;
    float value;
};

inline const char* ChunkTypeToString(C3ChunkType type) {
    switch (type) {
    case C3ChunkType::PHY: return "PHY";
    case C3ChunkType::PHY3: return "PHY3";
    case C3ChunkType::PHY4: return "PHY4";
    case C3ChunkType::SMOT: return "SMOT";
    case C3ChunkType::SHAP: return "SHAP";
    case C3ChunkType::PTCL: return "PTCL";
    case C3ChunkType::SCNE: return "SCNE";
    case C3ChunkType::ANIM: return "ANIM";
    case C3ChunkType::SKEL: return "SKEL";
    default: return "Unknown";
    }
}