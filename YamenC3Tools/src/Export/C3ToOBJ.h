#pragma once
#include "C3Exporter.h"

class C3ToOBJ : public C3Exporter {
public:
    bool Export(const C3Model& model, const ExportOptions& options) override;
    const char* GetFormatName() const override { return "Wavefront OBJ"; }
    const char* GetFileExtension() const override { return ".obj"; }
};

