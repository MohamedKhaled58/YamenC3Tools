#include <Windows.h>
#include <memory>
#include "Core/C3Model.h"
#include "Core/C3Types.h"
#include "Renderer/D3D11Renderer.h"
#include "Renderer/Camera.h"
#include "UI/ImGuiManager.h"
#include "Utils/FileDialog.h"
#include "Export/C3Writer.h"
#include "Export/C3ToGLTF.h"
#include "Export/C3ToOBJ.h"
#include "Import/GLTFToC3.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <filesystem>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct AppState {
    std::unique_ptr<D3D11Renderer> renderer;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<ImGuiManager> imgui;
    std::unique_ptr<C3Model> model;

    bool modelLoaded = false;
    bool wireframe = false;
    bool playAnimation = false;  // Start paused
    float animationSpeed = 1.0f;
    float animationTime = 0.0f;
    float morphWeights[4] = { 1.0f, 0.0f, 0.0f, 0.0f };

    int windowWidth = 1600;
    int windowHeight = 900;

    POINT lastMousePos = { 0, 0 };
    bool mouseLeftDown = false;
    bool mouseMiddleDown = false;

    std::string loadedFilePath;
    std::string statusMessage;
    
    // Import/Export
    std::unique_ptr<C3Writer> c3Writer;
    std::unique_ptr<C3ToGLTF> gltfExporter;
    std::unique_ptr<C3ToOBJ> objExporter;
    std::unique_ptr<GLTFToC3> gltfImporter;
};

AppState g_appState;

