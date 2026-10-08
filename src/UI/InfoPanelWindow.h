#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

struct PanelRow
{
    std::string Label;
    std::string Value;
    BYTE Red = 255;
    BYTE Green = 255;
    BYTE Blue = 255;

    // A heading draws its label in bold and has no value
    bool Heading = false;
};

// A small borderless, per-pixel transparent panel showing rows of text over the game. Only the panel receives the mouse, and only
// while Alt is held, so it can be dragged; the rest of the time clicks fall through. Its position is remembered per name.
class InfoPanelWindow : public CWindowImpl<InfoPanelWindow, CWindow, CWinTraits<WS_POPUP, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST>>
{
public:
    DECLARE_WND_CLASS_EX(L"EveOverlayInfoPanel", 0, 0)

    BEGIN_MSG_MAP(InfoPanelWindow)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_NCHITTEST, OnHitTest)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
        MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
        MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
    END_MSG_MAP()

    // SectionName keys the saved position in the settings file; Width is in device independent pixels
    InfoPanelWindow(std::filesystem::path SettingsFilePath, std::wstring SectionName, const float Width);
    InfoPanelWindow(const InfoPanelWindow&) = delete;
    InfoPanelWindow& operator=(const InfoPanelWindow&) = delete;

    ~InfoPanelWindow();

    // Shows the rows, or hides the panel when there are none or Visible is false
    void Update(const std::vector<PanelRow>& Rows, const bool Visible);

private:
    static constexpr UINT_PTR MOVE_TIMER_ID = 1;
    static constexpr UINT MOVE_POLL_INTERVAL_MS = 50;
    static constexpr float ROW_HEIGHT = 22.0f;
    static constexpr float PANEL_PADDING = 10.0f;
    static constexpr float VALUE_WIDTH = 78.0f;

    class GdiplusSession
    {
    public:
        GdiplusSession();
        GdiplusSession(const GdiplusSession&) = delete;
        GdiplusSession& operator=(const GdiplusSession&) = delete;

        ~GdiplusSession();

    private:
        ULONG_PTR Token = 0;
    };

    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnTimer(UINT, WPARAM Parameter, LPARAM, BOOL& Handled);
    LRESULT OnHitTest(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMouseActivate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnEnterSizeMove(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnExitSizeMove(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL&);

    void EnsureWindow();
    void UpdateMoveMode();
    void Redraw();
    void RestorePosition();
    void SavePosition() const;
    int ReadSetting(const wchar_t* const Key, const int Fallback) const;
    void WriteSetting(const wchar_t* const Key, const int Value) const;
    bool EnsureSurface(const HDC ScreenDc, const int PixelWidth, const int PixelHeight);
    void ReleaseSurface();

    HDC SurfaceDc = nullptr;
    HBITMAP Surface = nullptr;
    HGDIOBJ SurfaceOriginalBitmap = nullptr;
    void* SurfacePixels = nullptr;
    int SurfaceWidth = 0;
    int SurfaceHeight = 0;

    GdiplusSession Gdi;
    std::filesystem::path SettingsPath;
    std::wstring Section;
    float PanelWidth;
    std::vector<PanelRow> CurrentRows;
    std::string LastSignature;
    bool MoveMode = false;
    bool Dragging = false;
};
