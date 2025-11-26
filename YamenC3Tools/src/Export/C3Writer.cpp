#include "C3Writer.h"
#include "../Core/C3Types.h"
#include <fstream>
#include <cstring>

bool C3Writer::Write(const C3Model& model, const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        m_lastError = "Failed to create file: " + path;
        return false;
    }

    // Write C3 file header
    C3FileHeader header;
    memset(header.magic, 0, sizeof(header.magic));
    strncpy(header.magic, "MAXFILE C3", 10);
    
    // Determine chunk type from model
    C3ChunkType type = model.GetType();
    if (type == C3ChunkType::PHY || type == C3ChunkType::PHY3 || type == C3ChunkType::PHY4) {
        const char* typeStr = (type == C3ChunkType::PHY3) ? "PHY3" : 
                            (type == C3ChunkType::PHY4) ? "PHY4" : "PHY ";
        memcpy(header.physicsType, typeStr, 4);
    } else if (type == C3ChunkType::SHAP) {
        memcpy(header.physicsType, "SHAP", 4);
    } else if (type == C3ChunkType::PTCL) {
        memcpy(header.physicsType, "PTCL", 4);
    } else {
        memcpy(header.physicsType, "PHY ", 4);
    }

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write PHYS/PHY chunks
    const auto& meshes = model.GetMeshes();
    for (const auto& mesh : meshes) {
        WritePHYChunk(file, mesh, type);
    }

    // Write MOTI chunks (animations)
    const auto& anims = model.GetAnimations();
    for (const auto& anim : anims) {
        WriteMOTIChunk(file, anim);
    }

    // Write SHAP chunks
    const auto& shapes = model.GetShapes();
    for (const auto& shape : shapes) {
        WriteSHAPChunk(file, shape);
    }

    // Write PTCL chunks
    const auto& particles = model.GetParticles();
    for (const auto& ps : particles) {
        WritePTCLChunk(file, ps);
    }

    file.close();
    return true;
}

