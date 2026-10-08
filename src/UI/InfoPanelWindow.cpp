#include "UI/InfoPanelWindow.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <utility>

#include <objidl.h>

namespace Gdiplus
{
using std::max;
using std::min;
}

#include <gdiplus.h>

#include "Config/TextUtil.h"

InfoPanelWindow::GdiplusSession::GdiplusSession()
{
    Gdiplus::GdiplusStartupInput StartupInput;
    Gdiplus::GdiplusStartup(&Token, &StartupInput, nullptr);
}

InfoPanelWindow::GdiplusSession::~GdiplusSession()
{
    Gdiplus::GdiplusShutdown(Token);
}

InfoPanelWindow::InfoPanelWindow(std::filesystem::path SettingsFilePath, std::wstring SectionName, const float Width)
    : SettingsPath(std::move(SettingsFilePath))
    , Section(std::move(SectionName))
    , PanelWidth(Width)
{
}

InfoPanelWindow::~InfoPanelWindow()
{
    if (m_hWnd != nullptr)
    {
        DestroyWindow();
    }
}

void InfoPanelWindow::Update(const std::vector<PanelRow>& Rows, const bool Visible)
{
    const bool WantVisible = Visible == true && Rows.empty() == false;
    if (m_hWnd == nullptr && WantVisible == false)
    {
        return;
    }

    EnsureWindow();
    CurrentRows = Rows;
    if (WantVisible == false)
    {
        if (IsWindowVisible() == TRUE)
        {
            SavePosition();
            ShowWindow(SW_HIDE);
        }

        return;
    }

    if (IsWindowVisible() == FALSE)
    {
        ShowWindow(SW_SHOWNOACTIVATE);
        LastSignature.clear();
    }

    Redraw();
}

void InfoPanelWindow::EnsureWindow()
{
    if (m_hWnd != nullptr)
    {
        return;
    }

    Create(nullptr, CWindow::rcDefault, L"Eve Overlay panel");
    RestorePosition();
    SetTimer(MOVE_TIMER_ID, MOVE_POLL_INTERVAL_MS);
}

LRESULT InfoPanelWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
    KillTimer(MOVE_TIMER_ID);
    ReleaseSurface();
    return 0;
}

LRESULT InfoPanelWindow::OnTimer(UINT, WPARAM Parameter, LPARAM, BOOL& Handled)
{
    if (Parameter != MOVE_TIMER_ID)
    {
        Handled = FALSE;
        return 0;
    }

    UpdateMoveMode();
    return 0;
}

LRESULT InfoPanelWindow::OnHitTest(UINT, WPARAM, LPARAM, BOOL&)
{
    return HTCAPTION;
}

LRESULT InfoPanelWindow::OnMouseActivate(UINT, WPARAM, LPARAM, BOOL&)
{
    return MA_NOACTIVATE;
}

LRESULT InfoPanelWindow::OnEnterSizeMove(UINT, WPARAM, LPARAM, BOOL&)
{
    Dragging = true;
    return 0;
}

LRESULT InfoPanelWindow::OnExitSizeMove(UINT, WPARAM, LPARAM, BOOL&)
{
    Dragging = false;
    SavePosition();
    return 0;
}

LRESULT InfoPanelWindow::OnDpiChanged(UINT, WPARAM, LPARAM, BOOL&)
{
    LastSignature.clear();
    Redraw();
    return 0;
}

void InfoPanelWindow::UpdateMoveMode()
{
    const bool AltDown = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    if (AltDown == MoveMode || Dragging == true || IsWindowVisible() == FALSE)
    {
        return;
    }

    MoveMode = AltDown;
    const LONG_PTR ExtendedStyle = GetWindowLongPtr(GWL_EXSTYLE);
    SetWindowLongPtr(GWL_EXSTYLE, MoveMode == true ? ExtendedStyle & ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT) : ExtendedStyle | WS_EX_TRANSPARENT);
    LastSignature.clear();
    Redraw();
}

