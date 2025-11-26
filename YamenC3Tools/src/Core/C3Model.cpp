#include "C3Model.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <cstdio>
using namespace DirectX;

bool C3Model::LoadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        m_error = "Failed to open file: " + path;
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();

    return LoadFromMemory(data);
}

bool C3Model::LoadFromMemory(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(C3FileHeader)) {
        m_error = "File too small";
        return false;
    }

    size_t offset = 0;

    // Read C3 header (exactly like the reference code)
    C3FileHeader header;
    memcpy(&header, data.data(), sizeof(C3FileHeader));
    offset += sizeof(C3FileHeader);

    if (strncmp(header.magic, "MAXFILE C3", 10) != 0) {
        m_error = "Invalid C3 magic header";
        return false;
    }

    // Get chunk type from header (NOT from a chunk ID!)
    std::string chunkType(header.physicsType, 4);

    // Read chunk size (at offset 20, NO chunk ID!)
    if (offset + 4 > data.size()) {
        m_error = "No chunk size";
        return false;
    }

    uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(data.data() + offset);
    offset += 4;

    if (chunkSize == 0 || offset + chunkSize > data.size()) {
        m_error = "Invalid chunk size: " + std::to_string(chunkSize);
        return false;
    }

    // Parse PHY chunks only (like reference code)
    if (chunkType == "PHY4" || chunkType == "PHY " || chunkType == "PHY3") {
        if (!ParsePHYS(data.data(), offset, chunkSize)) {
            return false;
        }
        m_type = (chunkType == "PHY3") ? C3ChunkType::PHY3 : 
                 (chunkType == "PHY4") ? C3ChunkType::PHY4 : C3ChunkType::PHY;
    }
    else if (chunkType == "MOTI") {
        if (!ParseMOTI(data.data(), offset, chunkSize)) {
            return false;
        }
    }
    else if (chunkType == "SMOT" || chunkType == "SHAP") {
        if (!ParseSMOT(data.data(), offset, chunkSize)) {
            return false;
        }
        m_type = C3ChunkType::SHAP;
    }
    else if (chunkType == "PTCL") {
        if (!ParsePTCL(data.data(), offset, chunkSize)) {
            return false;
        }
        m_type = C3ChunkType::PTCL;
    }
    else {
        m_error = "Not a supported file type (type: " + chunkType + ")";
        return false;
    }

    if (m_meshes.empty() && m_shapes.empty() && m_particles.empty()) {
        m_error = "No data loaded from file";
        return false;
    }

    CalculateBounds();
    return true;
}

