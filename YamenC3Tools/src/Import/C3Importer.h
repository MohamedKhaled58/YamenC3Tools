#pragma once
#include "../Core/C3Model.h"
#include <string>

class C3Importer {
public:
    struct ImportOptions {
        bool preserveMorphTargets = true;
        bool importVertexColors = true;
        bool importNormals = true;
        bool importTexCoords = true;
        std::string inputPath;
    };

    virtual ~C3Importer() = default;
    virtual bool Import(const std::string& path, C3Model& outModel, const ImportOptions& options) = 0;
    virtual const char* GetFormatName() const = 0;
    virtual const char* GetFileExtension() const = 0;

protected:
    std::string m_lastError;
};