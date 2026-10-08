#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM Parameter, LPARAM Argument);

// Owns the D3D11 device, the swap chain of the settings window and the ImGui context
class GuiRenderer
{
public:
    GuiRenderer() = default;
    GuiRenderer(const GuiRenderer&) = delete;
    GuiRenderer& operator=(const GuiRenderer&) = delete;

    ~GuiRenderer();

    bool Initialize(const HWND Window, const float Scale);
    void Shutdown();
    bool IsReady() const;
    void SetScale(const float Scale);
    void Resize(const UINT Width, const UINT Height);
    void BeginFrame();
    void EndFrame();

private:
    static constexpr float BASE_FONT_SIZE = 15.0f;

    template <typename Interface>
    static void Release(Interface*& Target)
    {
        if (Target == nullptr)
        {
            return;
        }

        Target->Release();
        Target = nullptr;
    }

    static void LoadFonts();

    bool CreateDevice(const HWND Window);
    void CreateTarget();
    void ReleaseTarget();

    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* Context = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
    ID3D11RenderTargetView* RenderTarget = nullptr;
    bool BackendReady = false;
};
