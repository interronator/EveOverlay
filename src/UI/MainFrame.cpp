#include "UI/MainFrame.h"

#include <algorithm>
#include <filesystem>

#include <dwmapi.h>
#include <shellapi.h>

#include "Application/AppPaths.h"
#include "Config/TextUtil.h"
#include "UI/AppInfo.h"
#include "UI/ResourceIds.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

MainFrame::MainFrame(ThumbnailConfiguration& ConfigurationReference, ConfigurationStorage& StorageReference)
    : Configuration(ConfigurationReference)
    , TaskbarCreatedMessage(::RegisterWindowMessageW(L"TaskbarCreated"))
    , Pages{&General, &Thumbnail, &Organizer, &Zoom, &Overlay, &Clients, &Universe, &AccountSyncer, &About}
    , UniverseMap(AppPaths::GetUniverseDataPath(), AppPaths::GetUniverseMapSettingsPath())
    , Presenter(ConfigurationReference, StorageReference, Pages, Thumbnail, Clients, Organizer)
    , AppNameUtf8(TextUtil::ToUtf8(AppInfo::NAME))
{
    // The presenter stores the tab into the configuration first because it connected to SettingsChanged before this handler
    Universe.SettingsChanged.Connect([this]()
    {
        ApplyUniverseSettings();
        UpdateUniverseMapVisibility();
    });

    General.SettingsChanged.Connect([this]()
    {
        ApplyWindowOnTop();
        ApplyTheme();
    });

    Presenter.ClientsOpenChanged.Connect([this](const bool IsOpen)
    {
        ClientsOpen = IsOpen;
        Universe.SetClientsOpen(IsOpen);
        UpdateUniverseMapVisibility();
    });

    UniverseMap.IntelChannelWorked.Connect([this](const std::string& Channel)
    {
        Universe.AddSavedChannel(Channel);
    });

    Universe.TestSoundRequested.Connect([this]()
    {
        UniverseMap.PlayAlert();
    });

    Universe.SizePreviewed.Connect([this](const int MapSize)
    {
        UniverseMap.SetMapSize(MapSize);
    });

    Universe.RotationPreviewed.Connect([this](const int Degrees)
    {
        UniverseMap.SetMapRotation(Degrees);
    });
}

SettingsPresenter& MainFrame::GetPresenter()
{
    return Presenter;
}

void MainFrame::ShowInitially(const int ShowCommand)
{
    if (Configuration.MinimizeToTray == true)
    {
        return;
    }

    const bool UseDefaultShow = ShowCommand == SW_SHOWNORMAL || ShowCommand == SW_SHOWDEFAULT;
    ShowWindow(UseDefaultShow == true && Configuration.MainWindowMaximized == true ? SW_SHOWMAXIMIZED : ShowCommand);
    UpdateWindow();
}

void MainFrame::Restore()
{
    ShowWindow(SW_RESTORE);
    ::SetForegroundWindow(m_hWnd);
    RequestRedraw();
}

void MainFrame::RequestExit()
{
    ExitRequested = true;
    PostMessage(WM_CLOSE);
}

bool MainFrame::IsRedrawMessage(const UINT Message)
{
    if (Message >= WM_MOUSEFIRST && Message <= WM_MOUSELAST)
    {
        return true;
    }

    if (Message >= WM_KEYFIRST && Message <= WM_KEYLAST)
    {
        return true;
    }

    return Message == WM_SETFOCUS || Message == WM_KILLFOCUS || Message == WM_MOUSELEAVE || Message == WM_SHOWWINDOW || Message == WM_PAINT || Message == WM_DISPLAYCHANGE;
}

HICON MainFrame::LoadAppIcon(const int Size)
{
    return static_cast<HICON>(::LoadImageW(_Module.GetResourceInstance(), MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, Size, Size, LR_SHARED));
}

COLORREF MainFrame::ToColorRef(const ImVec4& Source)
{
    return RGB(static_cast<int>(Source.x * 255.0f + 0.5f), static_cast<int>(Source.y * 255.0f + 0.5f), static_cast<int>(Source.z * 255.0f + 0.5f));
}

