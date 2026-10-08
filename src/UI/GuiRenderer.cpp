#include "UI/GuiRenderer.h"

#include <algorithm>

#include "UI/Theme.h"

GuiRenderer::~GuiRenderer()
{
    Shutdown();
}

bool GuiRenderer::Initialize(const HWND Window, const float Scale)
{
    if (CreateDevice(Window) == false)
    {
        Shutdown();
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& Io = ImGui::GetIO();
    Io.IniFilename = nullptr;
    Io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    LoadFonts();
    Theme::Apply(Scale);

    ImGui_ImplWin32_Init(Window);
    ImGui_ImplDX11_Init(Device, Context);
    BackendReady = true;
    return true;
}

void GuiRenderer::Shutdown()
{
    if (BackendReady == true)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        BackendReady = false;
    }

    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGui::DestroyContext();
    }

    ReleaseTarget();
    Release(SwapChain);
    Release(Context);
    Release(Device);
}

bool GuiRenderer::IsReady() const
{
    return BackendReady;
}

void GuiRenderer::SetScale(const float Scale)
{
    Theme::Apply(Scale);
}

void GuiRenderer::Resize(const UINT Width, const UINT Height)
{
    if (SwapChain == nullptr || Width == 0 || Height == 0)
    {
        return;
    }

    ReleaseTarget();
    SwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);
    CreateTarget();
}

void GuiRenderer::BeginFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void GuiRenderer::EndFrame()
{
    ImGui::Render();

    const ImVec4& Clear = Theme::BACKGROUND;
    const float ClearColor[4] = {Clear.x, Clear.y, Clear.z, 1.0f};
    Context->OMSetRenderTargets(1, &RenderTarget, nullptr);
    Context->ClearRenderTargetView(RenderTarget, ClearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    SwapChain->Present(1, 0);
}

bool GuiRenderer::CreateDevice(const HWND Window)
{
    RECT Client = {};
    ::GetClientRect(Window, &Client);

    DXGI_SWAP_CHAIN_DESC Description = {};
    Description.BufferCount = 2;
    Description.BufferDesc.Width = static_cast<UINT>(std::max<LONG>(Client.right, 1));
    Description.BufferDesc.Height = static_cast<UINT>(std::max<LONG>(Client.bottom, 1));
    Description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Description.OutputWindow = Window;
    Description.SampleDesc.Count = 1;
    Description.Windowed = TRUE;
    Description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    const D3D_FEATURE_LEVEL Levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL Created = D3D_FEATURE_LEVEL_10_0;

    HRESULT Result = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, Levels, 2, D3D11_SDK_VERSION, &Description, &SwapChain, &Device, &Created, &Context);
    if (Result == DXGI_ERROR_UNSUPPORTED)
    {
        Result = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, Levels, 2, D3D11_SDK_VERSION, &Description, &SwapChain, &Device, &Created, &Context);
    }

    if (FAILED(Result) == true)
    {
        return false;
    }

    CreateTarget();
    return RenderTarget != nullptr;
}

void GuiRenderer::CreateTarget()
{
    ID3D11Texture2D* BackBuffer = nullptr;
    if (FAILED(SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer))) == true)
    {
        return;
    }

    Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTarget);
    BackBuffer->Release();
}

void GuiRenderer::ReleaseTarget()
{
    Release(RenderTarget);
}

void GuiRenderer::LoadFonts()
{
    ImGuiIO& Io = ImGui::GetIO();
    ImFont* const Regular = Io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", BASE_FONT_SIZE);
    ImFont* const Bold = Io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", BASE_FONT_SIZE);

    ImFont* const FallbackRegular = Regular != nullptr ? Regular : Io.Fonts->AddFontDefault();
    ImFont* const FallbackBold = Bold != nullptr ? Bold : FallbackRegular;
    Theme::SetFonts(FallbackRegular, FallbackBold);
}
