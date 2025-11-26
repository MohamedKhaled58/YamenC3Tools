#pragma once
#include "../Core/C3Model.h"
#include "../Core/C3Types.h"
#include <fstream>
#include <string>

class C3Writer {
public:
    bool Write(const C3Model& model, const std::string& path);
    const std::string& GetLastError() const { return m_lastError; }

private:
    void WritePHYChunk(std::ofstream& file, const C3Model::MeshPart& mesh, C3ChunkType type);
    void WriteMOTIChunk(std::ofstream& file, const C3Model::Animation& anim);
    void WriteSHAPChunk(std::ofstream& file, const C3Model::ShapeData& shape);
    void WritePTCLChunk(std::ofstream& file, const C3Model::ParticleSystem& ps);

    std::string m_lastError;
};

