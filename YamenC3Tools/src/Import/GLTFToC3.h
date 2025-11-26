#pragma once
#include "C3Importer.h"
#include <nlohmann/json.hpp>

class GLTFToC3 : public C3Importer {
public:
    bool Import(const std::string& path, C3Model& outModel, const ImportOptions& options) override;
    const char* GetFormatName() const override { return "glTF 2.0"; }
    const char* GetFileExtension() const override { return ".gltf"; }

private:
    using json = nlohmann::json;

    std::vector<XMFLOAT3> ReadVec3Accessor(const json& gltf, const std::vector<uint8_t>& binData, int accessorIdx);
    std::vector<XMFLOAT2> ReadVec2Accessor(const json& gltf, const std::vector<uint8_t>& binData, int accessorIdx);
    std::vector<XMFLOAT4> ReadVec4Accessor(const json& gltf, const std::vector<uint8_t>& binData, int accessorIdx);
    std::vector<uint16_t> ReadU16Accessor(const json& gltf, const std::vector<uint8_t>& binData, int accessorIdx);
};