bool C3Model::ParsePHY(const uint8_t* data, size_t offset, size_t chunkSize) {
    size_t chunkEnd = offset + chunkSize;
    MeshPart part;

    // Read name (optional, variable length)
    if (offset + 4 > chunkEnd) {
        m_error = "Unexpected end of chunk";
        return false;
    }

    uint32_t nameLen = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4; // Always advance past nameLen field
    
    if (nameLen > 0 && nameLen < 256 && offset + nameLen <= chunkEnd) {
        part.name = std::string(reinterpret_cast<const char*>(data + offset), nameLen);
        offset += nameLen;
    }
    else {
        part.name = "mesh";
    }

    // Read blend count
    if (offset + 4 > chunkEnd) {
        m_error = "Unexpected end of chunk while reading blend count";
        return false;
    }
    part.blendCount = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    // Read vertex counts
    if (offset + 8 > chunkEnd) {
        m_error = "Unexpected end of chunk while reading vertex counts";
        return false;
    }
    uint32_t normalVertCount = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;
    uint32_t alphaVertCount = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    uint32_t totalVerts = normalVertCount + alphaVertCount;
    if (totalVerts == 0 || totalVerts > 100000) {
        m_error = "Invalid vertex count: " + std::to_string(totalVerts);
        return false;
    }

    // Check if we have 76-byte or 40-byte format
    size_t remainingBytes = chunkEnd - offset;
    size_t required76 = totalVerts * 76;
    size_t required40 = totalVerts * 40;

    part.vertices.resize(totalVerts);

    if (remainingBytes >= required76) {
        // 76-byte format (morph targets)
        memcpy(part.vertices.data(), data + offset, required76);
        offset += required76;
    }
    else if (remainingBytes >= required40) {
        // 40-byte compact format - convert to 76
        struct CompactVertex {
            XMFLOAT3 pos;
            XMFLOAT3 normal;
            float u, v;
            uint32_t boneIdx;
            uint32_t color;
        };

        for (uint32_t i = 0; i < totalVerts; i++) {
            CompactVertex cv;
            memcpy(&cv, data + offset + i * sizeof(CompactVertex), sizeof(CompactVertex));

            PhyVertex& v = part.vertices[i];
            v.positions[0] = v.positions[1] = v.positions[2] = v.positions[3] = cv.pos;
            v.u = cv.u;
            v.v = cv.v;
            v.color = cv.color;
            v.boneIndices[0] = cv.boneIdx & 0xFF;
            v.boneIndices[1] = (cv.boneIdx >> 8) & 0xFF;
            v.boneWeights[0] = 1.0f;
            v.boneWeights[1] = 0.0f;
        }
        offset += required40;
    }
    else {
        m_error = "Not enough data for vertices";
        return false;
    }

    // Read triangle counts
    if (offset + 8 > chunkEnd) {
        m_error = "Unexpected end of chunk while reading triangle counts";
        return false;
    }
    uint32_t normalTriCount = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;
    uint32_t alphaTriCount = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    // Read indices
    if (normalTriCount > 0) {
        size_t indexSize = normalTriCount * 3 * sizeof(uint16_t);
        if (offset + indexSize > chunkEnd) {
            m_error = "Not enough data for normal indices";
            return false;
        }
        part.normalIndices.resize(normalTriCount * 3);
        memcpy(part.normalIndices.data(), data + offset, indexSize);
        offset += indexSize;
    }

    if (alphaTriCount > 0) {
        size_t indexSize = alphaTriCount * 3 * sizeof(uint16_t);
        if (offset + indexSize > chunkEnd) {
            m_error = "Not enough data for alpha indices";
            return false;
        }
        part.alphaIndices.resize(alphaTriCount * 3);
        memcpy(part.alphaIndices.data(), data + offset, indexSize);
        offset += indexSize;
    }

    // Read texture name
    if (offset + 4 <= chunkEnd) {
        uint32_t texLen = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += 4;
        if (texLen > 0 && texLen < 256 && offset + texLen <= chunkEnd) {
            part.textureName = std::string(reinterpret_cast<const char*>(data + offset), texLen);
            offset += texLen;
        }
    }

    // Read bounding box
    if (offset + sizeof(XMFLOAT3) * 2 <= chunkEnd) {
        memcpy(&part.bboxMin, data + offset, sizeof(XMFLOAT3));
        offset += sizeof(XMFLOAT3);
        memcpy(&part.bboxMax, data + offset, sizeof(XMFLOAT3));
        offset += sizeof(XMFLOAT3);
    }

    // Read initial matrix
    if (offset + sizeof(XMFLOAT4X4) <= chunkEnd) {
        memcpy(&part.initialMatrix, data + offset, sizeof(XMFLOAT4X4));
        offset += sizeof(XMFLOAT4X4);
    }

    // Read texture row
    if (offset + 4 <= chunkEnd) {
        part.textureRow = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += 4;
    }

    m_meshes.push_back(std::move(part));
    return true;
}

bool C3Model::ParseSMOT(const uint8_t* data, size_t offset, size_t chunkSize) {
    size_t chunkEnd = offset + chunkSize;
    ShapeData shape;

    // Read name
    if (offset + 4 > chunkEnd) return false;
    uint32_t nameLen = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    if (nameLen > 0 && nameLen < 256 && offset + nameLen <= chunkEnd) {
        shape.name = std::string(reinterpret_cast<const char*>(data + offset), nameLen);
        offset += nameLen;
    }

    // Read line count
    if (offset + 4 > chunkEnd) return false;
    uint32_t lineCount = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    // Read lines
    for (uint32_t i = 0; i < lineCount && i < 100; i++) {
        if (offset + 4 > chunkEnd) break;

        uint32_t pointCount = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += 4;

        if (pointCount > 0 && pointCount < 1000) {
            ShapeData::Line line;
            size_t pointSize = pointCount * sizeof(XMFLOAT3);

            if (offset + pointSize <= chunkEnd) {
                line.points.resize(pointCount);
                memcpy(line.points.data(), data + offset, pointSize);
                offset += pointSize;
                shape.lines.push_back(std::move(line));
            }
        }
    }

    // Read texture
    if (offset + 4 <= chunkEnd) {
        uint32_t texLen = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += 4;
        if (texLen > 0 && texLen < 256 && offset + texLen <= chunkEnd) {
            shape.textureName = std::string(reinterpret_cast<const char*>(data + offset), texLen);
            offset += texLen;
        }
    }

    // Read segment count
    if (offset + 4 <= chunkEnd) {
        shape.segmentCount = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += 4;
    }

    m_shapes.push_back(std::move(shape));
    return true;
}

