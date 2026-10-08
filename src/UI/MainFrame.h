#pragma once

#include <string>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#include "Config/ConfigurationStorage.h"
#include "Config/ThumbnailConfiguration.h"
#include "Presenters/SettingsPresenter.h"
#include "UI/AboutTab.h"
#include "UI/AccountSyncerTab.h"
#include "UI/ClientsTab.h"
#include "UI/DpiContext.h"
#include "UI/GeneralTab.h"
#include "UI/GuiRenderer.h"
#include "UI/ITabPage.h"
#include "UI/OrganizerTab.h"
#include "UI/OverlayTab.h"
#include "UI/ThumbnailTab.h"
#include "UI/TrayIcon.h"
#include "UI/UniverseMapWindow.h"
#include "UI/UniverseTab.h"
#include "UI/ZoomTab.h"

class MainFrame : public CWindowImpl<MainFrame, CWindow, CWinTraits<WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CLIPCHILDREN, 0>>
{
public:
    DECLARE_WND_CLASS_EX(L"EveOverlayMainFrame", 0, COLOR_BTNFACE)

    BEGIN_MSG_MAP(MainFrame)
        MESSAGE_RANGE_HANDLER(0, 0xFFFF, OnAnyMessage)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)
        MESSAGE_HANDLER(WM_TRAYICON, OnTrayIcon)
        MESSAGE_HANDLER(TaskbarCreatedMessage, OnTaskbarCreated)
    END_MSG_MAP()

    MainFrame(ThumbnailConfiguration& ConfigurationReference, ConfigurationStorage& StorageReference);

    SettingsPresenter& GetPresenter();
    void ShowInitially(const int ShowCommand);
    void Restore();
    void RequestExit();

private:
    static constexpr int CLIENT_WIDTH = 700;
    static constexpr int CLIENT_HEIGHT = 470;
    static constexpr float SIDEBAR_WIDTH = 200.0f;
    static constexpr UINT WM_TRAYICON = WM_APP + 1;
    static constexpr UINT_PTR RENDER_TIMER_ID = 1;
    static constexpr UINT RENDER_INTERVAL_MS = 16;
    static constexpr int ACTIVE_FRAME_COUNT = 12;
    static constexpr int IDLE_REFRESH_TICKS = 30;

    static bool IsRedrawMessage(const UINT Message);
    static HICON LoadAppIcon(const int Size);
    static COLORREF ToColorRef(const ImVec4& Source);

    void ApplyWindowOnTop();
    void ApplyTheme();

    void ApplyUniverseSettings();

    // The map is only shown while it is switched on and at least one EVE client is open
    void UpdateUniverseMapVisibility();

    LRESULT OnAnyMessage(UINT Message, WPARAM Parameter, LPARAM Argument, BOOL& Handled);
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);

    // Closing only hides the window when the app is configured to live in the tray
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);

    LRESULT OnSize(UINT, WPARAM SizeType, LPARAM ClientSize, BOOL&);

    // The window can grow freely but never below the size the pages were laid out for
    LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM Argument, BOOL&);

    LRESULT OnTimer(UINT, WPARAM TimerId, LPARAM, BOOL& Handled);
    LRESULT OnEraseBackground(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDpiChanged(UINT, WPARAM Parameter, LPARAM SuggestedRectangle, BOOL&);
    LRESULT OnDropFiles(UINT, WPARAM Parameter, LPARAM, BOOL&);
    LRESULT OnTrayIcon(UINT, WPARAM, LPARAM Parameter, BOOL&);
    LRESULT OnTaskbarCreated(UINT, WPARAM, LPARAM, BOOL&);

    float GetScale() const;
    void RequestRedraw();
    void ApplyTitleBarColors();
    SIZE GetMinimumOuterSize() const;

    // The remembered size is kept at 96 dpi so it survives a move to a monitor with another scale
    void ResizeWindowForClient();

    void StoreWindowSize();
    void RenderFrame();
    void DrawInterface();
    void DrawSidebar();
    void DrawTabRow(const size_t Index);
    void DrawContent();

    ThumbnailConfiguration& Configuration;
    const UINT TaskbarCreatedMessage;
    DpiContext Dpi;
    GuiRenderer Renderer;
    GeneralTab General;
    ThumbnailTab Thumbnail;
    OrganizerTab Organizer;
    ZoomTab Zoom;
    OverlayTab Overlay;
    ClientsTab Clients;
    UniverseTab Universe;
    AccountSyncerTab AccountSyncer;
    AboutTab About;
    std::vector<ITabPage*> Pages;
    UniverseMapWindow UniverseMap;
    bool ClientsOpen = false;
    bool UniverseMapShown = false;
    SettingsPresenter Presenter;
    TrayIcon Tray;
    const std::string AppNameUtf8;
    size_t SelectedPage = 0;
    int FramesRemaining = 0;
    int IdleTicks = 0;
    bool ExitRequested = false;
};