void InfoPanelWindow::Redraw()
{
    if (m_hWnd == nullptr || IsWindowVisible() == FALSE || CurrentRows.empty() == true)
    {
        return;
    }

    std::string Signature = MoveMode == true ? "move|" : "|";
    for (const PanelRow& Entry : CurrentRows)
    {
        Signature += Entry.Label + "\t" + Entry.Value + "\t" + std::to_string(Entry.Red) + "\t" + (Entry.Heading == true ? "h" : "r") + "\n";
    }

    if (Signature == LastSignature)
    {
        return;
    }

    LastSignature = Signature;

    const float Scale = static_cast<float>(::GetDpiForWindow(m_hWnd)) / 96.0f;
    const int Width = static_cast<int>(std::lround(PanelWidth * Scale));
    const int Height = static_cast<int>(std::lround((static_cast<float>(CurrentRows.size()) * ROW_HEIGHT + 2.0f * PANEL_PADDING) * Scale));

    const HDC ScreenDc = ::GetDC(nullptr);
    if (EnsureSurface(ScreenDc, Width, Height) == false)
    {
        ::ReleaseDC(nullptr, ScreenDc);
        return;
    }

    std::memset(SurfacePixels, 0, static_cast<size_t>(Width) * static_cast<size_t>(Height) * 4);
    {
        Gdiplus::Bitmap Canvas(Width, Height, Width * 4, PixelFormat32bppPARGB, static_cast<BYTE*>(SurfacePixels));
        Gdiplus::Graphics Painter(&Canvas);
        Painter.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        Painter.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

        const float Radius = 8.0f * Scale;
        Gdiplus::GraphicsPath Outline;
        const Gdiplus::RectF Panel(0.5f, 0.5f, static_cast<float>(Width) - 1.0f, static_cast<float>(Height) - 1.0f);
        Outline.AddArc(Panel.X, Panel.Y, Radius * 2.0f, Radius * 2.0f, 180.0f, 90.0f);
        Outline.AddArc(Panel.GetRight() - Radius * 2.0f, Panel.Y, Radius * 2.0f, Radius * 2.0f, 270.0f, 90.0f);
        Outline.AddArc(Panel.GetRight() - Radius * 2.0f, Panel.GetBottom() - Radius * 2.0f, Radius * 2.0f, Radius * 2.0f, 0.0f, 90.0f);
        Outline.AddArc(Panel.X, Panel.GetBottom() - Radius * 2.0f, Radius * 2.0f, Radius * 2.0f, 90.0f, 90.0f);
        Outline.CloseFigure();

        const Gdiplus::SolidBrush Background(Gdiplus::Color(200, 16, 18, 24));
        const Gdiplus::Pen Border(MoveMode == true ? Gdiplus::Color(255, 80, 170, 255) : Gdiplus::Color(110, 120, 130, 150), MoveMode == true ? 3.0f * Scale : 1.0f);
        Painter.FillPath(&Background, &Outline);
        Painter.DrawPath(&Border, &Outline);

        const Gdiplus::FontFamily Family(L"Segoe UI");
        const Gdiplus::Font LabelFont(&Family, 13.0f * Scale, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        const Gdiplus::Font BoldFont(&Family, 13.0f * Scale, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::StringFormat LabelFormat;
        LabelFormat.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
        LabelFormat.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        LabelFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::StringFormat ValueFormat;
        ValueFormat.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
        ValueFormat.SetAlignment(Gdiplus::StringAlignmentFar);
        ValueFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);

        const Gdiplus::SolidBrush LabelBrush(Gdiplus::Color(255, 200, 205, 215));
        const float Padding = PANEL_PADDING * Scale;
        const float RowHeight = ROW_HEIGHT * Scale;
        const float ValueWidth = VALUE_WIDTH * Scale;
        for (size_t Index = 0; Index < CurrentRows.size(); Index++)
        {
            const PanelRow& Entry = CurrentRows[Index];
            const float Top = Padding + static_cast<float>(Index) * RowHeight;
            const std::wstring Label = TextUtil::FromUtf8(Entry.Label);
            const std::wstring Value = TextUtil::FromUtf8(Entry.Value);
            const float LabelWidth = Entry.Heading == true ? static_cast<float>(Width) - 2.0f * Padding : static_cast<float>(Width) - 2.0f * Padding - ValueWidth;
            const Gdiplus::RectF LabelBox(Padding, Top, LabelWidth, RowHeight);
            const Gdiplus::RectF ValueBox(static_cast<float>(Width) - Padding - ValueWidth, Top, ValueWidth, RowHeight);
            const Gdiplus::SolidBrush EntryBrush(Gdiplus::Color(255, Entry.Red, Entry.Green, Entry.Blue));
            if (Entry.Heading == true)
            {
                Painter.DrawString(Label.c_str(), static_cast<INT>(Label.size()), &BoldFont, LabelBox, &LabelFormat, &EntryBrush);
                continue;
            }

            Painter.DrawString(Label.c_str(), static_cast<INT>(Label.size()), &LabelFont, LabelBox, &LabelFormat, &LabelBrush);
            Painter.DrawString(Value.c_str(), static_cast<INT>(Value.size()), &BoldFont, ValueBox, &ValueFormat, &EntryBrush);
        }
    }

    SIZE WindowSize = {Width, Height};
    POINT Origin = {0, 0};
    BLENDFUNCTION Blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    ::UpdateLayeredWindow(m_hWnd, ScreenDc, nullptr, &WindowSize, SurfaceDc, &Origin, 0, &Blend, ULW_ALPHA);
    ::ReleaseDC(nullptr, ScreenDc);
}

void InfoPanelWindow::RestorePosition()
{
    constexpr int UNSET = INT_MIN;
    const int Left = ReadSetting(L"Left", UNSET);
    const int Top = ReadSetting(L"Top", UNSET);

    // A monitor may have been unplugged since the position was saved
    const RECT Saved = {Left, Top, Left + 100, Top + 40};
    if (Left != UNSET && Top != UNSET && ::MonitorFromRect(&Saved, MONITOR_DEFAULTTONULL) != nullptr)
    {
        SetWindowPos(nullptr, Left, Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        return;
    }

    SetWindowPos(nullptr, 40, 40, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void InfoPanelWindow::SavePosition() const
{
    RECT Bounds = {};
    if (m_hWnd == nullptr || ::GetWindowRect(m_hWnd, &Bounds) == FALSE)
    {
        return;
    }

    WriteSetting(L"Left", Bounds.left);
    WriteSetting(L"Top", Bounds.top);
}

int InfoPanelWindow::ReadSetting(const wchar_t* const Key, const int Fallback) const
{
    return static_cast<int>(::GetPrivateProfileIntW(Section.c_str(), Key, Fallback, SettingsPath.c_str()));
}

void InfoPanelWindow::WriteSetting(const wchar_t* const Key, const int Value) const
{
    ::WritePrivateProfileStringW(Section.c_str(), Key, std::to_wstring(Value).c_str(), SettingsPath.c_str());
}

bool InfoPanelWindow::EnsureSurface(const HDC ScreenDc, const int PixelWidth, const int PixelHeight)
{
    if (Surface != nullptr && SurfaceWidth == PixelWidth && SurfaceHeight == PixelHeight)
    {
        return true;
    }

    ReleaseSurface();

    BITMAPINFO Info = {};
    Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
    Info.bmiHeader.biWidth = PixelWidth;
    Info.bmiHeader.biHeight = -PixelHeight;
    Info.bmiHeader.biPlanes = 1;
    Info.bmiHeader.biBitCount = 32;
    Info.bmiHeader.biCompression = BI_RGB;

    SurfaceDc = ::CreateCompatibleDC(ScreenDc);
    Surface = ::CreateDIBSection(ScreenDc, &Info, DIB_RGB_COLORS, &SurfacePixels, nullptr, 0);
    if (SurfaceDc == nullptr || Surface == nullptr)
    {
        ReleaseSurface();
        return false;
    }

    SurfaceOriginalBitmap = ::SelectObject(SurfaceDc, Surface);
    SurfaceWidth = PixelWidth;
    SurfaceHeight = PixelHeight;
    return true;
}

void InfoPanelWindow::ReleaseSurface()
{
    if (SurfaceDc != nullptr && SurfaceOriginalBitmap != nullptr)
    {
        ::SelectObject(SurfaceDc, SurfaceOriginalBitmap);
    }

    if (Surface != nullptr)
    {
        ::DeleteObject(Surface);
    }

    if (SurfaceDc != nullptr)
    {
        ::DeleteDC(SurfaceDc);
    }

    SurfaceDc = nullptr;
    Surface = nullptr;
    SurfaceOriginalBitmap = nullptr;
    SurfacePixels = nullptr;
    SurfaceWidth = 0;
    SurfaceHeight = 0;
}