void RenderUI();
void LoadC3File();
void MergeC3File();
void ExportC3File();
void ExportToGLTF();
void ExportToOBJ();
void ImportFromGLTF();

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_appState.windowWidth = LOWORD(lParam);
            g_appState.windowHeight = HIWORD(lParam);
            if (g_appState.renderer) {
                g_appState.renderer->Resize(g_appState.windowWidth, g_appState.windowHeight);
            }
            if (g_appState.camera) {
                g_appState.camera->SetAspect((float)g_appState.windowWidth / g_appState.windowHeight);
            }
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (!g_appState.imgui || !g_appState.imgui->WantCaptureMouse()) {
            g_appState.mouseLeftDown = true;
            g_appState.lastMousePos.x = LOWORD(lParam);
            g_appState.lastMousePos.y = HIWORD(lParam);
            SetCapture(hwnd);
        }
        return 0;

    case WM_LBUTTONUP:
        g_appState.mouseLeftDown = false;
        ReleaseCapture();
        return 0;

    case WM_MBUTTONDOWN:
        if (!g_appState.imgui || !g_appState.imgui->WantCaptureMouse()) {
            g_appState.mouseMiddleDown = true;
            g_appState.lastMousePos.x = LOWORD(lParam);
            g_appState.lastMousePos.y = HIWORD(lParam);
            SetCapture(hwnd);
        }
        return 0;

    case WM_MBUTTONUP:
        g_appState.mouseMiddleDown = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (!g_appState.imgui || !g_appState.imgui->WantCaptureMouse()) {
            POINT currentPos = { LOWORD(lParam), HIWORD(lParam) };
            int dx = currentPos.x - g_appState.lastMousePos.x;
            int dy = currentPos.y - g_appState.lastMousePos.y;

            if (g_appState.mouseLeftDown && g_appState.camera) {
                g_appState.camera->OrbitTarget(static_cast<float>(dx) * 0.005f, static_cast<float>(-dy) * 0.005f);
            }
            if (g_appState.mouseMiddleDown && g_appState.camera) {
                g_appState.camera->Pan(static_cast<float>(-dx), static_cast<float>(dy));
            }

            g_appState.lastMousePos = currentPos;
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (!g_appState.imgui || !g_appState.imgui->WantCaptureMouse()) {
            if (g_appState.camera) {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                g_appState.camera->Zoom(static_cast<float>(delta) * 0.005f);
            }
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        // Ctrl+O shortcut
        else if (wParam == 'O' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            LoadC3File();
        }
        // Ctrl+M shortcut for merge
        else if (wParam == 'M' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            MergeC3File();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Register window class
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "YamenC3ToolsClass";
    RegisterClassExA(&wc);

    // Create window
    HWND hwnd = CreateWindowExA(
        0, "YamenC3ToolsClass", "Yamen C3 Tools - Professional C3 Asset Viewer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        g_appState.windowWidth, g_appState.windowHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        MessageBoxA(nullptr, "Failed to create window!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Initialize D3D11 Renderer
    g_appState.renderer = std::make_unique<D3D11Renderer>();
    if (!g_appState.renderer->Initialize(hwnd, g_appState.windowWidth, g_appState.windowHeight)) {
        MessageBoxA(hwnd, "Failed to initialize D3D11 renderer!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Initialize Camera
    g_appState.camera = std::make_unique<Camera>();
    g_appState.camera->SetAspect((float)g_appState.windowWidth / g_appState.windowHeight);

    // Initialize ImGui
    g_appState.imgui = std::make_unique<ImGuiManager>();
    if (!g_appState.imgui->Initialize(hwnd, g_appState.renderer->GetDevice(), g_appState.renderer->GetContext())) {
        MessageBoxA(hwnd, "Failed to initialize ImGui!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Initialize import/export
    g_appState.c3Writer = std::make_unique<C3Writer>();
    g_appState.gltfExporter = std::make_unique<C3ToGLTF>();
    g_appState.objExporter = std::make_unique<C3ToOBJ>();
    g_appState.gltfImporter = std::make_unique<GLTFToC3>();

    g_appState.statusMessage = "Ready. Press Ctrl+O or use File menu to load C3 files.";

    // Main Loop
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // === UPDATE PHASE ===

            // Update morph target animation if playing
            if (g_appState.playAnimation && g_appState.renderer) {
                g_appState.animationTime += 0.016f * g_appState.animationSpeed;

                float t = g_appState.animationTime;
                g_appState.morphWeights[0] = 0.5f + 0.5f * cosf(t);
                g_appState.morphWeights[1] = 0.5f + 0.5f * sinf(t);
                g_appState.morphWeights[2] = 0.5f + 0.5f * cosf(t + 3.14159f);
                g_appState.morphWeights[3] = 0.5f + 0.5f * sinf(t + 3.14159f);

                g_appState.renderer->SetMorphWeights(
                    g_appState.morphWeights[0],
                    g_appState.morphWeights[1],
                    g_appState.morphWeights[2],
                    g_appState.morphWeights[3]
                );
            }

            // === RENDER PHASE ===

            // Render 3D scene (clears screen, draws geometry)
            if (g_appState.renderer && g_appState.camera) {
                g_appState.renderer->Render(*g_appState.camera);
            }

            // Render ImGui UI overlay
            g_appState.imgui->NewFrame();
            RenderUI();
            ImGui::Render();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            // Present final frame to screen
            if (g_appState.renderer) {
                g_appState.renderer->GetSwapChain()->Present(1, 0);
            }
        }
    }

    // Cleanup
    g_appState.imgui->Shutdown();
    g_appState.renderer->Shutdown();

    return 0;
}

void LoadC3File() {
    std::string path;
    if (FileDialog::OpenFile("C3 Files (*.c3)\0*.c3\0All Files (*.*)\0*.*\0", path)) {
        g_appState.model = std::make_unique<C3Model>();

        if (g_appState.model->LoadFromFile(path)) {
            if (g_appState.renderer->LoadModel(*g_appState.model)) {
                g_appState.modelLoaded = true;
                g_appState.loadedFilePath = path;

                // Extract filename from path
                size_t lastSlash = path.find_last_of("\\/");
                std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
                g_appState.statusMessage = "Loaded: " + filename;

                // Reset animation state
                g_appState.playAnimation = false;
                g_appState.animationTime = 0.0f;
                g_appState.morphWeights[0] = 1.0f;
                g_appState.morphWeights[1] = 0.0f;
                g_appState.morphWeights[2] = 0.0f;
                g_appState.morphWeights[3] = 0.0f;

                g_appState.renderer->SetMorphWeights(1.0f, 0.0f, 0.0f, 0.0f);
            }
            else {
                g_appState.statusMessage = "ERROR: Failed to upload model to GPU";
                MessageBoxA(nullptr, "Failed to create GPU buffers for model!", "Renderer Error", MB_OK | MB_ICONERROR);
            }
        }
        else {
            g_appState.statusMessage = "ERROR: " + g_appState.model->GetError();
            MessageBoxA(nullptr, g_appState.model->GetError().c_str(), "Parse Error", MB_OK | MB_ICONERROR);
        }
    }
}

void RenderUI() {
    // Main Menu Bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open C3 File...", "Ctrl+O")) {
                LoadC3File();
            }
            if (ImGui::MenuItem("Merge C3 File...", "Ctrl+M")) {
                MergeC3File();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Export")) {
                if (ImGui::MenuItem("Export to C3...")) {
                    ExportC3File();
                }
                if (ImGui::MenuItem("Export to GLTF...")) {
                    ExportToGLTF();
                }
                if (ImGui::MenuItem("Export to OBJ...")) {
                    ExportToOBJ();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Import")) {
                if (ImGui::MenuItem("Import from GLTF...")) {
                    ImportFromGLTF();
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "ESC")) {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Wireframe", nullptr, g_appState.wireframe)) {
                g_appState.wireframe = !g_appState.wireframe;
                if (g_appState.renderer) {
                    g_appState.renderer->SetWireframe(g_appState.wireframe);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Controls")) {
                MessageBoxA(nullptr,
                    "Yamen C3 Tools - Professional Asset Viewer\n\n"
                    "MOUSE CONTROLS:\n"
                    "  Left Click + Drag: Rotate camera\n"
                    "  Middle Click + Drag: Pan camera\n"
                    "  Mouse Wheel: Zoom in/out\n\n"
                    "KEYBOARD SHORTCUTS:\n"
                    "  Ctrl+O: Open C3 file\n"
                    "  ESC: Exit application\n\n"
                    "NOTE: Click on the 3D viewport (right side)\n"
                    "to use mouse controls - not over UI panels!",
                    "Controls & Help", MB_OK | MB_ICONINFORMATION);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Properties Panel
    float menuBarHeight = ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight + 1), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, g_appState.windowHeight - menuBarHeight - 21), ImGuiCond_FirstUseEver);

    ImGui::Begin("Properties", nullptr);

    // Model Info Section
    if (ImGui::CollapsingHeader("Model Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (g_appState.model) {
            ImGui::Text("Type: %s", ChunkTypeToString(g_appState.model->GetType()));
            ImGui::Text("Meshes: %zu", g_appState.model->GetMeshes().size());

            XMFLOAT3 center = g_appState.model->GetCenter();
            ImGui::Text("Center: %.2f, %.2f, %.2f", center.x, center.y, center.z);
            ImGui::Text("Radius: %.2f", g_appState.model->GetRadius());
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "No model loaded");
            ImGui::Text("Press Ctrl+O to open a file");
        }
    }

    // Morph Targets Section
    if (ImGui::CollapsingHeader("Morph Targets", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (g_appState.modelLoaded) {
            ImGui::SliderFloat("Target 0", &g_appState.morphWeights[0], 0.0f, 1.0f);
            ImGui::SliderFloat("Target 1", &g_appState.morphWeights[1], 0.0f, 1.0f);
            ImGui::SliderFloat("Target 2", &g_appState.morphWeights[2], 0.0f, 1.0f);
            ImGui::SliderFloat("Target 3", &g_appState.morphWeights[3], 0.0f, 1.0f);

            if (ImGui::Button("Reset Weights")) {
                g_appState.morphWeights[0] = 1.0f;
                g_appState.morphWeights[1] = 0.0f;
                g_appState.morphWeights[2] = 0.0f;
                g_appState.morphWeights[3] = 0.0f;
            }

            // Apply manual morph weight changes
            if (g_appState.renderer && !g_appState.playAnimation) {
                g_appState.renderer->SetMorphWeights(
                    g_appState.morphWeights[0],
                    g_appState.morphWeights[1],
                    g_appState.morphWeights[2],
                    g_appState.morphWeights[3]
                );
            }
        }
        else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Load a model to adjust weights");
        }
    }

    // Animation Section
    if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (g_appState.modelLoaded && g_appState.model) {
            const auto& anims = g_appState.model->GetAnimations();
            if (!anims.empty()) {
                static int selectedAnim = 0;
                if (ImGui::Combo("Animation", &selectedAnim, [](void* data, int idx, const char** out_text) {
                    const auto* anims = static_cast<const std::vector<C3Model::Animation>*>(data);
                    if (idx >= 0 && idx < anims->size()) {
                        *out_text = (*anims)[idx].name.c_str();
                        return true;
                    }
                    return false;
                }, const_cast<void*>(static_cast<const void*>(&anims)), anims.size())) {
                    g_appState.animationTime = 0.0f;
                }
                
                if (selectedAnim < anims.size()) {
                    const auto& anim = anims[selectedAnim];
                    ImGui::Text("Frames: %u", anim.frameCount);
                    ImGui::Text("Keyframes: %u", anim.keyFrameCount);
                    ImGui::Text("Bones: %u", anim.boneCount);
                    
                    uint32_t currentFrame = static_cast<uint32_t>(g_appState.animationTime) % anim.frameCount;
                    if (ImGui::SliderInt("Frame", reinterpret_cast<int*>(&currentFrame), 0, anim.frameCount - 1)) {
                        g_appState.model->SetAnimationFrame(selectedAnim, currentFrame);
                    }
                }
            }
            
            if (ImGui::Button(g_appState.playAnimation ? "Pause" : "Play")) {
                g_appState.playAnimation = !g_appState.playAnimation;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                g_appState.animationTime = 0.0f;
                g_appState.playAnimation = false;
                g_appState.morphWeights[0] = 1.0f;
                g_appState.morphWeights[1] = 0.0f;
                g_appState.morphWeights[2] = 0.0f;
                g_appState.morphWeights[3] = 0.0f;
            }

            ImGui::SliderFloat("Speed", &g_appState.animationSpeed, 0.1f, 5.0f);
        }
        else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Load a model to animate");
        }
    }

    // Camera Section
    if (ImGui::CollapsingHeader("Camera")) {
        if (g_appState.camera) {
            XMFLOAT3 pos = g_appState.camera->GetPosition();
            ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);

            if (ImGui::Button("Reset Camera")) {
                g_appState.camera->Reset();
            }
        }
    }

    ImGui::End();

    // Status Bar
    ImGui::SetNextWindowPos(ImVec2(0, g_appState.windowHeight - 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(g_appState.windowWidth, 20), ImGuiCond_Always);

    ImGui::Begin("##StatusBar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("%s", g_appState.statusMessage.c_str());
    ImGui::End();
}

void MergeC3File() {
    if (!g_appState.model) {
        MessageBoxA(nullptr, "Please load a C3 file first before merging!", "No Model", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string path;
    if (FileDialog::OpenFile("C3 Files (*.c3)\0*.c3\0All Files (*.*)\0*.*\0", path)) {
        if (g_appState.model->MergeFromFile(path)) {
            // Reload model to GPU
            if (g_appState.renderer->LoadModel(*g_appState.model)) {
                size_t lastSlash = path.find_last_of("\\/");
                std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
                g_appState.statusMessage = "Merged: " + filename;
            } else {
                g_appState.statusMessage = "ERROR: Failed to upload merged model to GPU";
            }
        } else {
            g_appState.statusMessage = "ERROR: " + g_appState.model->GetError();
            MessageBoxA(nullptr, g_appState.model->GetError().c_str(), "Merge Error", MB_OK | MB_ICONERROR);
        }
    }
}

void ExportC3File() {
    if (!g_appState.model || !g_appState.modelLoaded) {
        MessageBoxA(nullptr, "No model loaded to export!", "Export Error", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string path;
    if (FileDialog::SaveFile("C3 Files (*.c3)\0*.c3\0All Files (*.*)\0*.*\0", "c3", path)) {
        if (!path.empty()) {
            if (path.find(".c3") == std::string::npos) {
                path += ".c3";
            }

            if (g_appState.c3Writer->Write(*g_appState.model, path)) {
                g_appState.statusMessage = "Exported to: " + path;
            } else {
                g_appState.statusMessage = "ERROR: " + g_appState.c3Writer->GetLastError();
                MessageBoxA(nullptr, g_appState.c3Writer->GetLastError().c_str(), "Export Error", MB_OK | MB_ICONERROR);
            }
        }
    }
}

void ExportToGLTF() {
    if (!g_appState.model || !g_appState.modelLoaded) {
        MessageBoxA(nullptr, "No model loaded to export!", "Export Error", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string path;
    if (FileDialog::SaveFile("GLTF Files (*.gltf)\0*.gltf\0All Files (*.*)\0*.*\0", "gltf", path)) {
        if (!path.empty()) {
            if (path.find(".gltf") == std::string::npos) {
                path += ".gltf";
            }

            C3ToGLTF::ExportOptions options;
            options.outputPath = path;
            options.exportMorphTargets = true;
            options.exportVertexColors = true;

            if (g_appState.gltfExporter->Export(*g_appState.model, options)) {
                g_appState.statusMessage = "Exported GLTF to: " + path;
            } else {
                g_appState.statusMessage = "ERROR: " + g_appState.gltfExporter->GetLastError();
                MessageBoxA(nullptr, g_appState.gltfExporter->GetLastError().c_str(), "Export Error", MB_OK | MB_ICONERROR);
            }
        }
    }
}

void ExportToOBJ() {
    if (!g_appState.model || !g_appState.modelLoaded) {
        MessageBoxA(nullptr, "No model loaded to export!", "Export Error", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string path;
    if (FileDialog::SaveFile("OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0", "obj", path)) {
        if (!path.empty()) {
            if (path.find(".obj") == std::string::npos) {
                path += ".obj";
            }

            C3Exporter::ExportOptions options;
            options.outputPath = path;
            options.exportMorphTargets = false;
            options.exportVertexColors = false;

            if (g_appState.objExporter->Export(*g_appState.model, options)) {
                g_appState.statusMessage = "Exported OBJ to: " + path;
            } else {
                g_appState.statusMessage = "ERROR: " + g_appState.objExporter->GetLastError();
                MessageBoxA(nullptr, g_appState.objExporter->GetLastError().c_str(), "Export Error", MB_OK | MB_ICONERROR);
            }
        }
    }
}

void ImportFromGLTF() {
    std::string path;
    if (FileDialog::OpenFile("GLTF Files (*.gltf)\0*.gltf\0All Files (*.*)\0*.*\0", path)) {
        g_appState.model = std::make_unique<C3Model>();

        GLTFToC3::ImportOptions options;
        options.inputPath = path;
        options.preserveMorphTargets = true;
        options.importVertexColors = true;

        if (g_appState.gltfImporter->Import(path, *g_appState.model, options)) {
            if (g_appState.renderer->LoadModel(*g_appState.model)) {
                g_appState.modelLoaded = true;
                g_appState.loadedFilePath = path;

                size_t lastSlash = path.find_last_of("\\/");
                std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
                g_appState.statusMessage = "Imported: " + filename;

                // Reset animation state
                g_appState.playAnimation = false;
                g_appState.animationTime = 0.0f;
                g_appState.morphWeights[0] = 1.0f;
                g_appState.morphWeights[1] = 0.0f;
                g_appState.morphWeights[2] = 0.0f;
                g_appState.morphWeights[3] = 0.0f;

                g_appState.renderer->SetMorphWeights(1.0f, 0.0f, 0.0f, 0.0f);
            } else {
                g_appState.statusMessage = "ERROR: Failed to upload imported model to GPU";
                MessageBoxA(nullptr, "Failed to create GPU buffers for imported model!", "Renderer Error", MB_OK | MB_ICONERROR);
            }
        } else {
            g_appState.statusMessage = "ERROR: " + g_appState.gltfImporter->GetLastError();
            MessageBoxA(nullptr, g_appState.gltfImporter->GetLastError().c_str(), "Import Error", MB_OK | MB_ICONERROR);
        }
    }
}