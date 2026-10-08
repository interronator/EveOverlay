#include "Views/ThumbnailView.h"

#include <algorithm>
#include <cmath>

#include "Application/Logger.h"

ThumbnailView::ThumbnailView(const IWindowManager& WindowManagerReference)
    : WindowManagerInstance(WindowManagerReference)
{
}

ThumbnailView::~ThumbnailView()
{
    ReleaseBackgroundBrush();
}

void ThumbnailView::Initialize(const HWND ClientWindow, const std::wstring& InitialTitle, const Size InitialSize)
{
    SuppressResizeEvent();

    ClientHandle = ClientWindow;
    TitleText = InitialTitle;
    BackgroundBrush = ::CreateSolidBrush(BackgroundColor);

    Create(nullptr, CWindow::rcDefault, InitialTitle.c_str(), WS_POPUP | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE);
    ::SetLayeredWindowAttributes(m_hWnd, 0, ToAlpha(CurrentOpacity), LWA_ALPHA);

    Overlay.Initialize(m_hWnd);
    Overlay.SetLabel(InitialTitle);

    SetThumbnailSize(InitialSize);
}

HWND ThumbnailView::GetId() const
{
    return ClientHandle;
}

const std::wstring& ThumbnailView::GetTitle() const
{
    return TitleText;
}

void ThumbnailView::SetTitle(const std::wstring& NewTitle)
{
    TitleText = NewTitle;
    SetWindowText(NewTitle.c_str());
    Overlay.SetLabel(NewTitle);
}

bool ThumbnailView::IsActive() const
{
    return Active;
}

void ThumbnailView::SetOverlayEnabled(const bool Enabled)
{
    OverlayEnabled = Enabled;
}

Point ThumbnailView::GetThumbnailLocation() const
{
    RECT Bounds = {};
    GetWindowRect(&Bounds);
    return Point{Bounds.left, Bounds.top};
}