void C3Writer::WritePHYChunk(std::ofstream& file, const C3Model::MeshPart& mesh, C3ChunkType type) {
    // Chunk header
    ChunkHeader chunk;
    if (type == C3ChunkType::PHY3) {
        memcpy(chunk.byChunkID, "PHY3", 4);
    } else if (type == C3ChunkType::PHY4) {
        memcpy(chunk.byChunkID, "PHY4", 4);
    } else {
        memcpy(chunk.byChunkID, "PHY ", 4);
    }
    chunk.dwChunkSize = 0;
    size_t chunkStart = file.tellp();
    file.write(reinterpret_cast<const char*>(&chunk), sizeof(chunk));

    // Name
    uint32_t nameLen = static_cast<uint32_t>(mesh.name.length());
    file.write(reinterpret_cast<const char*>(&nameLen), 4);
    file.write(mesh.name.c_str(), nameLen);
    chunk.dwChunkSize += 4 + nameLen;

    // Blend count
    file.write(reinterpret_cast<const char*>(&mesh.blendCount), 4);
    chunk.dwChunkSize += 4;

    // Vertex counts
    uint32_t normalVertCount = 0;
    uint32_t alphaVertCount = 0;
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        if (i < mesh.normalIndices.size() / 3) {
            normalVertCount++;
        } else {
            alphaVertCount++;
        }
    }
    file.write(reinterpret_cast<const char*>(&normalVertCount), 4);
    file.write(reinterpret_cast<const char*>(&alphaVertCount), 4);
    chunk.dwChunkSize += 8;

    // Vertices
    uint32_t totalVerts = static_cast<uint32_t>(mesh.vertices.size());
    file.write(reinterpret_cast<const char*>(mesh.vertices.data()), totalVerts * sizeof(PhyVertex));
    chunk.dwChunkSize += totalVerts * sizeof(PhyVertex);

    // Triangle counts
    uint32_t normalTriCount = static_cast<uint32_t>(mesh.normalIndices.size() / 3);
    uint32_t alphaTriCount = static_cast<uint32_t>(mesh.alphaIndices.size() / 3);
    file.write(reinterpret_cast<const char*>(&normalTriCount), 4);
    file.write(reinterpret_cast<const char*>(&alphaTriCount), 4);
    chunk.dwChunkSize += 8;

    // Indices
    if (!mesh.normalIndices.empty()) {
        file.write(reinterpret_cast<const char*>(mesh.normalIndices.data()), mesh.normalIndices.size() * sizeof(uint16_t));
        chunk.dwChunkSize += mesh.normalIndices.size() * sizeof(uint16_t);
    }
    if (!mesh.alphaIndices.empty()) {
        file.write(reinterpret_cast<const char*>(mesh.alphaIndices.data()), mesh.alphaIndices.size() * sizeof(uint16_t));
        chunk.dwChunkSize += mesh.alphaIndices.size() * sizeof(uint16_t);
    }

    // Texture name
    uint32_t texLen = static_cast<uint32_t>(mesh.textureName.length());
    file.write(reinterpret_cast<const char*>(&texLen), 4);
    file.write(mesh.textureName.c_str(), texLen);
    chunk.dwChunkSize += 4 + texLen;

    // Bounding box
    file.write(reinterpret_cast<const char*>(&mesh.bboxMin), sizeof(XMFLOAT3));
    file.write(reinterpret_cast<const char*>(&mesh.bboxMax), sizeof(XMFLOAT3));
    chunk.dwChunkSize += sizeof(XMFLOAT3) * 2;

    // Initial matrix
    file.write(reinterpret_cast<const char*>(&mesh.initialMatrix), sizeof(XMFLOAT4X4));
    chunk.dwChunkSize += sizeof(XMFLOAT4X4);

    // Texture row
    file.write(reinterpret_cast<const char*>(&mesh.textureRow), 4);
    chunk.dwChunkSize += 4;

    // Keyframes
    uint32_t alphaKeyCount = static_cast<uint32_t>(mesh.alphaKeyframes.size());
    file.write(reinterpret_cast<const char*>(&alphaKeyCount), 4);
    if (alphaKeyCount > 0) {
        file.write(reinterpret_cast<const char*>(mesh.alphaKeyframes.data()), alphaKeyCount * sizeof(C3KeyFrame));
        chunk.dwChunkSize += alphaKeyCount * sizeof(C3KeyFrame);
    }
    chunk.dwChunkSize += 4;

    uint32_t drawKeyCount = static_cast<uint32_t>(mesh.drawKeyframes.size());
    file.write(reinterpret_cast<const char*>(&drawKeyCount), 4);
    if (drawKeyCount > 0) {
        file.write(reinterpret_cast<const char*>(mesh.drawKeyframes.data()), drawKeyCount * sizeof(C3KeyFrame));
        chunk.dwChunkSize += drawKeyCount * sizeof(C3KeyFrame);
    }
    chunk.dwChunkSize += 4;

    // Update chunk size
    size_t currentPos = file.tellp();
    file.seekp(chunkStart);
    file.write(reinterpret_cast<const char*>(&chunk), sizeof(chunk));
    file.seekp(currentPos);
}

void C3Writer::WriteMOTIChunk(std::ofstream& file, const C3Model::Animation& anim) {
    ChunkHeader chunk;
    memcpy(chunk.byChunkID, "MOTI", 4);
    chunk.dwChunkSize = 0;
    size_t chunkStart = file.tellp();
    file.write(reinterpret_cast<const char*>(&chunk), sizeof(chunk));

    // Bone count
    file.write(reinterpret_cast<const char*>(&anim.boneCount), 4);
    chunk.dwChunkSize += 4;

    // Frame count
    file.write(reinterpret_cast<const char*>(&anim.frameCount), 4);
    chunk.dwChunkSize += 4;

    // Keyframe format (KKEY for now)
    const char kkey[] = "KKEY";
    file.write(kkey, 4);
    chunk.dwChunkSize += 4;

    // Keyframe count
    uint32_t keyFrameCount = static_cast<uint32_t>(anim.keyFrames.size());
    file.write(reinterpret_cast<const char*>(&keyFrameCount), 4);
    chunk.dwChunkSize += 4;

    // Keyframes
    for (const auto& kf : anim.keyFrames) {
        file.write(reinterpret_cast<const char*>(&kf.frame), 4);
        chunk.dwChunkSize += 4;

        for (const auto& matrix : kf.boneMatrices) {
            file.write(reinterpret_cast<const char*>(&matrix), sizeof(XMFLOAT4X4));
            chunk.dwChunkSize += sizeof(XMFLOAT4X4);
        }
    }

    // Morph weights
    file.write(reinterpret_cast<const char*>(&anim.morphCount), 4);
    chunk.dwChunkSize += 4;

    if (anim.morphCount > 0 && anim.frameCount > 0) {
        file.write(reinterpret_cast<const char*>(anim.morphWeights.data()), 
                   anim.morphCount * anim.frameCount * sizeof(float));
        chunk.dwChunkSize += anim.morphCount * anim.frameCount * sizeof(float);
    }

    // Update chunk size
    size_t currentPos = file.tellp();
    file.seekp(chunkStart);
    file.write(reinterpret_cast<const char*>(&chunk), sizeof(chunk));
    file.seekp(currentPos);
}