bool C3Model::ParsePTCL(const uint8_t* data, size_t offset, size_t chunkSize) {
    size_t chunkEnd = offset + chunkSize;
    ParticleSystem ps;

    // Read name
    if (offset + 4 > chunkEnd) return false;
    uint32_t nameLen = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    if (nameLen > 0 && nameLen < 256 && offset + nameLen <= chunkEnd) {
        ps.name = std::string(reinterpret_cast<const char*>(data + offset), nameLen);
        offset += nameLen;
    }

    // Read emitter position
    if (offset + sizeof(XMFLOAT3) > chunkEnd) return false;
    memcpy(&ps.emitterPos, data + offset, sizeof(XMFLOAT3));
    offset += sizeof(XMFLOAT3);

    // Read properties
    if (offset + 12 <= chunkEnd) {
        ps.emitRate = *reinterpret_cast<const float*>(data + offset); offset += 4;
        ps.lifetime = *reinterpret_cast<const float*>(data + offset); offset += 4;
        ps.speed = *reinterpret_cast<const float*>(data + offset); offset += 4;
    }

    // Read size
    if (offset + sizeof(XMFLOAT3) <= chunkEnd) {
        memcpy(&ps.size, data + offset, sizeof(XMFLOAT3));
        offset += sizeof(XMFLOAT3);
    }

    // Read colors
    if (offset + sizeof(XMFLOAT4) * 2 <= chunkEnd) {
        memcpy(&ps.startColor, data + offset, sizeof(XMFLOAT4));
        offset += sizeof(XMFLOAT4);
        memcpy(&ps.endColor, data + offset, sizeof(XMFLOAT4));
        offset += sizeof(XMFLOAT4);
    }

    // Read texture
    if (offset + 4 <= chunkEnd) {
        uint32_t texLen = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += 4;
        if (texLen > 0 && texLen < 256 && offset + texLen <= chunkEnd) {
            ps.textureName = std::string(reinterpret_cast<const char*>(data + offset), texLen);
            offset += texLen;
        }
    }

    // Read max particles
    if (offset + 4 <= chunkEnd) {
        ps.maxParticles = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += 4;
    }

    m_particles.push_back(std::move(ps));
    return true;
}

