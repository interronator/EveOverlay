#include "Views/ThumbnailOverlay.h"

#include <cmath>
#include <utility>

ThumbnailOverlay::~ThumbnailOverlay()
{
    ReleaseFont();
}

void ThumbnailOverlay::Initialize(const HWND OwnerWindow)
{
    Create(OwnerWindow, CWindow::rcDefault, L"PreviewOverlay", WS_POPUP, WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    ApplyLayeredAttributes();
    RebuildFont();
}

void ThumbnailOverlay::SetLabel(std::wstring Text)
{
    Label = std::move(Text);
    Invalidate();
}

void ThumbnailOverlay::EnableLabel(const bool Enabled)
{
    LabelEnabled = Enabled;
    Invalidate();
}

void ThumbnailOverlay::SetOpacity(const double Opacity)
{
    OverlayAlpha = static_cast<BYTE>(std::lround(Opacity * 255.0));
    ApplyLayeredAttributes();
}

void ThumbnailOverlay::ShowOverlay()
{
    ShowWindow(SW_SHOWNOACTIVATE);
}

void ThumbnailOverlay::HideOverlay()
{
    ShowWindow(SW_HIDE);
}

void ThumbnailOverlay::SetBounds(const Point Location, const Size NewSize)
{
    SetWindowPos(nullptr, Location.X, Location.Y, NewSize.Width, NewSize.Height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void ThumbnailOverlay::SetTopMost(const bool EnableTopMost)
{
    SetWindowPos(EnableTopMost == true ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void ThumbnailOverlay::Refresh()
{
    Invalidate();
    UpdateWindow();
}

LRESULT ThumbnailOverlay::OnEraseBackground(UINT, WPARAM Parameter, LPARAM, BOOL&)
{
    RECT Client = {};
    GetClientRect(&Client);
    ::FillRect(reinterpret_cast<HDC>(Parameter), &Client, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
    return 1;
}

LRESULT ThumbnailOverlay::OnPaint(UINT, WPARAM, LPARAM, BOOL&)
{
    PAINTSTRUCT Paint = {};
    const HDC DeviceContext = BeginPaint(&Paint);

    if (LabelEnabled == true && Label.empty() == false)
    {
        const int Offset = ::MulDiv(LABEL_OFFSET, static_cast<int>(::GetDpiForWindow(m_hWnd)), 96);
        const HGDIOBJ PreviousFont = ::SelectObject(DeviceContext, LabelFont);
        ::SetBkMode(DeviceContext, TRANSPARENT);
        ::SetTextColor(DeviceContext, LABEL_COLOR);
        ::TextOutW(DeviceContext, Offset, Offset, Label.c_str(), static_cast<int>(Label.size()));
        ::SelectObject(DeviceContext, PreviousFont);
    }

    EndPaint(&Paint);
    return 0;
}

LRESULT ThumbnailOverlay::OnMouseActivate(UINT, WPARAM, LPARAM, BOOL&)
{
    return MA_NOACTIVATE;
}

void ThumbnailOverlay::ApplyLayeredAttributes()
{
    ::SetLayeredWindowAttributes(m_hWnd, TRANSPARENT_KEY, OverlayAlpha, LWA_COLORKEY | LWA_ALPHA);
}

void ThumbnailOverlay::RebuildFont()
{
    ReleaseFont();

    const int Dpi = static_cast<int>(::GetDpiForWindow(m_hWnd));
    LOGFONTW Description = {};
    Description.lfHeight = -::MulDiv(825, Dpi, 7200);
    Description.lfWeight = FW_NORMAL;
    Description.lfCharSet = DEFAULT_CHARSET;
    Description.lfQuality = ANTIALIASED_QUALITY;
    ::wcscpy_s(Description.lfFaceName, L"Consolas");
    LabelFont = ::CreateFontIndirectW(&Description);
}

void ThumbnailOverlay::ReleaseFont()
{
    if (LabelFont == nullptr)
    {
        return;
    }

    ::DeleteObject(LabelFont);
    LabelFont = nullptr;
}