void MainFrame::ApplyUniverseSettings()
{
    UniverseMapOptions Options;
    Options.System = Configuration.UniverseSystem;
    Options.Jumps = Configuration.UniverseJumps;
    Options.IntelChannel = Configuration.UniverseIntelChannel;
    Options.AlwaysOnTop = Configuration.UniverseMapAlwaysOnTop;
    Options.SmartMap = Configuration.UniverseSmartMap;
    Options.Size = Configuration.UniverseMapSize;
    Options.Rotation = Configuration.UniverseMapRotation;
    Options.SoundEnabled = Configuration.UniverseAlertSoundEnabled;
    Options.SoundVolume = Configuration.UniverseAlertVolume;
    Options.AlertSeconds = Configuration.UniverseAlertTimeout;
    Options.SoundPath = Configuration.UniverseAlertSoundPath;
    UniverseMap.Configure(Options);
    Universe.SetStatus(UniverseMap.GetStatus());
    Universe.SetSystemListing(&UniverseMap.GetSystemListing());
}

void MainFrame::UpdateUniverseMapVisibility()
{
    const bool ShouldShow = Configuration.UniverseMapVisible == true && ClientsOpen == true;
    if (ShouldShow == UniverseMapShown)
    {
        return;
    }

    UniverseMapShown = ShouldShow;
    if (ShouldShow == true)
    {
        UniverseMap.ShowMap();
        return;
    }

    UniverseMap.HideMap();
}

LRESULT MainFrame::OnAnyMessage(UINT Message, WPARAM Parameter, LPARAM Argument, BOOL& Handled)
{
    Handled = FALSE;
    if (Renderer.IsReady() == false)
    {
        return 0;
    }

    const LRESULT GuiResult = ImGui_ImplWin32_WndProcHandler(m_hWnd, Message, Parameter, Argument);
    if (IsRedrawMessage(Message) == true)
    {
        RequestRedraw();
    }

    if (GuiResult == 0)
    {
        return 0;
    }

    Handled = TRUE;
    return GuiResult;
}

LRESULT MainFrame::OnCreate(UINT, WPARAM, LPARAM, BOOL&)
{
    Dpi.SetDpi(DpiContext::GetWindowDpi(m_hWnd));

    const HICON LargeIcon = LoadAppIcon(::GetSystemMetricsForDpi(SM_CXICON, Dpi.GetDpi()));
    const HICON SmallIcon = LoadAppIcon(::GetSystemMetricsForDpi(SM_CXSMICON, Dpi.GetDpi()));
    SetIcon(LargeIcon, TRUE);
    SetIcon(SmallIcon, FALSE);

    ResizeWindowForClient();
    Theme::SetDark(Configuration.LightTheme == false);
    ApplyTitleBarColors();

    if (Renderer.Initialize(m_hWnd, GetScale()) == false)
    {
        return -1;
    }

    Presenter.LoadSettings();
    ApplyWindowOnTop();
    ApplyUniverseSettings();
    ::DragAcceptFiles(m_hWnd, TRUE);

    UpdateUniverseMapVisibility();

    Tray.Add(m_hWnd, WM_TRAYICON, LoadAppIcon(::GetSystemMetrics(SM_CXSMICON)), L"Eve Overlay");

    ::SetTimer(m_hWnd, RENDER_TIMER_ID, RENDER_INTERVAL_MS, nullptr);
    RequestRedraw();
    return 0;
}

LRESULT MainFrame::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
    ::KillTimer(m_hWnd, RENDER_TIMER_ID);
    Tray.Remove();
    Renderer.Shutdown();

    ::PostQuitMessage(0);
    return 0;
}

LRESULT MainFrame::OnClose(UINT, WPARAM, LPARAM, BOOL&)
{
    if (ExitRequested == false && Configuration.MinimizeToTray == true)
    {
        ShowWindow(SW_MINIMIZE);
        return 0;
    }

    StoreWindowSize();
    DestroyWindow();
    return 0;
}