bool C3Model::ParseMOTI(const uint8_t* data, size_t offset, size_t chunkSize) {
    size_t chunkEnd = offset + chunkSize;
    Animation anim;
    anim.name = "motion";
    
    // Read bone count
    if (offset + 4 > chunkEnd) return false;
    anim.boneCount = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;
    
    // Read frame count
    if (offset + 4 > chunkEnd) return false;
    anim.frameCount = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;
    
    // Check for keyframe format
    if (offset + 4 > chunkEnd) return false;
    char kf[4];
    memcpy(kf, data + offset, 4);
    offset += 4;
    
    bool isKKEY = (kf[0] == 'K' && kf[1] == 'K' && kf[2] == 'E' && kf[3] == 'Y');
    bool isXKEY = (kf[0] == 'X' && kf[1] == 'K' && kf[2] == 'E' && kf[3] == 'Y');
    bool isZKEY = (kf[0] == 'Z' && kf[1] == 'K' && kf[2] == 'E' && kf[3] == 'Y');
    
    if (isKKEY || isXKEY || isZKEY) {
        // Read keyframe count
        if (offset + 4 > chunkEnd) return false;
        anim.keyFrameCount = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += 4;
        
        anim.keyFrames.resize(anim.keyFrameCount);
        
        for (uint32_t k = 0; k < anim.keyFrameCount; k++) {
            Animation::KeyFrame& kf = anim.keyFrames[k];
            kf.boneMatrices.resize(anim.boneCount);
            
            if (isKKEY) {
                // Full 4x4 matrices (64 bytes per bone)
                if (offset + 4 > chunkEnd) return false;
                kf.frame = *reinterpret_cast<const uint32_t*>(data + offset);
                offset += 4;
                
                for (uint32_t b = 0; b < anim.boneCount; b++) {
                    if (offset + 64 > chunkEnd) return false;
                    memcpy(&kf.boneMatrices[b], data + offset, 64);
                    offset += 64;
                }
            }
            else if (isXKEY) {
                // Compressed 3x4 matrices (48 bytes per bone)
                if (offset + 2 > chunkEnd) return false;
                uint16_t wPos;
                memcpy(&wPos, data + offset, 2);
                kf.frame = wPos;
                offset += 2;
                
                for (uint32_t b = 0; b < anim.boneCount; b++) {
                    if (offset + 48 > chunkEnd) return false;
                    // Read TIDY_MATRIX (3x4)
                    float m[12];
                    memcpy(m, data + offset, 48);
                    offset += 48;
                    
                    // Convert to 4x4
                    XMFLOAT4X4& mat = kf.boneMatrices[b];
                    mat._11 = m[0]; mat._12 = m[1]; mat._13 = m[2]; mat._14 = 0;
                    mat._21 = m[3]; mat._22 = m[4]; mat._23 = m[5]; mat._24 = 0;
                    mat._31 = m[6]; mat._32 = m[7]; mat._33 = m[8]; mat._34 = 0;
                    mat._41 = m[9]; mat._42 = m[10]; mat._43 = m[11]; mat._44 = 1;
                }
            }
            else if (isZKEY) {
                // Quaternion + translation (DIV_INFO format)
                if (offset + 2 > chunkEnd) return false;
                uint16_t wPos;
                memcpy(&wPos, data + offset, 2);
                kf.frame = wPos;
                offset += 2;
                
                for (uint32_t b = 0; b < anim.boneCount; b++) {
                    if (offset + 28 > chunkEnd) return false; // quaternion(16) + xyz(12)
                    
                    XMFLOAT4 quat;
                    XMFLOAT3 trans;
                    memcpy(&quat, data + offset, 16);
                    offset += 16;
                    memcpy(&trans, data + offset, 12);
                    offset += 12;
                    
                    // Build matrix from quaternion + translation
                    XMMATRIX rot = XMMatrixRotationQuaternion(XMLoadFloat4(&quat));
                    XMMATRIX transMat = XMMatrixTranslation(trans.x, trans.y, trans.z);
                    XMMATRIX combined = rot * transMat;
                    XMStoreFloat4x4(&kf.boneMatrices[b], combined);
                }
            }
        }
    } else {
        // No keyframes - all frames are keyframes
        offset -= 4; // Rewind
        anim.keyFrameCount = anim.frameCount;
        anim.keyFrames.resize(anim.keyFrameCount);
        
        for (uint32_t k = 0; k < anim.keyFrameCount; k++) {
            anim.keyFrames[k].frame = k;
            anim.keyFrames[k].boneMatrices.resize(anim.boneCount);
        }
        
        // Read matrices per bone (bone-major order)
        for (uint32_t b = 0; b < anim.boneCount; b++) {
            for (uint32_t f = 0; f < anim.frameCount; f++) {
                if (offset + 64 > chunkEnd) return false;
                memcpy(&anim.keyFrames[f].boneMatrices[b], data + offset, 64);
                offset += 64;
            }
        }
    }
    
    // Read morph weights
    if (offset + 4 <= chunkEnd) {
        anim.morphCount = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += 4;
        
        if (anim.morphCount > 0 && anim.frameCount > 0) {
            size_t morphSize = anim.morphCount * anim.frameCount * sizeof(float);
            if (offset + morphSize <= chunkEnd) {
                anim.morphWeights.resize(anim.morphCount * anim.frameCount);
                memcpy(anim.morphWeights.data(), data + offset, morphSize);
            }
        }
    }
    
    m_animations.push_back(std::move(anim));
    return true;
}

bool C3Model::ParsePHYS(const uint8_t* data, size_t offset, size_t chunkSize) {
    // Same as ParsePHY but can be part of multi-chunk file
    return ParsePHY(data, offset, chunkSize);
}

bool C3Model::MergeFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        m_error = "Failed to open file: " + path;
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();

    return MergeFromMemory(data);
}

bool C3Model::MergeFromMemory(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(C3FileHeader)) {
        m_error = "File too small";
        return false;
    }

    C3FileHeader header;
    memcpy(&header, data.data(), sizeof(C3FileHeader));

    if (strncmp(header.magic, "MAXFILE C3", 10) != 0) {
        m_error = "Invalid C3 file header";
        return false;
    }

    // Parse chunks and merge data
    size_t offset = sizeof(C3FileHeader);
    bool merged = false;
    
    while (offset < data.size() - 8) {
        char chunkID[4];
        uint32_t chunkSize;
        
        if (offset + 8 > data.size()) break;
        memcpy(chunkID, data.data() + offset, 4);
        memcpy(&chunkSize, data.data() + offset + 4, 4);
        offset += 8;
        
        std::string chunkStr(chunkID, 4);
        bool parsed = false;
        
        if (chunkStr == "PHYS" || chunkStr == "PHY " || chunkStr == "PHY3" || chunkStr == "PHY4") {
            parsed = ParsePHYS(data.data(), offset, chunkSize);
            merged = true;
        }
        else if (chunkStr == "MOTI") {
            parsed = ParseMOTI(data.data(), offset, chunkSize);
            merged = true;
        }
        else if (chunkStr == "SMOT" || chunkStr == "SHAP") {
            parsed = ParseSMOT(data.data(), offset, chunkSize);
            merged = true;
        }
        else if (chunkStr == "PTCL") {
            parsed = ParsePTCL(data.data(), offset, chunkSize);
            merged = true;
        }
        
        if (parsed) {
            offset += chunkSize;
        } else if (chunkSize > 0) {
            offset += chunkSize;
        } else {
            break;
        }
    }
    
    if (merged) {
        CalculateBounds();
    }
    
    return merged;
}

