#pragma once

#include <string>

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

#include "Config/Geometry.h"

// A click-through window owned by the thumbnail. A DWM thumbnail always paints above the thumbnail window's own
// content, so the title text has to live in a separate window that sits above it.
class ThumbnailOverlay : public CWindowImpl<ThumbnailOverlay>
{
public:
    DECLARE_WND_CLASS_EX(L"EveOverlayThumbnailOverlay", 0, 0)

    BEGIN_MSG_MAP(ThumbnailOverlay)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
    END_MSG_MAP()

    ThumbnailOverlay() = default;
    ~ThumbnailOverlay();

    void Initialize(const HWND OwnerWindow);
    void SetLabel(std::wstring Text);
    void EnableLabel(const bool Enabled);
    void SetOpacity(const double Opacity);
    void ShowOverlay();
    void HideOverlay();
    void SetBounds(const Point Location, const Size NewSize);
    void SetTopMost(const bool EnableTopMost);
    void Refresh();

private:
    static constexpr COLORREF TRANSPARENT_KEY = RGB(0, 0, 0);
    static constexpr COLORREF LABEL_COLOR = RGB(0xA9, 0xA9, 0xA9);
    static constexpr int LABEL_OFFSET = 8;

    LRESULT OnEraseBackground(UINT, WPARAM Parameter, LPARAM, BOOL&);
    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMouseActivate(UINT, WPARAM, LPARAM, BOOL&);

    void ApplyLayeredAttributes();

    // Grayscale antialiasing: ClearType fringes would not match the color key
    void RebuildFont();

    void ReleaseFont();

    std::wstring Label;
    bool LabelEnabled = false;
    BYTE OverlayAlpha = 255;
    HFONT LabelFont = nullptr;
};
