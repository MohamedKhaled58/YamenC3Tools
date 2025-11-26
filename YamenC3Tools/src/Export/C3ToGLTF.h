#pragma once
#include "C3Exporter.h"

class C3ToGLTF : public C3Exporter {
public:
    bool Export(const C3Model& model, const ExportOptions& options) override;
    const char* GetFormatName() const override { return "glTF 2.0"; }
    const char* GetFileExtension() const override { return ".gltf"; }

private:
    struct BufferData {
        std::vector<uint8_t> data;
        size_t WriteFloat3(const XMFLOAT3& v);
        size_t WriteFloat2(const XMFLOAT2& v);
        size_t WriteFloat4(const XMFLOAT4& v);
        size_t WriteUInt16(uint16_t v);
        void Align4();
    };

    bool ExportMesh(const C3Model::MeshPart& mesh, BufferData& buffer,
        const ExportOptions& opts, void* jsonMesh);
};