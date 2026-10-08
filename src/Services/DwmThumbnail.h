#pragma once

#include "Services/IDwmThumbnail.h"
#include "Services/IWindowManager.h"
#include "Services/Interop/DwmApi.h"

class DwmThumbnail : public IDwmThumbnail
{
public:
    explicit DwmThumbnail(const IWindowManager& WindowManagerReference);

    DwmThumbnail(const DwmThumbnail&) = delete;
    DwmThumbnail& operator=(const DwmThumbnail&) = delete;

    ~DwmThumbnail() override;

    void Register(const HWND Destination, const HWND Source) override;
    void Unregister() override;
    void Move(const int Left, const int Top, const int Right, const int Bottom) override;
    void Update() override;
    bool IsRegistered() const override;

private:
    const IWindowManager& WindowManagerInstance;
    HTHUMBNAIL ThumbnailHandle = nullptr;
    DWM_THUMBNAIL_PROPERTIES Properties = {};
};