void C3Writer::WriteSHAPChunk(std::ofstream& file, const C3Model::ShapeData& shape) {
    ChunkHeader chunk;
    memcpy(chunk.byChunkID, "SHAP", 4);
    chunk.dwChunkSize = 0;
    size_t chunkStart = file.tellp();
    file.write(reinterpret_cast<const char*>(&chunk), sizeof(chunk));

    // Name
    uint32_t nameLen = static_cast<uint32_t>(shape.name.length());
    file.write(reinterpret_cast<const char*>(&nameLen), 4);
    file.write(shape.name.c_str(), nameLen);
    chunk.dwChunkSize += 4 + nameLen;

    // Line count
    uint32_t lineCount = static_cast<uint32_t>(shape.lines.size());
    file.write(reinterpret_cast<const char*>(&lineCount), 4);
    chunk.dwChunkSize += 4;

    // Lines
    for (const auto& line : shape.lines) {
        uint32_t pointCount = static_cast<uint32_t>(line.points.size());
        file.write(reinterpret_cast<const char*>(&pointCount), 4);
        file.write(reinterpret_cast<const char*>(line.points.data()), pointCount * sizeof(XMFLOAT3));
        chunk.dwChunkSize += 4 + pointCount * sizeof(XMFLOAT3);
    }

    // Texture name
    uint32_t texLen = static_cast<uint32_t>(shape.textureName.length());
    file.write(reinterpret_cast<const char*>(&texLen), 4);
    file.write(shape.textureName.c_str(), texLen);
    chunk.dwChunkSize += 4 + texLen;

    // Segment count
    file.write(reinterpret_cast<const char*>(&shape.segmentCount), 4);
    chunk.dwChunkSize += 4;

    // Update chunk size
    size_t currentPos = file.tellp();
    file.seekp(chunkStart);
    file.write(reinterpret_cast<const char*>(&chunk), sizeof(chunk));
    file.seekp(currentPos);
}

void C3Writer::WritePTCLChunk(std::ofstream& file, const C3Model::ParticleSystem& ps) {
    ChunkHeader chunk;
    memcpy(chunk.byChunkID, "PTCL", 4);
    chunk.dwChunkSize = 0;
    size_t chunkStart = file.tellp();
    file.write(reinterpret_cast<const char*>(&chunk), sizeof(chunk));

    // Name
    uint32_t nameLen = static_cast<uint32_t>(ps.name.length());
    file.write(reinterpret_cast<const char*>(&nameLen), 4);
    file.write(ps.name.c_str(), nameLen);
    chunk.dwChunkSize += 4 + nameLen;

    // Emitter position
    file.write(reinterpret_cast<const char*>(&ps.emitterPos), sizeof(XMFLOAT3));
    chunk.dwChunkSize += sizeof(XMFLOAT3);

    // Properties
    file.write(reinterpret_cast<const char*>(&ps.emitRate), 4);
    file.write(reinterpret_cast<const char*>(&ps.lifetime), 4);
    file.write(reinterpret_cast<const char*>(&ps.speed), 4);
    chunk.dwChunkSize += 12;

    // Size
    file.write(reinterpret_cast<const char*>(&ps.size), sizeof(XMFLOAT3));
    chunk.dwChunkSize += sizeof(XMFLOAT3);

    // Colors
    file.write(reinterpret_cast<const char*>(&ps.startColor), sizeof(XMFLOAT4));
    file.write(reinterpret_cast<const char*>(&ps.endColor), sizeof(XMFLOAT4));
    chunk.dwChunkSize += sizeof(XMFLOAT4) * 2;

    // Texture name
    uint32_t texLen = static_cast<uint32_t>(ps.textureName.length());
    file.write(reinterpret_cast<const char*>(&texLen), 4);
    file.write(ps.textureName.c_str(), texLen);
    chunk.dwChunkSize += 4 + texLen;

    // Max particles
    file.write(reinterpret_cast<const char*>(&ps.maxParticles), 4);
    chunk.dwChunkSize += 4;

    // Update chunk size
    size_t currentPos = file.tellp();
    file.seekp(chunkStart);
    file.write(reinterpret_cast<const char*>(&chunk), sizeof(chunk));
    file.seekp(currentPos);
}

