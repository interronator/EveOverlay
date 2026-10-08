#pragma once

#include <memory>
#include <string>

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

#include "Services/IWindowManager.h"
#include "Views/HotkeyHandler.h"
#include "Views/IThumbnailView.h"
#include "Views/ThumbnailOverlay.h"

// Base of the live and static thumbnail windows. Call Initialize() once after construction.
class ThumbnailView : public CWindowImpl<ThumbnailView>, public IThumbnailView
{
public:
    DECLARE_WND_CLASS_EX(L"EveOverlayThumbnail", 0, 0)

    BEGIN_MSG_MAP(ThumbnailView)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_MOVE, OnMove)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLeftButtonDown)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRightButtonDown)
        MESSAGE_HANDLER(WM_RBUTTONUP, OnRightButtonUp)
        MESSAGE_HANDLER(WM_HOTKEY, OnHotkey)
    END_MSG_MAP()

    explicit ThumbnailView(const IWindowManager& WindowManagerReference);
    ~ThumbnailView() override;

    void Initialize(const HWND ClientWindow, const std::wstring& InitialTitle, const Size InitialSize);

    HWND GetId() const override;
    const std::wstring& GetTitle() const override;
    void SetTitle(const std::wstring& NewTitle) override;
    bool IsActive() const override;

    void SetOverlayEnabled(const bool Enabled) override;
    Point GetThumbnailLocation() const override;
    void SetThumbnailLocation(const Point Location) override;
    Size GetThumbnailSize() const override;
    void SetThumbnailSize(const Size NewSize) override;

    void Show() override;
    void Hide() override;
    void Close() override;

    bool IsKnownHandle(const HWND Window) const override;

    void SetSizeLimitations(const Size Minimum, const Size Maximum) override;
    void SetOpacity(double Opacity) override;
    void SetFrames(const bool Enable) override;
    void SetTopMost(const bool EnableTopMost) override;
    void SetLocked(const bool Locked) override;
    void SetHighlight(const bool Enabled, const Color HighlightColor, const int Width) override;
    void ZoomIn(const ZoomAnchor Anchor, const int ZoomFactor) override;
    void ZoomOut() override;
    void RegisterHotkey(const Hotkey& Key) override;
    void UnregisterHotkey() override;
    void Refresh(const bool ForceRefresh) override;

protected:
    virtual void RefreshThumbnail(bool ForceRefresh) = 0;

    // Destination rectangle of the thumbnail content: the client area minus the highlight insets
    virtual void ResizeThumbnail(int BaseWidth, int BaseHeight, int HighlightTop, int HighlightRight, int HighlightBottom, int HighlightLeft) = 0;

    virtual void PaintContent(HDC);

    const IWindowManager& GetWindowManager() const;

private:
    static constexpr int RESIZE_EVENT_TIMEOUT_MS = 500;
    static constexpr double OPACITY_THRESHOLD = 0.9;
    static constexpr double OPACITY_EPSILON = 0.1;
    static constexpr ULONGLONG TOPMOST_RETRY_DELAY_MS = 5000;

    static BYTE ToAlpha(const double Opacity);

    void ApplyTopMost(const bool EnableTopMost);

    Size GetOuterSize() const;
    void SetOuterSize(const Size NewSize);

    // Re-sets the current size so the new limits clamp it, like assigning Form.MinimumSize/MaximumSize does
    void ReapplyTrackSizeLimits();

    void SetBackgroundColor(const COLORREF NewColor);
    void ReleaseBackgroundBrush();

    // Ignores the Resize events that the OS fires with inconsistent sizes around show, hide and border changes
    void SuppressResizeEvent();

    void HighlightThumbnail(const bool ForceRefresh);
    void RefreshOverlay(const bool ForceRefresh);

    LRESULT OnEraseBackground(UINT, WPARAM Parameter, LPARAM, BOOL&);
    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMove(UINT, WPARAM, LPARAM, BOOL& Handled);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL& Handled);
    LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM Parameter, BOOL&);
    LRESULT OnMouseActivate(UINT, WPARAM, LPARAM, BOOL&);

    // Thumbnail sizes are configured in pixels, so moving between monitors of different DPI must not rescale the window
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL&);

    LRESULT OnMouseMove(UINT, WPARAM Keys, LPARAM, BOOL&);
    LRESULT OnMouseLeave(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnLeftButtonDown(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnRightButtonDown(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnRightButtonUp(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnHotkey(UINT, WPARAM HotkeyId, LPARAM, BOOL&);

    void RaiseDeactivated(const bool SwitchOut);
    void HandleMouseEnter();

    // Saved so the zoom effect can be undone, and so the custom move/resize starts from the unzoomed window
    void SaveWindowSizeAndLocation();

    void RestoreWindowSizeAndLocation();
    void EnterCustomMouseMode();

    // Right button moves the thumbnail, left + right resizes it
    void ProcessCustomMouseMode(const bool LeftButton, const bool RightButton);

    void ExitCustomMouseMode();

    const IWindowManager& WindowManagerInstance;
    ThumbnailOverlay Overlay;
    std::unique_ptr<HotkeyHandler> HotkeyBinding;

    HWND ClientHandle = nullptr;
    std::wstring TitleText;

    bool Active = false;
    bool OverlayEnabled = false;
    bool OverlayVisible = false;
    bool TopMost = false;
    bool TopMostWarned = false;
    ULONGLONG NextTopMostRetryTick = 0;
    bool LockedInPlace = false;
    bool HighlightEnabled = false;
    bool HighlightRequested = false;
    int HighlightWidth = 0;
    Color HighlightColor;
    bool LocationChanged = true;
    bool SizeChanged = true;
    bool CustomMouseModeActive = false;
    bool MouseTracking = false;
    double CurrentOpacity = 0.1;

    COLORREF BackgroundColor = RGB(0, 0, 0);
    HBRUSH BackgroundBrush = nullptr;

    Size MinimumSize{64, 64};
    Size MaximumSize{0, 0};
    ULONGLONG SuppressResizeEventsUntil = 0;
    Size BaseZoomSize;
    Point BaseZoomLocation;
    Size BaseZoomMaximumSize;
    POINT BaseMousePosition = {};
};
