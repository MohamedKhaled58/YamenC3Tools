#include "GLTFToC3.h"
#include "../Core/C3Model.h"
#include "../Core/C3Types.h"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <DirectXMath.h>

using namespace DirectX;
using json = nlohmann::json;

bool GLTFToC3::Import(const std::string& path, C3Model& outModel, const ImportOptions& options) {
    // Parse glTF JSON
    std::ifstream file(path);
    if (!file) {
        m_lastError = "Failed to open glTF file";
        return false;
    }

    json gltf;
    try {
        file >> gltf;
    }
    catch (const std::exception& e) {
        m_lastError = std::string("Failed to parse JSON: ") + e.what();
        return false;
    }
    file.close();

    // Load binary data
    std::filesystem::path gltfPath(path);
    std::string binPath = gltfPath.parent_path().string() + "/" +
        gltf["buffers"][0]["uri"].get<std::string>();

    std::ifstream binFile(binPath, std::ios::binary);
    if (!binFile) {
        m_lastError = "Failed to open .bin file";
        return false;
    }

    binFile.seekg(0, std::ios::end);
    size_t binSize = binFile.tellg();
    binFile.seekg(0, std::ios::beg);

    std::vector<uint8_t> binData(binSize);
    binFile.read(reinterpret_cast<char*>(binData.data()), binSize);
    binFile.close();

    // Parse mesh
    if (!gltf.contains("meshes") || gltf["meshes"].empty()) {
        m_lastError = "No meshes found in glTF";
        return false;
    }

    const auto& mesh = gltf["meshes"][0];
    const auto& primitive = mesh["primitives"][0];
    const auto& attributes = primitive["attributes"];

    // Read positions
    int posAccessor = attributes["POSITION"];
    std::vector<XMFLOAT3> positions = ReadVec3Accessor(gltf, binData, posAccessor);

    // Read morph targets
    std::vector<std::vector<XMFLOAT3>> morphTargets;
    if (primitive.contains("targets") && options.preserveMorphTargets) {
        for (const auto& target : primitive["targets"]) {
            int morphAccessor = target["POSITION"];
            morphTargets.push_back(ReadVec3Accessor(gltf, binData, morphAccessor));
        }
    }

    // Read UVs
    std::vector<XMFLOAT2> uvs;
    if (attributes.contains("TEXCOORD_0")) {
        uvs = ReadVec2Accessor(gltf, binData, attributes["TEXCOORD_0"]);
    }
    else {
        uvs.resize(positions.size(), XMFLOAT2{ 0, 0 });
    }

    // Read colors
    std::vector<XMFLOAT4> colors;
    if (attributes.contains("COLOR_0") && options.importVertexColors) {
        colors = ReadVec4Accessor(gltf, binData, attributes["COLOR_0"]);
    }
    else {
        colors.resize(positions.size(), XMFLOAT4{ 1, 1, 1, 1 });
    }

    // Read indices
    int indexAccessor = primitive["indices"];
    std::vector<uint16_t> indices = ReadU16Accessor(gltf, binData, indexAccessor);

    // Convert to C3 format
    C3Model::MeshPart part;
    part.name = mesh.contains("name") ? mesh["name"].get<std::string>() : "imported_mesh";
    part.vertices.resize(positions.size());

    for (size_t i = 0; i < positions.size(); i++) {
        PhyVertex& v = part.vertices[i];

        // Base position
        v.positions[0] = positions[i];

        // Morph targets (add deltas to base)
        for (int t = 0; t < 3 && static_cast<size_t>(t) < morphTargets.size(); t++) {
            v.positions[t + 1] = XMFLOAT3{
                positions[i].x + morphTargets[t][i].x,
                positions[i].y + morphTargets[t][i].y,
                positions[i].z + morphTargets[t][i].z
            };
        }

        // Fill remaining morph targets with base
        for (int t = morphTargets.size(); t < 3; t++) {
            v.positions[t + 1] = positions[i];
        }

        v.u = uvs[i].x;
        v.v = uvs[i].y;

        // Convert float color to ARGB
        uint8_t a = static_cast<uint8_t>(colors[i].w * 255);
        uint8_t r = static_cast<uint8_t>(colors[i].x * 255);
        uint8_t g = static_cast<uint8_t>(colors[i].y * 255);
        uint8_t b = static_cast<uint8_t>(colors[i].z * 255);
        v.color = (a << 24) | (r << 16) | (g << 8) | b;

        v.boneIndices[0] = 0;
        v.boneIndices[1] = 0;
        v.boneWeights[0] = 1.0f;
        v.boneWeights[1] = 0.0f;
    }

    part.normalIndices = indices;

    // Add mesh to model (using non-const version)
    outModel.GetMeshes().push_back(part);
    
    return true;
}

std::vector<XMFLOAT3> GLTFToC3::ReadVec3Accessor(const json& gltf, const std::vector<uint8_t>& binData, int accessorIdx) {
    const auto& accessor = gltf["accessors"][accessorIdx];
    const auto& bufferView = gltf["bufferViews"][int(accessor["bufferView"])];

    size_t offset = bufferView.value("byteOffset", 0) + accessor.value("byteOffset", 0);
    size_t count = accessor["count"];

    std::vector<XMFLOAT3> result(count);
    memcpy(result.data(), binData.data() + offset, count * sizeof(XMFLOAT3));
    return result;
}

std::vector<XMFLOAT2> GLTFToC3::ReadVec2Accessor(const json& gltf, const std::vector<uint8_t>& binData, int accessorIdx) {
    const auto& accessor = gltf["accessors"][accessorIdx];
    const auto& bufferView = gltf["bufferViews"][int(accessor["bufferView"])];

    size_t offset = bufferView.value("byteOffset", 0) + accessor.value("byteOffset", 0);
    size_t count = accessor["count"];

    std::vector<XMFLOAT2> result(count);
    memcpy(result.data(), binData.data() + offset, count * sizeof(XMFLOAT2));
    return result;
}

std::vector<XMFLOAT4> GLTFToC3::ReadVec4Accessor(const json& gltf, const std::vector<uint8_t>& binData, int accessorIdx) {
    const auto& accessor = gltf["accessors"][accessorIdx];
    const auto& bufferView = gltf["bufferViews"][int(accessor["bufferView"])];

    size_t offset = bufferView.value("byteOffset", 0) + accessor.value("byteOffset", 0);
    size_t count = accessor["count"];

    std::vector<XMFLOAT4> result(count);
    memcpy(result.data(), binData.data() + offset, count * sizeof(XMFLOAT4));
    return result;
}

std::vector<uint16_t> GLTFToC3::ReadU16Accessor(const json& gltf, const std::vector<uint8_t>& binData, int accessorIdx) {
    const auto& accessor = gltf["accessors"][accessorIdx];
    const auto& bufferView = gltf["bufferViews"][int(accessor["bufferView"])];

    size_t offset = bufferView.value("byteOffset", 0) + accessor.value("byteOffset", 0);
    size_t count = accessor["count"];

    std::vector<uint16_t> result(count);
    memcpy(result.data(), binData.data() + offset, count * sizeof(uint16_t));
    return result;
}