void ThumbnailView::SetThumbnailLocation(const Point Location)
{
    const Point Current = GetThumbnailLocation();
    if (Current.X == Location.X && Current.Y == Location.Y)
    {
        return;
    }

    SetWindowPos(nullptr, Location.X, Location.Y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

Size ThumbnailView::GetThumbnailSize() const
{
    RECT Client = {};
    GetClientRect(&Client);
    return Size{Client.right - Client.left, Client.bottom - Client.top};
}

void ThumbnailView::SetThumbnailSize(const Size NewSize)
{
    RECT Outer = {0, 0, NewSize.Width, NewSize.Height};
    ::AdjustWindowRectExForDpi(&Outer, static_cast<DWORD>(GetStyle()), FALSE, static_cast<DWORD>(GetExStyle()), ::GetDpiForWindow(m_hWnd));
    SetWindowPos(nullptr, 0, 0, Outer.right - Outer.left, Outer.bottom - Outer.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void ThumbnailView::Show()
{
    SuppressResizeEvent();

    ShowWindow(SW_SHOWNOACTIVATE);

    LocationChanged = true;
    SizeChanged = true;
    OverlayVisible = false;

    Refresh(true);

    Active = true;
}

void ThumbnailView::Hide()
{
    SuppressResizeEvent();

    Active = false;

    Overlay.HideOverlay();
    ShowWindow(SW_HIDE);
}

void ThumbnailView::Close()
{
    SuppressResizeEvent();

    Active = false;
    UnregisterHotkey();

    if (Overlay.m_hWnd != nullptr)
    {
        Overlay.DestroyWindow();
    }

    if (m_hWnd != nullptr)
    {
        DestroyWindow();
    }
}

bool ThumbnailView::IsKnownHandle(const HWND Window) const
{
    return ClientHandle == Window || m_hWnd == Window || Overlay.m_hWnd == Window;
}

void ThumbnailView::SetSizeLimitations(const Size Minimum, const Size Maximum)
{
    MinimumSize = Minimum;
    MaximumSize = Maximum;
    ReapplyTrackSizeLimits();
}

void ThumbnailView::SetOpacity(double Opacity)
{
    if (Opacity >= OPACITY_THRESHOLD)
    {
        Opacity = 1.0;
    }

    if (std::abs(Opacity - CurrentOpacity) < OPACITY_EPSILON)
    {
        return;
    }

    if (::SetLayeredWindowAttributes(m_hWnd, 0, ToAlpha(Opacity), LWA_ALPHA) == FALSE)
    {
        return;
    }

    // Overlay stays fully opaque for a nearly opaque thumbnail, otherwise it is halfway between the two
    Overlay.SetOpacity(Opacity > 0.8 ? 1.0 : 1.0 - (1.0 - Opacity) / 2.0);
    CurrentOpacity = Opacity;
}

void ThumbnailView::SetFrames(const bool Enable)
{
    const LONG_PTR Style = ::GetWindowLongPtrW(m_hWnd, GWL_STYLE);
    const LONG_PTR FrameStyle = WS_CAPTION | WS_THICKFRAME;
    const bool HasFrames = (Style & WS_CAPTION) == WS_CAPTION;
    if (HasFrames == Enable)
    {
        return;
    }

    SuppressResizeEvent();

    // The thumbnail size is the client size, so it survives the border change
    const Size Client = GetThumbnailSize();
    ::SetWindowLongPtrW(m_hWnd, GWL_STYLE, Enable == true ? (Style | FrameStyle) : (Style & ~FrameStyle));
    SetWindowPos(nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    SetThumbnailSize(Client);
}

void ThumbnailView::SetTopMost(const bool EnableTopMost)
{
    // Windows can drop the topmost flag behind our back, which hides the picture under the game while the owned label stays visible
    const bool ActuallyTopMost = m_hWnd != nullptr && (GetExStyle() & WS_EX_TOPMOST) != 0;
    if (TopMost == EnableTopMost && ActuallyTopMost == EnableTopMost)
    {
        return;
    }

    if (TopMost == EnableTopMost && ::GetTickCount64() < NextTopMostRetryTick)
    {
        return;
    }

    Overlay.SetTopMost(EnableTopMost);
    ApplyTopMost(EnableTopMost);

    const bool Applied = ((GetExStyle() & WS_EX_TOPMOST) != 0) == EnableTopMost;
    if (Applied == false)
    {
        // Windows ignores z-order changes from a background process while a fullscreen game owns the foreground, so borrow the foreground thread's input state
        const HWND Foreground = ::GetForegroundWindow();
        const DWORD ForegroundThread = Foreground != nullptr ? ::GetWindowThreadProcessId(Foreground, nullptr) : 0;
        const DWORD OwnThread = ::GetCurrentThreadId();
        const bool Attached = ForegroundThread != 0 && ForegroundThread != OwnThread && ::AttachThreadInput(OwnThread, ForegroundThread, TRUE) != FALSE;
        ApplyTopMost(EnableTopMost);
        if (Attached == true)
        {
            ::AttachThreadInput(OwnThread, ForegroundThread, FALSE);
        }

        if (((GetExStyle() & WS_EX_TOPMOST) != 0) != EnableTopMost)
        {
            NextTopMostRetryTick = ::GetTickCount64() + TOPMOST_RETRY_DELAY_MS;
            if (TopMostWarned == false)
            {
                TopMostWarned = true;
                Logger::Warning("Thumbnail could not be made topmost even with attached input");
            }
        }
    }

    if (((GetExStyle() & WS_EX_TOPMOST) != 0) == EnableTopMost)
    {
        TopMostWarned = false;
    }

    TopMost = EnableTopMost;
}

void ThumbnailView::ApplyTopMost(const bool EnableTopMost)
{
    const BOOL Result = SetWindowPos(EnableTopMost == true ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    if (Result == FALSE)
    {
        Logger::Warning("SetWindowPos for topmost failed, error " + std::to_string(::GetLastError()));
    }
}

void ThumbnailView::SetHighlight(const bool Enabled, const Color NewColor, const int Width)
{
    if (Enabled == false && HighlightRequested == false)
    {
        return;
    }

    if (Enabled == true && HighlightRequested == true && HighlightWidth == Width && HighlightColor == NewColor)
    {
        return;
    }

    HighlightRequested = Enabled;
    if (Enabled == true)
    {
        HighlightWidth = Width;
        HighlightColor = NewColor;
        SetBackgroundColor(NewColor.ToColorRef());
    }
    else
    {
        SetBackgroundColor(::GetSysColor(COLOR_BTNFACE));
    }

    SizeChanged = true;
}

void ThumbnailView::ZoomIn(const ZoomAnchor Anchor, const int ZoomFactor)
{
    const int OldWidth = BaseZoomSize.Width;
    const int OldHeight = BaseZoomSize.Height;

    const Point Location = GetThumbnailLocation();
    const Size Client = GetThumbnailSize();
    const Size Outer = GetOuterSize();
    const int NewWidth = ZoomFactor * Client.Width + (Outer.Width - Client.Width);
    const int NewHeight = ZoomFactor * Client.Height + (Outer.Height - Client.Height);

    // Resize first and move afterwards: moving first can lose the hover, which moves the window back and loops
    MaximumSize = Size{0, 0};
    SetOuterSize(Size{NewWidth, NewHeight});

    switch (Anchor)
    {
    case ZoomAnchor::NW:
        break;
    case ZoomAnchor::N:
        SetThumbnailLocation(Point{Location.X - NewWidth / 2 + OldWidth / 2, Location.Y});
        break;
    case ZoomAnchor::NE:
        SetThumbnailLocation(Point{Location.X - NewWidth + OldWidth, Location.Y});
        break;
    case ZoomAnchor::W:
        SetThumbnailLocation(Point{Location.X, Location.Y - NewHeight / 2 + OldHeight / 2});
        break;
    case ZoomAnchor::C:
        SetThumbnailLocation(Point{Location.X - NewWidth / 2 + OldWidth / 2, Location.Y - NewHeight / 2 + OldHeight / 2});
        break;
    case ZoomAnchor::E:
        SetThumbnailLocation(Point{Location.X - NewWidth + OldWidth, Location.Y - NewHeight / 2 + OldHeight / 2});
        break;
    case ZoomAnchor::SW:
        SetThumbnailLocation(Point{Location.X, Location.Y - NewHeight + OldHeight});
        break;
    case ZoomAnchor::S:
        SetThumbnailLocation(Point{Location.X - NewWidth / 2 + OldWidth / 2, Location.Y - NewHeight + OldHeight});
        break;
    case ZoomAnchor::SE:
        SetThumbnailLocation(Point{Location.X - NewWidth + OldWidth, Location.Y - NewHeight + OldHeight});
        break;
    }
}

void ThumbnailView::ZoomOut()
{
    RestoreWindowSizeAndLocation();
}

void ThumbnailView::RegisterHotkey(const Hotkey& Key)
{
    UnregisterHotkey();

    if (Key.IsNone() == true)
    {
        return;
    }

    HotkeyBinding = std::make_unique<HotkeyHandler>(m_hWnd, Key);
    HotkeyBinding->Register();
}

void ThumbnailView::UnregisterHotkey()
{
    HotkeyBinding.reset();
}

void ThumbnailView::Refresh(const bool ForceRefresh)
{
    RefreshThumbnail(ForceRefresh);
    HighlightThumbnail(ForceRefresh == true || SizeChanged == true);
    RefreshOverlay(ForceRefresh == true || SizeChanged == true || LocationChanged == true);

    SizeChanged = false;
}

void ThumbnailView::PaintContent(HDC)
{
}

const IWindowManager& ThumbnailView::GetWindowManager() const
{
    return WindowManagerInstance;
}

BYTE ThumbnailView::ToAlpha(const double Opacity)
{
    return static_cast<BYTE>(std::lround(std::clamp(Opacity, 0.0, 1.0) * 255.0));
}

Size ThumbnailView::GetOuterSize() const
{
    RECT Bounds = {};
    GetWindowRect(&Bounds);
    return Size{Bounds.right - Bounds.left, Bounds.bottom - Bounds.top};
}

void ThumbnailView::SetOuterSize(const Size NewSize)
{
    SetWindowPos(nullptr, 0, 0, NewSize.Width, NewSize.Height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void ThumbnailView::ReapplyTrackSizeLimits()
{
    SetOuterSize(GetOuterSize());
}

void ThumbnailView::SetBackgroundColor(const COLORREF NewColor)
{
    BackgroundColor = NewColor;
    ReleaseBackgroundBrush();
    BackgroundBrush = ::CreateSolidBrush(NewColor);
    Invalidate();
}

void ThumbnailView::ReleaseBackgroundBrush()
{
    if (BackgroundBrush == nullptr)
    {
        return;
    }

    ::DeleteObject(BackgroundBrush);
    BackgroundBrush = nullptr;
}

void ThumbnailView::SuppressResizeEvent()
{
    SuppressResizeEventsUntil = ::GetTickCount64() + RESIZE_EVENT_TIMEOUT_MS;
}

void ThumbnailView::HighlightThumbnail(const bool ForceRefresh)
{
    if (ForceRefresh == false && HighlightRequested == HighlightEnabled)
    {
        return;
    }

    HighlightEnabled = HighlightRequested;

    const Size Client = GetThumbnailSize();
    if (HighlightRequested == false)
    {
        ResizeThumbnail(Client.Width, Client.Height, 0, 0, 0, 0);
        return;
    }

    const int ActualHeight = Client.Height - 2 * HighlightWidth;
    if (Client.Width <= 0 || Client.Height <= 0 || ActualHeight <= 0)
    {
        ResizeThumbnail(Client.Width, Client.Height, 0, 0, 0, 0);
        return;
    }

    const double BaseAspectRatio = static_cast<double>(Client.Width) / Client.Height;
    const int ActualWidth = static_cast<int>(std::round(ActualHeight * BaseAspectRatio));
    const int HighlightLeft = (Client.Width - ActualWidth) / 2;
    const int HighlightRight = Client.Width - ActualWidth - HighlightLeft;

    ResizeThumbnail(Client.Width, Client.Height, HighlightWidth, HighlightRight, HighlightWidth, HighlightLeft);
}

void ThumbnailView::RefreshOverlay(const bool ForceRefresh)
{
    if (OverlayVisible == true && ForceRefresh == false)
    {
        return;
    }

    Overlay.EnableLabel(OverlayEnabled);

    // Shown once before it is positioned, otherwise the position would not be applied
    if (OverlayVisible == false)
    {
        Overlay.ShowOverlay();
        OverlayVisible = true;
    }

    POINT ClientOrigin = {0, 0};
    ClientToScreen(&ClientOrigin);

    LocationChanged = false;
    Overlay.SetBounds(Point{ClientOrigin.x, ClientOrigin.y}, GetThumbnailSize());
    Overlay.Refresh();
}

LRESULT ThumbnailView::OnEraseBackground(UINT, WPARAM Parameter, LPARAM, BOOL&)
{
    RECT Client = {};
    GetClientRect(&Client);
    ::FillRect(reinterpret_cast<HDC>(Parameter), &Client, BackgroundBrush);
    return 1;
}

LRESULT ThumbnailView::OnPaint(UINT, WPARAM, LPARAM, BOOL&)
{
    PAINTSTRUCT Paint = {};
    const HDC DeviceContext = BeginPaint(&Paint);
    PaintContent(DeviceContext);
    EndPaint(&Paint);
    return 0;
}

LRESULT ThumbnailView::OnMove(UINT, WPARAM, LPARAM, BOOL& Handled)
{
    Handled = FALSE;
    LocationChanged = true;

    if (ThumbnailMoved != nullptr)
    {
        ThumbnailMoved(ClientHandle);
    }

    return 0;
}

LRESULT ThumbnailView::OnSize(UINT, WPARAM, LPARAM, BOOL& Handled)
{
    Handled = FALSE;

    if (::GetTickCount64() < SuppressResizeEventsUntil)
    {
        return 0;
    }

    SizeChanged = true;

    if (ThumbnailResized != nullptr)
    {
        ThumbnailResized(ClientHandle);
    }

    return 0;
}

LRESULT ThumbnailView::OnGetMinMaxInfo(UINT, WPARAM, LPARAM Parameter, BOOL&)
{
    MINMAXINFO* const Info = reinterpret_cast<MINMAXINFO*>(Parameter);

    if (MinimumSize.Width > 0)
    {
        Info->ptMinTrackSize.x = MinimumSize.Width;
    }

    if (MinimumSize.Height > 0)
    {
        Info->ptMinTrackSize.y = MinimumSize.Height;
    }

    if (MaximumSize.Width > 0)
    {
        Info->ptMaxTrackSize.x = MaximumSize.Width;
    }

    if (MaximumSize.Height > 0)
    {
        Info->ptMaxTrackSize.y = MaximumSize.Height;
    }

    return 0;
}

LRESULT ThumbnailView::OnMouseActivate(UINT, WPARAM, LPARAM, BOOL&)
{
    return MA_NOACTIVATE;
}

LRESULT ThumbnailView::OnDpiChanged(UINT, WPARAM, LPARAM, BOOL&)
{
    return 0;
}

LRESULT ThumbnailView::OnMouseMove(UINT, WPARAM Keys, LPARAM, BOOL&)
{
    if (MouseTracking == false)
    {
        TRACKMOUSEEVENT Tracking = {};
        Tracking.cbSize = sizeof(Tracking);
        Tracking.dwFlags = TME_LEAVE;
        Tracking.hwndTrack = m_hWnd;
        ::TrackMouseEvent(&Tracking);
        MouseTracking = true;

        HandleMouseEnter();
    }

    if (CustomMouseModeActive == true && (Keys & (MK_LBUTTON | MK_RBUTTON)) == 0)
    {
        ExitCustomMouseMode();
    }

    if (CustomMouseModeActive == true)
    {
        ProcessCustomMouseMode((Keys & MK_LBUTTON) != 0, (Keys & MK_RBUTTON) != 0);
    }

    return 0;
}

LRESULT ThumbnailView::OnMouseLeave(UINT, WPARAM, LPARAM, BOOL&)
{
    MouseTracking = false;

    if (ThumbnailLostFocus != nullptr)
    {
        ThumbnailLostFocus(ClientHandle);
    }

    return 0;
}

LRESULT ThumbnailView::OnLeftButtonDown(UINT, WPARAM, LPARAM, BOOL&)
{
    const bool Control = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool Shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool Alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;

    if (Control == true && Shift == false && Alt == false)
    {
        RaiseDeactivated(false);
        return 0;
    }

    if (Control == true && Shift == true && Alt == false)
    {
        RaiseDeactivated(true);
        return 0;
    }

    if (ThumbnailActivated != nullptr)
    {
        ThumbnailActivated(ClientHandle);
    }

    return 0;
}

void ThumbnailView::SetLocked(const bool Locked)
{
    LockedInPlace = Locked;
}

LRESULT ThumbnailView::OnRightButtonDown(UINT, WPARAM, LPARAM, BOOL&)
{
    if (LockedInPlace == true)
    {
        return 0;
    }

    EnterCustomMouseMode();
    return 0;
}

LRESULT ThumbnailView::OnRightButtonUp(UINT, WPARAM, LPARAM, BOOL&)
{
    ExitCustomMouseMode();
    return 0;
}

LRESULT ThumbnailView::OnHotkey(UINT, WPARAM HotkeyId, LPARAM, BOOL&)
{
    if (HotkeyBinding == nullptr || static_cast<int>(HotkeyId) != HotkeyBinding->GetId())
    {
        return 0;
    }

    if (ThumbnailActivated != nullptr)
    {
        ThumbnailActivated(ClientHandle);
    }

    return 0;
}

void ThumbnailView::RaiseDeactivated(const bool SwitchOut)
{
    if (ThumbnailDeactivated != nullptr)
    {
        ThumbnailDeactivated(ClientHandle, SwitchOut);
    }
}

void ThumbnailView::HandleMouseEnter()
{
    ExitCustomMouseMode();
    SaveWindowSizeAndLocation();

    if (ThumbnailFocused != nullptr)
    {
        ThumbnailFocused(ClientHandle);
    }
}

void ThumbnailView::SaveWindowSizeAndLocation()
{
    BaseZoomSize = GetOuterSize();
    BaseZoomLocation = GetThumbnailLocation();
    BaseZoomMaximumSize = MaximumSize;
}

void ThumbnailView::RestoreWindowSizeAndLocation()
{
    SetOuterSize(BaseZoomSize);
    MaximumSize = BaseZoomMaximumSize;
    SetThumbnailLocation(BaseZoomLocation);
}

void ThumbnailView::EnterCustomMouseMode()
{
    RestoreWindowSizeAndLocation();

    CustomMouseModeActive = true;
    ::GetCursorPos(&BaseMousePosition);
    SetCapture();
}

void ThumbnailView::ProcessCustomMouseMode(const bool LeftButton, const bool RightButton)
{
    POINT MousePosition = {};
    ::GetCursorPos(&MousePosition);
    const int OffsetX = MousePosition.x - BaseMousePosition.x;
    const int OffsetY = MousePosition.y - BaseMousePosition.y;
    BaseMousePosition = MousePosition;

    if (LeftButton == true && RightButton == true)
    {
        const Size Outer = GetOuterSize();
        SetOuterSize(Size{Outer.Width + OffsetX, Outer.Height + OffsetY});
        BaseZoomSize = GetOuterSize();
        return;
    }

    const Point Location = GetThumbnailLocation();
    SetThumbnailLocation(Point{Location.X + OffsetX, Location.Y + OffsetY});
    BaseZoomLocation = GetThumbnailLocation();
}

void ThumbnailView::ExitCustomMouseMode()
{
    CustomMouseModeActive = false;

    if (::GetCapture() == m_hWnd)
    {
        ::ReleaseCapture();
    }
}
