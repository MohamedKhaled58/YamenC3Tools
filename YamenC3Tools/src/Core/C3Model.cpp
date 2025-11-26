#include "C3Model.h"
#include <fstream>
#include <algorithm>
#include <cstring>

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

    C3FileHeader header;
    memcpy(&header, data.data(), sizeof(C3FileHeader));

    if (strncmp(header.magic, "MAXFILE C3", 10) != 0) {
        m_error = "Invalid C3 file header";
        return false;
    }

    // Detect chunk type
    std::string typeStr(header.physicsType, 4);
    if (typeStr == "PHY ") m_type = C3ChunkType::PHY;
    else if (typeStr == "PHY3") m_type = C3ChunkType::PHY3;
    else if (typeStr == "PHY4") m_type = C3ChunkType::PHY4;
    else if (typeStr == "SMOT") m_type = C3ChunkType::SMOT;
    else if (typeStr == "SHAP") m_type = C3ChunkType::SHAP;
    else if (typeStr == "PTCL") m_type = C3ChunkType::PTCL;
    else {
        m_error = "Unknown chunk type: " + typeStr;
        return false;
    }

    // Read chunk size (at offset 20)
    if (data.size() < 24) {
        m_error = "No chunk size found";
        return false;
    }

    uint32_t chunkSize;
    memcpy(&chunkSize, data.data() + 20, 4);

    size_t offset = 24; // After header + size

    // Parse based on type
    bool success = false;
    switch (m_type) {
    case C3ChunkType::PHY:
    case C3ChunkType::PHY3:
    case C3ChunkType::PHY4:
        success = ParsePHY(data.data(), offset, chunkSize);
        break;
    case C3ChunkType::SMOT:
    case C3ChunkType::SHAP:
        success = ParseSMOT(data.data(), offset, chunkSize);
        break;
    case C3ChunkType::PTCL:
        success = ParsePTCL(data.data(), offset, chunkSize);
        break;
    default:
        m_error = "Parser not implemented for this type";
        return false;
    }

    if (success) {
        CalculateBounds();
    }

    return success;
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
    if (nameLen > 0 && nameLen < 256 && offset + 4 + nameLen <= chunkEnd) {
        offset += 4;
        part.name = std::string(reinterpret_cast<const char*>(data + offset), nameLen);
        offset += nameLen;
    }
    else {
        part.name = "mesh";
    }

    // Read blend count
    if (offset + 4 > chunkEnd) return false;
    part.blendCount = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    // Read vertex counts
    if (offset + 8 > chunkEnd) return false;
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
    if (offset + 8 > chunkEnd) return false;
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