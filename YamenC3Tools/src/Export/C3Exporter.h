#pragma once
#include "../Core/C3Model.h"
#include <string>

class C3Exporter {
public:
    struct ExportOptions {
        bool exportMorphTargets = true;
        bool exportVertexColors = true;
        bool exportNormals = true;
        bool exportTexCoords = true;
        bool embedBinary = false;
        std::string outputPath;
    };

    virtual ~C3Exporter() = default;
    virtual bool Export(const C3Model& model, const ExportOptions& options) = 0;
    virtual const char* GetFormatName() const = 0;
    virtual const char* GetFileExtension() const = 0;
    const std::string& GetLastError() const { return m_lastError; }

protected:
    std::string m_lastError;
};