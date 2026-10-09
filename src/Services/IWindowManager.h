#pragma once

#include <memory>

#include <Windows.h>

#include "Services/IDwmThumbnail.h"

class IWindowManager
{
public:
    virtual ~IWindowManager() = default;

    virtual bool IsCompositionEnabled() const = 0;

    virtual HWND GetForegroundWindowHandle() const = 0;
    virtual void ActivateWindow(HWND Window) const = 0;
    virtual void MinimizeWindow(HWND Window, bool EnableAnimation) const = 0;
    virtual void MoveWindow(HWND Window, int Left, int Top, int Width, int Height) const = 0;
    virtual void MaximizeWindow(HWND Window) const = 0;
    virtual RECT GetWindowPosition(HWND Window) const = 0;

    // The usable area of the monitor nearest to the point; empty when it cannot be found
    virtual RECT GetWorkArea(POINT Location) const = 0;

    virtual bool IsWindowMaximized(HWND Window) const = 0;
    virtual bool IsWindowMinimized(HWND Window) const = 0;

    virtual std::unique_ptr<IDwmThumbnail> GetLiveThumbnail(HWND Destination, HWND Source) const = 0;

    // Returns an HBITMAP owned by the caller (DeleteObject), or nullptr if there is nothing to capture.
    virtual HBITMAP GetStaticThumbnail(HWND Source) const = 0;
};
