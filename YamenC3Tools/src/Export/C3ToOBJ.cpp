#include "C3Exporter.h"
#include "../Core/C3Types.h"
#include <fstream>

class C3ToOBJ : public C3Exporter {
public:
    bool Export(const C3Model& model, const ExportOptions& options) override {
        const auto& meshes = model.GetMeshes();
        if (meshes.empty()) {
            m_lastError = "No meshes to export";
            return false;
        }

        std::string objPath = options.outputPath + ".obj";
        std::ofstream file(objPath);
        if (!file) {
            m_lastError = "Failed to create .obj file";
            return false;
        }

        file << "# Exported from Yamen C3 Tools\n";
        //file << "# C3 Model: " << model.ChunkTypeToString() << "\n\n";

        const auto& mesh = meshes[0];

        // Write vertices (use base morph target)
        for (const auto& v : mesh.vertices) {
            file << "v " << v.positions[0].x << " "
                << v.positions[0].y << " "
                << v.positions[0].z << "\n";
        }
        file << "\n";

        // Write UVs
        for (const auto& v : mesh.vertices) {
            file << "vt " << v.u << " " << v.v << "\n";
        }
        file << "\n";

        // Write normals (calculate from first two morph targets)
        for (const auto& v : mesh.vertices) {
            XMFLOAT3 v0 = v.positions[0];
            XMFLOAT3 v1 = v.positions[1];
            XMFLOAT3 v2 = v.positions[2];

            float dx1 = v1.x - v0.x, dy1 = v1.y - v0.y, dz1 = v1.z - v0.z;
            float dx2 = v2.x - v0.x, dy2 = v2.y - v0.y, dz2 = v2.z - v0.z;

            float nx = dy1 * dz2 - dz1 * dy2;
            float ny = dz1 * dx2 - dx1 * dz2;
            float nz = dx1 * dy2 - dy1 * dx2;
            float len = sqrtf(nx * nx + ny * ny + nz * nz);

            if (len > 0.001f) {
                nx /= len; ny /= len; nz /= len;
            }
            else {
                nx = 0; ny = 1; nz = 0;
            }

            file << "vn " << nx << " " << ny << " " << nz << "\n";
        }
        file << "\n";

        // Write faces (OBJ uses 1-based indexing)
        std::vector<uint16_t> allIndices = mesh.normalIndices;
        allIndices.insert(allIndices.end(), mesh.alphaIndices.begin(), mesh.alphaIndices.end());

        for (size_t i = 0; i < allIndices.size(); i += 3) {
            file << "f ";
            for (int j = 0; j < 3; j++) {
                int idx = allIndices[i + j] + 1; // 1-based
                file << idx << "/" << idx << "/" << idx;
                if (j < 2) file << " ";
            }
            file << "\n";
        }

        file.close();
        return true;
    }

    const char* GetFormatName() const override { return "Wavefront OBJ"; }
    const char* GetFileExtension() const override { return ".obj"; }
};