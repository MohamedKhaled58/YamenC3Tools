#pragma once
#include <d3d11.h>
#include <Windows.h>

class ImGuiManager {
public:
    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();
    void NewFrame();
    void Render();

    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;

private:
    bool m_initialized = false;
};