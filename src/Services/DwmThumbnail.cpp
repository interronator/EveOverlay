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
    DestinationWindow = Destination;
    SourceWindow = Source;
    LastSourceSize = {};
    BadUpdates = 0;
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

    const HRESULT UpdateResult = DwmApi::UpdateThumbnailProperties(ThumbnailHandle, Properties);

    SIZE SourceSize = {};
    const HRESULT QueryResult = DwmApi::QuerySourceSize(ThumbnailHandle, &SourceSize);

    // A thumbnail that DWM keeps rejecting or that keeps having no source picture is dropped so the next refresh registers a fresh one;
    // a single bad reading is common while a client is loading or resizing and is given a few refreshes to pass
    if (FAILED(UpdateResult) == true || FAILED(QueryResult) == true || SourceSize.cx <= 0 || SourceSize.cy <= 0)
    {
        BadUpdates++;
        if (BadUpdates >= MAX_BAD_UPDATES)
        {
            Unregister();
        }

        return;
    }

    BadUpdates = 0;

    // A client that resized after registering (e.g. finished loading) gets a fresh thumbnail, swapped in place so the picture does not go blank
    const bool SizeChanged = LastSourceSize.cx != 0 && (LastSourceSize.cx != SourceSize.cx || LastSourceSize.cy != SourceSize.cy);
    LastSourceSize = SourceSize;
    if (SizeChanged == false)
    {
        return;
    }

    HTHUMBNAIL NewHandle = nullptr;
    if (FAILED(DwmApi::RegisterThumbnail(DestinationWindow, SourceWindow, &NewHandle)) == true)
    {
        Unregister();
        return;
    }

    DwmApi::UnregisterThumbnail(ThumbnailHandle);
    ThumbnailHandle = NewHandle;
    if (FAILED(DwmApi::UpdateThumbnailProperties(ThumbnailHandle, Properties)) == true)
    {
        Unregister();
    }
}

bool DwmThumbnail::IsRegistered() const
{
    return ThumbnailHandle != nullptr;
}