LRESULT MainFrame::OnSize(UINT, WPARAM SizeType, LPARAM ClientSize, BOOL&)
{
    if (SizeType == SIZE_MINIMIZED)
    {
        if (Configuration.MinimizeToTray == true)
        {
            ShowWindow(SW_HIDE);
        }

        return 0;
    }

    Renderer.Resize(LOWORD(ClientSize), HIWORD(ClientSize));
    RequestRedraw();
    return 0;
}

LRESULT MainFrame::OnGetMinMaxInfo(UINT, WPARAM, LPARAM Argument, BOOL&)
{
    const SIZE Minimum = GetMinimumOuterSize();

    MINMAXINFO* const Info = reinterpret_cast<MINMAXINFO*>(Argument);
    Info->ptMinTrackSize.x = Minimum.cx;
    Info->ptMinTrackSize.y = Minimum.cy;
    return 0;
}

LRESULT MainFrame::OnTimer(UINT, WPARAM TimerId, LPARAM, BOOL& Handled)
{
    if (TimerId != RENDER_TIMER_ID)
    {
        Handled = FALSE;
        return 0;
    }

    if (Renderer.IsReady() == false || IsWindowVisible() == FALSE || IsIconic() == TRUE)
    {
        return 0;
    }

    if (FramesRemaining > 0)
    {
        FramesRemaining--;
        IdleTicks = 0;
        RenderFrame();
        return 0;
    }

    // Client lists change without any input reaching this window, so refresh now and then
    IdleTicks++;
    if (IdleTicks >= IDLE_REFRESH_TICKS)
    {
        IdleTicks = 0;
        RenderFrame();
    }

    return 0;
}

LRESULT MainFrame::OnEraseBackground(UINT, WPARAM, LPARAM, BOOL&)
{
    return 1;
}

LRESULT MainFrame::OnDpiChanged(UINT, WPARAM Parameter, LPARAM SuggestedRectangle, BOOL&)
{
    Dpi.SetDpi(LOWORD(Parameter));
    Renderer.SetScale(GetScale());

    const RECT* const Suggested = reinterpret_cast<const RECT*>(SuggestedRectangle);
    SetWindowPos(nullptr, Suggested->left, Suggested->top, Suggested->right - Suggested->left, Suggested->bottom - Suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
    RequestRedraw();
    return 0;
}

LRESULT MainFrame::OnDropFiles(UINT, WPARAM Parameter, LPARAM, BOOL&)
{
    const HDROP DropHandle = reinterpret_cast<HDROP>(Parameter);
    const UINT FileCount = ::DragQueryFileW(DropHandle, 0xFFFFFFFF, nullptr, 0);

    std::vector<std::filesystem::path> Files;
    for (UINT Index = 0; Index < FileCount; Index++)
    {
        std::wstring FilePath(::DragQueryFileW(DropHandle, Index, nullptr, 0) + 1, L'\0');
        const UINT Length = ::DragQueryFileW(DropHandle, Index, FilePath.data(), static_cast<UINT>(FilePath.size()));
        FilePath.resize(Length);
        Files.push_back(FilePath);
    }

    ::DragFinish(DropHandle);
    AccountSyncer.HandleDroppedFiles(Files);
    RequestRedraw();
    return 0;
}

LRESULT MainFrame::OnTrayIcon(UINT, WPARAM, LPARAM Parameter, BOOL&)
{
    if (Parameter == WM_LBUTTONDBLCLK)
    {
        Restore();
        return 0;
    }

    if (Parameter != WM_RBUTTONUP)
    {
        return 0;
    }

    const TrayIcon::MenuCommand Command = Tray.ShowMenu(m_hWnd, Configuration.UniverseMapVisible);
    if (Command == TrayIcon::MenuCommand::Restore)
    {
        Restore();
        return 0;
    }

    if (Command == TrayIcon::MenuCommand::UniverseMap)
    {
        Universe.NotifyMapVisible(Configuration.UniverseMapVisible == false);
        return 0;
    }

    if (Command == TrayIcon::MenuCommand::Exit)
    {
        RequestExit();
    }

    return 0;
}

LRESULT MainFrame::OnTaskbarCreated(UINT, WPARAM, LPARAM, BOOL&)
{
    Tray.Readd();
    return 0;
}

float MainFrame::GetScale() const
{
    return static_cast<float>(Dpi.GetDpi()) / static_cast<float>(DpiContext::BASE_DPI);
}

void MainFrame::RequestRedraw()
{
    FramesRemaining = ACTIVE_FRAME_COUNT;
}

void MainFrame::ApplyTitleBarColors()
{
    const BOOL UseDark = Theme::IsDark() == true ? TRUE : FALSE;
    ::DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &UseDark, sizeof(UseDark));

    const COLORREF CaptionColor = ToColorRef(Theme::SURFACE);
    const COLORREF BorderColor = ToColorRef(Theme::BORDER);
    const COLORREF TextColor = ToColorRef(Theme::TEXT);
    ::DwmSetWindowAttribute(m_hWnd, DWMWA_CAPTION_COLOR, &CaptionColor, sizeof(CaptionColor));
    ::DwmSetWindowAttribute(m_hWnd, DWMWA_BORDER_COLOR, &BorderColor, sizeof(BorderColor));
    ::DwmSetWindowAttribute(m_hWnd, DWMWA_TEXT_COLOR, &TextColor, sizeof(TextColor));
}

