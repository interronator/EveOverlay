#include "Services/DwmThumbnail.h"

#include "Application/Logger.h"

DwmThumbnail::DwmThumbnail(const IWindowManager& WindowManagerReference)
    : WindowManagerInstance(WindowManagerReference)
{
}

DwmThumbnail::~DwmThumbnail()
{
    Unregister();
}

void DwmThumbnail::Register(const HWND Destination, const HWND Source)
{
    Properties = {};
    Properties.dwFlags = DWM_TNP_VISIBLE | DWM_TNP_OPACITY | DWM_TNP_RECTDESTINATION | DWM_TNP_SOURCECLIENTAREAONLY;
    Properties.opacity = 255;
    Properties.fVisible = TRUE;
    Properties.fSourceClientAreaOnly = TRUE;

    if (WindowManagerInstance.IsCompositionEnabled() == false)
    {
        return;
    }

    // Fails when the source window is already gone or DWM is momentarily unavailable
    HTHUMBNAIL NewHandle = nullptr;
    const HRESULT RegisterResult = DwmApi::RegisterThumbnail(Destination, Source, &NewHandle);
    if (FAILED(RegisterResult) == true)
    {
        Logger::Warning("DwmRegisterThumbnail failed, HRESULT " + std::to_string(static_cast<long>(RegisterResult)));
        ThumbnailHandle = nullptr;
        return;
    }

    ThumbnailHandle = NewHandle;
}

void DwmThumbnail::Unregister()
{
    if (ThumbnailHandle == nullptr)
    {
        return;
    }

    DwmApi::UnregisterThumbnail(ThumbnailHandle);
    ThumbnailHandle = nullptr;
}

void DwmThumbnail::Move(const int Left, const int Top, const int Right, const int Bottom)
{
    Properties.rcDestination = RECT{Left, Top, Right, Bottom};
}

void DwmThumbnail::Update()
{
    if (ThumbnailHandle == nullptr)
    {
        return;
    }

    DwmApi::UpdateThumbnailProperties(ThumbnailHandle, Properties);
}

bool DwmThumbnail::IsRegistered() const
{
    return ThumbnailHandle != nullptr;
}