void C3Model::SetAnimationFrame(uint32_t animIndex, uint32_t frame) {
    if (animIndex < m_animations.size()) {
        m_currentAnimIndex = animIndex;
        m_currentFrame = frame % m_animations[animIndex].frameCount;
    }
}

void C3Model::GetBoneMatrix(uint32_t boneIndex, uint32_t animIndex, uint32_t frame, XMFLOAT4X4& outMatrix) {
    // Initialize to identity matrix
    XMStoreFloat4x4(&outMatrix, XMMatrixIdentity());
    
    if (animIndex >= m_animations.size()) return;
    if (boneIndex >= m_animations[animIndex].boneCount) return;
    
    const Animation& anim = m_animations[animIndex];
    frame = frame % anim.frameCount;
    
    // Find surrounding keyframes
    size_t startIdx = anim.keyFrames.size(), endIdx = anim.keyFrames.size();
    for (size_t i = 0; i < anim.keyFrames.size(); i++) {
        if (anim.keyFrames[i].frame <= frame) {
            if (startIdx == anim.keyFrames.size() || i > startIdx) startIdx = i;
        }
        if (anim.keyFrames[i].frame > frame) {
            if (endIdx == anim.keyFrames.size() || i < endIdx) endIdx = i;
        }
    }
    
    if (startIdx == anim.keyFrames.size() && endIdx != anim.keyFrames.size()) {
        outMatrix = anim.keyFrames[endIdx].boneMatrices[boneIndex];
    }
    else if (startIdx != anim.keyFrames.size() && endIdx == anim.keyFrames.size()) {
        outMatrix = anim.keyFrames[startIdx].boneMatrices[boneIndex];
    }
    else if (startIdx != anim.keyFrames.size() && endIdx != anim.keyFrames.size()) {
        // Interpolate matrices manually (XMMatrixLerp doesn't exist)
        const XMFLOAT4X4& m1 = anim.keyFrames[startIdx].boneMatrices[boneIndex];
        const XMFLOAT4X4& m2 = anim.keyFrames[endIdx].boneMatrices[boneIndex];
        uint32_t f1 = anim.keyFrames[startIdx].frame;
        uint32_t f2 = anim.keyFrames[endIdx].frame;
        
        float t = (f2 > f1) ? (float)(frame - f1) / (float)(f2 - f1) : 0.0f;
        
        // Manual matrix interpolation (component-wise lerp)
        XMFLOAT4X4 result;
        for (int i = 0; i < 16; i++) {
            float* r = reinterpret_cast<float*>(&result);
            const float* a = reinterpret_cast<const float*>(&m1);
            const float* b = reinterpret_cast<const float*>(&m2);
            r[i] = a[i] + (b[i] - a[i]) * t;
        }
        outMatrix = result;
    }
}

void C3Model::InterpolateKeyFrames(const Animation& anim, uint32_t frame, std::vector<XMFLOAT4X4>& outMatrices) {
    outMatrices.resize(anim.boneCount);
    for (uint32_t b = 0; b < anim.boneCount; b++) {
        GetBoneMatrix(b, 0, frame, outMatrices[b]);
    }
}

void C3Model::CalculateBounds() {
    if (m_meshes.empty()) return;

    XMFLOAT3 min{ FLT_MAX, FLT_MAX, FLT_MAX };
    XMFLOAT3 max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (const auto& mesh : m_meshes) {
        for (const auto& v : mesh.vertices) {
            const XMFLOAT3& pos = v.positions[0];
            min.x = std::min(min.x, pos.x);
            min.y = std::min(min.y, pos.y);
            min.z = std::min(min.z, pos.z);
            max.x = std::max(max.x, pos.x);
            max.y = std::max(max.y, pos.y);
            max.z = std::max(max.z, pos.z);
        }
    }

    m_center = XMFLOAT3{
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f
    };

    float dx = max.x - min.x;
    float dy = max.y - min.y;
    float dz = max.z - min.z;
    m_radius = sqrtf(dx * dx + dy * dy + dz * dz) * 0.5f;
}