SIZE MainFrame::GetMinimumOuterSize() const
{
    RECT Outer = {0, 0, Dpi.Scale(CLIENT_WIDTH), Dpi.Scale(CLIENT_HEIGHT)};
    ::AdjustWindowRectExForDpi(&Outer, static_cast<DWORD>(GetStyle()), FALSE, static_cast<DWORD>(GetExStyle()), Dpi.GetDpi());
    return SIZE{Outer.right - Outer.left, Outer.bottom - Outer.top};
}

void MainFrame::ResizeWindowForClient()
{
    const SIZE Minimum = GetMinimumOuterSize();
    if (Configuration.MainWindowWidth <= 0 || Configuration.MainWindowHeight <= 0)
    {
        SetWindowPos(nullptr, 0, 0, Minimum.cx, Minimum.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return;
    }

    MONITORINFO Monitor = {};
    Monitor.cbSize = sizeof(Monitor);
    ::GetMonitorInfoW(::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &Monitor);

    const int MinimumWidth = static_cast<int>(Minimum.cx);
    const int MinimumHeight = static_cast<int>(Minimum.cy);
    const int Width = std::clamp(Dpi.Scale(Configuration.MainWindowWidth), MinimumWidth, std::max<int>(MinimumWidth, Monitor.rcWork.right - Monitor.rcWork.left));
    const int Height = std::clamp(Dpi.Scale(Configuration.MainWindowHeight), MinimumHeight, std::max<int>(MinimumHeight, Monitor.rcWork.bottom - Monitor.rcWork.top));
    SetWindowPos(nullptr, 0, 0, Width, Height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void MainFrame::StoreWindowSize()
{
    WINDOWPLACEMENT Placement = {};
    Placement.length = sizeof(Placement);
    if (GetWindowPlacement(&Placement) == FALSE)
    {
        return;
    }

    const int Base = static_cast<int>(DpiContext::BASE_DPI);
    Configuration.MainWindowWidth = ::MulDiv(Placement.rcNormalPosition.right - Placement.rcNormalPosition.left, Base, static_cast<int>(Dpi.GetDpi()));
    Configuration.MainWindowHeight = ::MulDiv(Placement.rcNormalPosition.bottom - Placement.rcNormalPosition.top, Base, static_cast<int>(Dpi.GetDpi()));
    Configuration.MainWindowMaximized = Placement.showCmd == SW_SHOWMAXIMIZED || (Placement.flags & WPF_RESTORETOMAXIMIZED) != 0;
}

void MainFrame::RenderFrame()
{
    Renderer.BeginFrame();
    DrawInterface();
    Renderer.EndFrame();
}

void MainFrame::DrawInterface()
{
    const ImGuiViewport* const Viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(Viewport->Pos);
    ImGui::SetNextWindowSize(Viewport->Size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const ImGuiWindowFlags RootFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("##Root", nullptr, RootFlags);
    ImGui::PopStyleVar();

    DrawSidebar();
    ImGui::SameLine(0.0f, 0.0f);
    DrawContent();

    ImGui::End();
}

void MainFrame::DrawSidebar()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::SURFACE);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Theme::Px(14.0f), Theme::Px(18.0f)));
    ImGui::BeginChild("##Sidebar", ImVec2(Theme::Px(SIDEBAR_WIDTH), 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::PushFont(Theme::GetBoldFont(), 18.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT);
    ImGui::TextUnformatted(AppNameUtf8.c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextUnformatted("Settings");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(14.0f)));

    for (size_t Index = 0; Index < Pages.size(); Index++)
    {
        DrawTabRow(Index);
    }

    const ImVec2 SidebarMin = ImGui::GetWindowPos();
    const ImVec2 SidebarSize = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(SidebarMin.x + SidebarSize.x - 1.0f, SidebarMin.y), ImVec2(SidebarMin.x + SidebarSize.x - 1.0f, SidebarMin.y + SidebarSize.y), ImGui::GetColorU32(ImGuiCol_Border));

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void MainFrame::DrawTabRow(const size_t Index)
{
    const float Width = ImGui::GetContentRegionAvail().x;
    const float Height = ImGui::GetFrameHeight() + Theme::Px(8.0f);
    const bool Selected = Index == SelectedPage;

    ImGui::PushID(static_cast<int>(Index));
    const ImVec2 Minimum = ImGui::GetCursorScreenPos();
    const ImVec2 Maximum(Minimum.x + Width, Minimum.y + Height);

    if (ImGui::InvisibleButton("##Tab", ImVec2(Width, Height)) == true)
    {
        SelectedPage = Index;
    }

    const bool Hovered = ImGui::IsItemHovered();
    Widgets::HoverTip(Pages[Index]->GetDescription().c_str());
    ImGui::PopID();

    ImDrawList* const DrawList = ImGui::GetWindowDrawList();
    if (Selected == true)
    {
        DrawList->AddRectFilled(Minimum, Maximum, ImGui::GetColorU32(Theme::WithAlpha(Theme::ACCENT, 0.16f)), Theme::Px(6.0f));
        DrawList->AddRectFilled(ImVec2(Minimum.x, Minimum.y + Theme::Px(8.0f)), ImVec2(Minimum.x + Theme::Px(3.0f), Maximum.y - Theme::Px(8.0f)), ImGui::GetColorU32(Theme::ACCENT), Theme::Px(2.0f));
    }
    else if (Hovered == true)
    {
        DrawList->AddRectFilled(Minimum, Maximum, ImGui::GetColorU32(ImGuiCol_HeaderHovered), Theme::Px(6.0f));
    }

    const ImVec4 TextColor = Selected == true || Hovered == true ? Theme::TEXT : Theme::TEXT_DISABLED;
    const std::string& Title = Pages[Index]->GetTitle();
    DrawList->AddText(ImVec2(Minimum.x + Theme::Px(16.0f), Minimum.y + (Height - ImGui::GetFontSize()) * 0.5f), ImGui::GetColorU32(TextColor), Title.c_str());
}

void MainFrame::DrawContent()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Theme::Px(28.0f), Theme::Px(24.0f)));
    ImGui::BeginChild("##Content", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None);

    ITabPage* const Page = Pages[SelectedPage];
    Widgets::PageHeading(Page->GetTitle().c_str(), Page->GetDescription().c_str());
    Page->Draw();

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void MainFrame::ApplyWindowOnTop()
{
    if (m_hWnd == nullptr)
    {
        return;
    }

    const HWND Order = Configuration.MainWindowAlwaysOnTop == true ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(Order, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void MainFrame::ApplyTheme()
{
    const bool WantDark = Configuration.LightTheme == false;
    if (Theme::IsDark() == WantDark || m_hWnd == nullptr)
    {
        return;
    }

    Theme::SetDark(WantDark);
    Renderer.SetScale(GetScale());
    ApplyTitleBarColors();
    RequestRedraw();
}
