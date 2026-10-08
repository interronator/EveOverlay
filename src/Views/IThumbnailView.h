#pragma once

#include <functional>
#include <string>

#include <Windows.h>

#include "Config/Color.h"
#include "Config/Geometry.h"
#include "Config/Hotkey.h"
#include "Config/ZoomAnchor.h"

class IThumbnailView
{
public:
    IThumbnailView() = default;
    IThumbnailView(const IThumbnailView&) = delete;
    IThumbnailView& operator=(const IThumbnailView&) = delete;
    virtual ~IThumbnailView() = default;

    std::function<void(HWND)> ThumbnailResized;
    std::function<void(HWND)> ThumbnailMoved;
    std::function<void(HWND)> ThumbnailFocused;
    std::function<void(HWND)> ThumbnailLostFocus;
    std::function<void(HWND)> ThumbnailActivated;
    // Second argument: true when the user asked to switch out to the previous non-EVE window
    std::function<void(HWND, bool)> ThumbnailDeactivated;

    // Handle of the EVE client window this thumbnail shows
    virtual HWND GetId() const = 0;
    virtual const std::wstring& GetTitle() const = 0;
    virtual void SetTitle(const std::wstring& NewTitle) = 0;
    virtual bool IsActive() const = 0;

    virtual void SetOverlayEnabled(bool Enabled) = 0;
    virtual Point GetThumbnailLocation() const = 0;
    virtual void SetThumbnailLocation(Point Location) = 0;
    virtual Size GetThumbnailSize() const = 0;
    virtual void SetThumbnailSize(Size NewSize) = 0;

    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual void Close() = 0;

    // True for the client window, the thumbnail window or its overlay
    virtual bool IsKnownHandle(HWND Window) const = 0;

    virtual void SetSizeLimitations(Size Minimum, Size Maximum) = 0;
    virtual void SetOpacity(double Opacity) = 0;
    virtual void SetFrames(bool Enable) = 0;
    virtual void SetTopMost(bool EnableTopMost) = 0;

    // A locked preview cannot be moved or resized with the mouse
    virtual void SetLocked(bool Locked) = 0;
    virtual void SetHighlight(bool Enabled, Color HighlightColor, int Width) = 0;
    virtual void ZoomIn(ZoomAnchor Anchor, int ZoomFactor) = 0;
    virtual void ZoomOut() = 0;
    virtual void RegisterHotkey(const Hotkey& Key) = 0;
    virtual void UnregisterHotkey() = 0;
    virtual void Refresh(bool ForceRefresh) = 0;
};
