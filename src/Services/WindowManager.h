#pragma once

#include <memory>

#include "Services/DwmThumbnail.h"
#include "Services/IWindowManager.h"

class WindowManager : public IWindowManager
{
public:
    WindowManager();

    bool IsCompositionEnabled() const override;
    HWND GetForegroundWindowHandle() const override;
    void ActivateWindow(const HWND Window) const override;
    void MinimizeWindow(const HWND Window, const bool EnableAnimation) const override;
    void MoveWindow(const HWND Window, const int Left, const int Top, const int Width, const int Height) const override;
    void MaximizeWindow(const HWND Window) const override;
    RECT GetWorkArea(const POINT Location) const override;
    RECT GetWindowPosition(const HWND Window) const override;
    bool IsWindowMaximized(const HWND Window) const override;
    bool IsWindowMinimized(const HWND Window) const override;
    std::unique_ptr<IDwmThumbnail> GetLiveThumbnail(const HWND Destination, const HWND Source) const override;
    HBITMAP GetStaticThumbnail(const HWND Source) const override;

private:
    static constexpr int WINDOW_SIZE_THRESHOLD = 300;

    const bool CompositionEnabled;
};
