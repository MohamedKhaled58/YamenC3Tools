#include "C3ToGLTF.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

size_t C3ToGLTF::BufferData::WriteFloat3(const XMFLOAT3& v) {
    size_t offset = data.size();
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&v);
    data.insert(data.end(), bytes, bytes + sizeof(XMFLOAT3));
    return offset;
}

size_t C3ToGLTF::BufferData::WriteFloat2(const XMFLOAT2& v) {
    size_t offset = data.size();
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&v);
    data.insert(data.end(), bytes, bytes + sizeof(XMFLOAT2));
    return offset;
}

size_t C3ToGLTF::BufferData::WriteFloat4(const XMFLOAT4& v) {
    size_t offset = data.size();
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&v);
    data.insert(data.end(), bytes, bytes + sizeof(XMFLOAT4));
    return offset;
}

size_t C3ToGLTF::BufferData::WriteUInt16(uint16_t v) {
    size_t offset = data.size();
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&v);
    data.insert(data.end(), bytes, bytes + sizeof(uint16_t));
    return offset;
}

void C3ToGLTF::BufferData::Align4() {
    while (data.size() % 4 != 0) {
        data.push_back(0);
    }
}

bool C3ToGLTF::Export(const C3Model& model, const ExportOptions& options) {
    const auto& meshes = model.GetMeshes();
    if (meshes.empty()) {
        m_lastError = "No meshes to export";
        return false;
    }

    json gltf;
    gltf["asset"] = {
        {"version", "2.0"},
        {"generator", "Yamen C3 Tools v1.0"}
    };

    BufferData bufferData;
    gltf["bufferViews"] = json::array();
    gltf["accessors"] = json::array();
    gltf["meshes"] = json::array();

    int accessorIdx = 0;

    // Export first mesh (multi-mesh support can be added later)
    const auto& mesh = meshes[0];

    // Calculate bounds
    XMFLOAT3 minPos{ FLT_MAX, FLT_MAX, FLT_MAX };
    XMFLOAT3 maxPos{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (const auto& v : mesh.vertices) {
        minPos.x = std::min(minPos.x, v.positions[0].x);
        minPos.y = std::min(minPos.y, v.positions[0].y);
        minPos.z = std::min(minPos.z, v.positions[0].z);
        maxPos.x = std::max(maxPos.x, v.positions[0].x);
        maxPos.y = std::max(maxPos.y, v.positions[0].y);
        maxPos.z = std::max(maxPos.z, v.positions[0].z);
    }

    // Write position data (base morph target)
    size_t posOffset = bufferData.data.size();
    for (const auto& v : mesh.vertices) {
        bufferData.WriteFloat3(v.positions[0]);
    }
    bufferData.Align4();
    size_t posSize = bufferData.data.size() - posOffset;

    gltf["bufferViews"].push_back({
        {"buffer", 0},
        {"byteOffset", posOffset},
        {"byteLength", posSize},
        {"target", 34962}
        });

    gltf["accessors"].push_back({
        {"bufferView", gltf["bufferViews"].size() - 1},
        {"componentType", 5126},
        {"count", mesh.vertices.size()},
        {"type", "VEC3"},
        {"min", json::array({minPos.x, minPos.y, minPos.z})},
        {"max", json::array({maxPos.x, maxPos.y, maxPos.z})}
        });
    int posAccessor = accessorIdx++;

    // Write normals (calculated from morph targets)
    size_t normOffset = bufferData.data.size();
    for (const auto& v : mesh.vertices) {
        XMFLOAT3 v0 = v.positions[0];
        XMFLOAT3 v1 = v.positions[1];
        XMFLOAT3 v2 = v.positions[2];

        XMVECTOR p0 = XMLoadFloat3(&v0);
        XMVECTOR p1 = XMLoadFloat3(&v1);
        XMVECTOR p2 = XMLoadFloat3(&v2);

        XMVECTOR tangent1 = XMVectorSubtract(p1, p0);
        XMVECTOR tangent2 = XMVectorSubtract(p2, p0);
        XMVECTOR normal = XMVector3Normalize(XMVector3Cross(tangent1, tangent2));

        XMFLOAT3 n;
        XMStoreFloat3(&n, normal);
        bufferData.WriteFloat3(n);
    }
    bufferData.Align4();
    size_t normSize = bufferData.data.size() - normOffset;

    gltf["bufferViews"].push_back({
        {"buffer", 0},
        {"byteOffset", normOffset},
        {"byteLength", normSize},
        {"target", 34962}
        });

    gltf["accessors"].push_back({
        {"bufferView", gltf["bufferViews"].size() - 1},
        {"componentType", 5126},
        {"count", mesh.vertices.size()},
        {"type", "VEC3"}
        });
    int normAccessor = accessorIdx++;

    // Write UVs
    size_t uvOffset = bufferData.data.size();
    for (const auto& v : mesh.vertices) {
        bufferData.WriteFloat2(XMFLOAT2(v.u, v.v));
    }
    bufferData.Align4();
    size_t uvSize = bufferData.data.size() - uvOffset;

    gltf["bufferViews"].push_back({
        {"buffer", 0},
        {"byteOffset", uvOffset},
        {"byteLength", uvSize},
        {"target", 34962}
        });

    gltf["accessors"].push_back({
        {"bufferView", gltf["bufferViews"].size() - 1},
        {"componentType", 5126},
        {"count", mesh.vertices.size()},
        {"type", "VEC2"}
        });
    int uvAccessor = accessorIdx++;

    // Write colors
    size_t colorOffset = bufferData.data.size();
    for (const auto& v : mesh.vertices) {
        uint8_t a = (v.color >> 24) & 0xFF;
        uint8_t r = (v.color >> 16) & 0xFF;
        uint8_t g = (v.color >> 8) & 0xFF;
        uint8_t b = v.color & 0xFF;
        bufferData.WriteFloat4(XMFLOAT4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f));
    }
    bufferData.Align4();
    size_t colorSize = bufferData.data.size() - colorOffset;

    gltf["bufferViews"].push_back({
        {"buffer", 0},
        {"byteOffset", colorOffset},
        {"byteLength", colorSize},
        {"target", 34962}
        });

    gltf["accessors"].push_back({
        {"bufferView", gltf["bufferViews"].size() - 1},
        {"componentType", 5126},
        {"count", mesh.vertices.size()},
        {"type", "VEC4"}
        });
    int colorAccessor = accessorIdx++;

    // Write indices (combine normal + alpha)
    std::vector<uint16_t> allIndices = mesh.normalIndices;
    allIndices.insert(allIndices.end(), mesh.alphaIndices.begin(), mesh.alphaIndices.end());

    size_t idxOffset = bufferData.data.size();
    for (uint16_t idx : allIndices) {
        bufferData.WriteUInt16(idx);
    }
    bufferData.Align4();
    size_t idxSize = bufferData.data.size() - idxOffset;

    gltf["bufferViews"].push_back({
        {"buffer", 0},
        {"byteOffset", idxOffset},
        {"byteLength", idxSize},
        {"target", 34963}
        });

    gltf["accessors"].push_back({
        {"bufferView", gltf["bufferViews"].size() - 1},
        {"componentType", 5123},
        {"count", allIndices.size()},
        {"type", "SCALAR"}
        });
    int idxAccessor = accessorIdx++;

    // Morph targets (if enabled)
    std::vector<int> morphAccessors;
    if (options.exportMorphTargets) {
        for (int target = 1; target < 4; target++) {
            size_t morphOffset = bufferData.data.size();
            for (const auto& v : mesh.vertices) {
                XMFLOAT3 delta;
                delta.x = v.positions[target].x - v.positions[0].x;
                delta.y = v.positions[target].y - v.positions[0].y;
                delta.z = v.positions[target].z - v.positions[0].z;
                bufferData.WriteFloat3(delta);
            }
            bufferData.Align4();
            size_t morphSize = bufferData.data.size() - morphOffset;

            gltf["bufferViews"].push_back({
                {"buffer", 0},
                {"byteOffset", morphOffset},
                {"byteLength", morphSize},
                {"target", 34962}
                });

            gltf["accessors"].push_back({
                {"bufferView", gltf["bufferViews"].size() - 1},
                {"componentType", 5126},
                {"count", mesh.vertices.size()},
                {"type", "VEC3"}
                });
            morphAccessors.push_back(accessorIdx++);
        }
    }

    // Create mesh primitive
    json primitive = {
        {"attributes", {
            {"POSITION", posAccessor},
            {"NORMAL", normAccessor},
            {"TEXCOORD_0", uvAccessor},
            {"COLOR_0", colorAccessor}
        }},
        {"indices", idxAccessor},
        {"mode", 4}
    };

    if (!morphAccessors.empty()) {
        json targets = json::array();
        for (int ma : morphAccessors) {
            targets.push_back({ {"POSITION", ma} });
        }
        primitive["targets"] = targets;
    }

    gltf["meshes"].push_back({
        {"name", mesh.name},
        {"primitives", json::array({primitive})}
        });

    // Nodes and scene
    gltf["nodes"] = json::array({
        {{"name", "C3Model"}, {"mesh", 0}}
        });

    gltf["scenes"] = json::array({
        {{"name", "Scene"}, {"nodes", json::array({0})}}
        });
    gltf["scene"] = 0;

    // Write binary file
    std::string binPath = options.outputPath + ".bin";
    std::ofstream binFile(binPath, std::ios::binary);
    if (!binFile) {
        m_lastError = "Failed to create .bin file";
        return false;
    }
    binFile.write(reinterpret_cast<const char*>(bufferData.data.data()), bufferData.data.size());
    binFile.close();

    // Write glTF JSON
    gltf["buffers"] = json::array({
        {{"byteLength", bufferData.data.size()}, {"uri", binPath.substr(binPath.find_last_of("/\\") + 1)}}
        });

    std::string gltfPath = options.outputPath + ".gltf";
    std::ofstream gltfFile(gltfPath);
    if (!gltfFile) {
        m_lastError = "Failed to create .gltf file";
        return false;
    }
    gltfFile << gltf.dump(2);
    gltfFile.close();

    return true;
}