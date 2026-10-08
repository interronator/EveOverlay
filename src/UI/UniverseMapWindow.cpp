#include "UI/UniverseMapWindow.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <iterator>
#include <utility>

#include "Config/TextUtil.h"
#include "Universe/IntelParser.h"
#include "Universe/UniverseNeighborhood.h"

UniverseMapWindow::GdiplusSession::GdiplusSession()
{
    Gdiplus::GdiplusStartupInput StartupInput;
    Gdiplus::GdiplusStartup(&Token, &StartupInput, nullptr);
}

UniverseMapWindow::GdiplusSession::~GdiplusSession()
{
    Gdiplus::GdiplusShutdown(Token);
}

UniverseMapWindow::UniverseMapWindow(std::filesystem::path DataPath, std::filesystem::path SettingsFilePath, std::filesystem::path JumpBridgesPath)
    : CsvPath(std::move(DataPath))
    , SettingsPath(std::move(SettingsFilePath))
    , BridgesPath(std::move(JumpBridgesPath))
{
}

UniverseMapWindow::~UniverseMapWindow()
{
    if (m_hWnd != nullptr)
    {
        DestroyWindow();
    }
}

void UniverseMapWindow::Configure(const UniverseMapOptions& Options)
{
    Center = TextUtil::Trim(Options.System);
    MaxJumps = std::clamp(Options.Jumps, 0, MAX_JUMPS);
    Channel = Options.IntelChannel;
    TopMost = Options.AlwaysOnTop;
    View.SetSmartMode(Options.SmartMap);
    Size = Options.Size;
    View.SetRotation(static_cast<float>(Options.Rotation));
    AlertDurationMs = static_cast<ULONGLONG>(std::max(1, Options.AlertSeconds)) * 1000ULL;
    SoundEnabled = Options.SoundEnabled;
    SoundVolume = Options.SoundVolume;
    SoundPath = TextUtil::FromUtf8(Options.SoundPath);
    IgnoreClear = Options.IgnoreClear;
    ScaleVolumeByDistance = Options.ScaleVolumeByDistance;
    Keywords = IntelParser::ParseKeywordList(Options.Keywords);
    KeywordSoundPath = TextUtil::FromUtf8(Options.KeywordSoundPath);

    // The current system is looked up on the next poll instead of here, because a change of home system saves the settings,
    // which would call back into this function
    FollowCheckPending = Options.FollowLocation == true && FollowLocation == false;
    FollowLocation = Options.FollowLocation;
    UseJumpBridges = Options.UseJumpBridges;
    ReloadIfBridgesChanged();

    RebuildNeighborhood();

    if (m_hWnd == nullptr)
    {
        return;
    }

    ApplyWindowState();
    Redraw();
}

void UniverseMapWindow::PlayAlert()
{
    Sound.Play(SoundPath, SoundVolume);
}

void UniverseMapWindow::PlayKeywordAlert()
{
    Sound.Play(KeywordSoundPath.empty() == true ? SoundPath : KeywordSoundPath, SoundVolume);
}

void UniverseMapWindow::SetMapSize(const int MapSize)
{
    Size = MapSize;
    if (m_hWnd == nullptr)
    {
        return;
    }

    ApplyWindowState();
    Redraw();
}

void UniverseMapWindow::SetMapRotation(const int Degrees)
{
    View.SetRotation(static_cast<float>(Degrees));
    Redraw();
}

void UniverseMapWindow::ShowMap()
{
    if (m_hWnd == nullptr)
    {
        Create(nullptr, CWindow::rcDefault, L"Intel Watcher");
        RestorePosition();
    }

    ApplyWindowState();
    ShowWindow(SW_SHOWNOACTIVATE);
    Redraw();
}

void UniverseMapWindow::HideMap()
{
    if (m_hWnd == nullptr)
    {
        return;
    }

    SavePosition();
    ShowWindow(SW_HIDE);
}

const std::string& UniverseMapWindow::GetStatus() const
{
    return Status;
}

const SystemListing& UniverseMapWindow::GetSystemListing() const
{
    return Listing;
}

const std::vector<IntelHistoryEntry>& UniverseMapWindow::GetHistory() const
{
    return History;
}

unsigned UniverseMapWindow::GetHistoryRevision() const
{
    return HistoryRevision;
}

void UniverseMapWindow::ClearHistory()
{
    History.clear();
    HistoryRevision++;
}

float UniverseMapWindow::DistanceVolumeScale(const int Jumps, const int MaxJumps)
{
    if (MaxJumps <= 0 || Jumps <= 0)
    {
        return 1.0f;
    }

    const float Share = std::min(1.0f, static_cast<float>(Jumps) / static_cast<float>(MaxJumps));
    return 1.0f - (1.0f - FAR_VOLUME_SHARE) * Share;
}

float UniverseMapWindow::FlashCycles(const double ElapsedSeconds, const double TotalSeconds)
{
    const double Cycles = FLASH_START_HZ * ElapsedSeconds + (FLASH_END_HZ - FLASH_START_HZ) * ElapsedSeconds * ElapsedSeconds / (2.0 * TotalSeconds);
    return static_cast<float>(Cycles);
}

LRESULT UniverseMapWindow::OnCreate(UINT, WPARAM, LPARAM, BOOL&)
{
    SetTimer(POLL_TIMER_ID, POLL_INTERVAL_MS);
    SetTimer(MOVE_TIMER_ID, MOVE_POLL_INTERVAL_MS);
    SetTimer(ANIMATION_TIMER_ID, ANIMATION_INTERVAL_MS);
    return 0;
}

LRESULT UniverseMapWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
    KillTimer(POLL_TIMER_ID);
    KillTimer(MOVE_TIMER_ID);
    KillTimer(ANIMATION_TIMER_ID);
    ReleaseSurface();
    return 0;
}

LRESULT UniverseMapWindow::OnTimer(UINT, WPARAM Parameter, LPARAM, BOOL& Handled)
{
    if (Parameter == ANIMATION_TIMER_ID)
    {
        AnimateAlerts();
        return 0;
    }

    if (Parameter == MOVE_TIMER_ID)
    {
        UpdateMoveMode();
        return 0;
    }

    if (Parameter != POLL_TIMER_ID)
    {
        Handled = FALSE;
        return 0;
    }

    PollIntel();
    return 0;
}

LRESULT UniverseMapWindow::OnHitTest(UINT, WPARAM, LPARAM, BOOL&)
{
    return HTCAPTION;
}

LRESULT UniverseMapWindow::OnEnterSizeMove(UINT, WPARAM, LPARAM, BOOL&)
{
    Dragging = true;
    return 0;
}

void UniverseMapWindow::UpdateMoveMode()
{
    const bool AltDown = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    if (AltDown == MoveMode || Dragging == true || IsWindowVisible() == FALSE)
    {
        return;
    }

    MoveMode = AltDown;
    const LONG_PTR ExtendedStyle = GetWindowLongPtr(GWL_EXSTYLE);
    SetWindowLongPtr(GWL_EXSTYLE, MoveMode == true ? ExtendedStyle & ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT) : ExtendedStyle | WS_EX_TRANSPARENT);
    Redraw();
}

LRESULT UniverseMapWindow::OnMouseActivate(UINT, WPARAM, LPARAM, BOOL&)
{
    return MA_NOACTIVATE;
}

LRESULT UniverseMapWindow::OnExitSizeMove(UINT, WPARAM, LPARAM, BOOL&)
{
    Dragging = false;
    SavePosition();
    return 0;
}

LRESULT UniverseMapWindow::OnDpiChanged(UINT, WPARAM, LPARAM, BOOL&)
{
    ApplyWindowState();
    Redraw();
    return 0;
}

int UniverseMapWindow::ReadSetting(const wchar_t* const Key, const int Fallback) const
{
    return static_cast<int>(::GetPrivateProfileIntW(SETTINGS_SECTION, Key, Fallback, SettingsPath.c_str()));
}

void UniverseMapWindow::WriteSetting(const wchar_t* const Key, const int Value) const
{
    ::WritePrivateProfileStringW(SETTINGS_SECTION, Key, std::to_wstring(Value).c_str(), SettingsPath.c_str());
}

int UniverseMapWindow::GetPixelSize() const
{
    const double Scale = static_cast<double>(::GetDpiForWindow(m_hWnd)) / 96.0;
    return std::max(MINIMUM_SIZE, static_cast<int>(Size * Scale));
}

void UniverseMapWindow::ApplyWindowState()
{
    const int PixelSize = GetPixelSize();
    SetWindowPos(TopMost == true ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, PixelSize, PixelSize, SWP_NOMOVE | SWP_NOACTIVATE);
}

void UniverseMapWindow::RestorePosition()
{
    constexpr int UNSET = INT_MIN;
    const int Left = ReadSetting(L"Left", UNSET);
    const int Top = ReadSetting(L"Top", UNSET);
    const int PixelSize = GetPixelSize();

    // A monitor may have been unplugged since the position was saved
    const RECT Saved = {Left, Top, Left + PixelSize, Top + PixelSize};
    if (Left != UNSET && Top != UNSET && ::MonitorFromRect(&Saved, MONITOR_DEFAULTTONULL) != nullptr)
    {
        SetWindowPos(nullptr, Left, Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        return;
    }

    const int ScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    const int ScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(nullptr, (ScreenWidth - PixelSize) / 2, (ScreenHeight - PixelSize) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void UniverseMapWindow::SavePosition() const
{
    RECT Bounds = {};
    if (::GetWindowRect(m_hWnd, &Bounds) == FALSE)
    {
        return;
    }

    WriteSetting(L"Left", Bounds.left);
    WriteSetting(L"Top", Bounds.top);
}

void UniverseMapWindow::LoadUniverse()
{
    Loaded = Data.Load(CsvPath);
    Ships.Load(CsvPath.parent_path() / L"types.tsv");
    BridgeCount = 0;
    BridgesApplied = UseJumpBridges;
    std::error_code Error;
    BridgesStamp = UseJumpBridges == true ? std::filesystem::last_write_time(BridgesPath, Error) : std::filesystem::file_time_type();
    if (Loaded == true && UseJumpBridges == true)
    {
        std::ifstream Stream(BridgesPath, std::ios::binary);
        const std::string Text((std::istreambuf_iterator<char>(Stream)), std::istreambuf_iterator<char>());
        BridgeCount = Data.AddBridges(Text);
    }

    Listing = SystemListing::Build(Data);
}

void UniverseMapWindow::ReloadIfBridgesChanged()
{
    if (Loaded == false)
    {
        return;
    }

    std::error_code Error;
    const std::filesystem::file_time_type Stamp = UseJumpBridges == true ? std::filesystem::last_write_time(BridgesPath, Error) : std::filesystem::file_time_type();
    if (UseJumpBridges != BridgesApplied || Stamp != BridgesStamp)
    {
        Loaded = false;
    }
}

void UniverseMapWindow::RebuildNeighborhood()
{
    NodeSystems.clear();
    Candidates.clear();
    SystemJumps.clear();

    if (Loaded == false)
    {
        LoadUniverse();
    }

    if (Loaded == false)
    {
        Status = "Could not load " + TextUtil::ToUtf8(CsvPath.wstring());
        View.SetMap(&Data, UniverseNeighborhood());
        return;
    }

    const int CenterSystem = Data.Find(Center);
    if (CenterSystem == UniverseData::NOT_FOUND)
    {
        Status = "Unknown system: " + Center;
        View.SetMap(&Data, UniverseNeighborhood());
        return;
    }

    UniverseNeighborhood Neighborhood = UniverseNeighborhood::Build(Data, CenterSystem, MaxJumps);
    const SolarSystem& CenterInfo = Data.Get(CenterSystem);
    Status = CenterInfo.Name + " (" + CenterInfo.Region + ") - " + std::to_string(Neighborhood.Nodes.size()) + " systems within " + std::to_string(MaxJumps) + " jumps" + (BridgeCount > 0 ? ", " + std::to_string(BridgeCount) + " jump bridges" : std::string());

    for (const NeighborhoodNode& Node : Neighborhood.Nodes)
    {
        NodeSystems.push_back(Node.SystemIndex);
        Candidates.insert(Node.SystemIndex);
        SystemJumps[Node.SystemIndex] = Node.Jumps;
    }

    View.SetMap(&Data, std::move(Neighborhood));
    View.SetHighlights(BuildHighlights());
}

std::vector<UniverseMapView::NodeAlert> UniverseMapWindow::BuildHighlights() const
{
    const ULONGLONG Now = ::GetTickCount64();
    std::vector<UniverseMapView::NodeAlert> Highlighted(NodeSystems.size());
    for (size_t Index = 0; Index < NodeSystems.size(); Index++)
    {
        const std::unordered_map<int, ULONGLONG>::const_iterator Cleared = ClearStart.find(NodeSystems[Index]);
        const std::unordered_map<int, ULONGLONG>::const_iterator Cautioned = CautionStart.find(NodeSystems[Index]);
        const bool IsCaution = Cleared == ClearStart.end() && Cautioned != CautionStart.end();
        const std::unordered_map<int, ULONGLONG>::const_iterator Pulse = IsCaution == true ? Cautioned : Cleared;
        const bool HasPulse = IsCaution == true || Cleared != ClearStart.end();
        if (HasPulse == true && Now - Pulse->second < CLEAR_PULSE_MS)
        {
            const double ClearElapsed = static_cast<double>(Now - Pulse->second) / 1000.0;
            Highlighted[Index].ClearCycles = static_cast<float>(CLEAR_PULSE_HZ * ClearElapsed);
            Highlighted[Index].ClearStrength = static_cast<float>(1.0 - static_cast<double>(Now - Pulse->second) / static_cast<double>(CLEAR_PULSE_MS));
            Highlighted[Index].Caution = IsCaution;
        }

        const std::unordered_map<int, ULONGLONG>::const_iterator Alert = AlertStart.find(NodeSystems[Index]);
        if (Alert == AlertStart.end() || Now - Alert->second >= AlertDurationMs)
        {
            continue;
        }

        const double ElapsedSeconds = static_cast<double>(Now - Alert->second) / 1000.0;
        const double TotalSeconds = static_cast<double>(AlertDurationMs) / 1000.0;
        Highlighted[Index].Cycles = FlashCycles(ElapsedSeconds, TotalSeconds);
        Highlighted[Index].Remaining = static_cast<float>(1.0 - ElapsedSeconds / TotalSeconds);
    }

    return Highlighted;
}

void UniverseMapWindow::AnimateAlerts()
{
    if (AlertStart.empty() == true && ClearStart.empty() == true && CautionStart.empty() == true)
    {
        return;
    }

    const ULONGLONG Now = ::GetTickCount64();
    for (std::unordered_map<int, ULONGLONG>::iterator Alert = AlertStart.begin(); Alert != AlertStart.end();)
    {
        Alert = Now - Alert->second >= AlertDurationMs ? AlertStart.erase(Alert) : std::next(Alert);
    }

    for (std::unordered_map<int, ULONGLONG>::iterator Cleared = ClearStart.begin(); Cleared != ClearStart.end();)
    {
        Cleared = Now - Cleared->second >= CLEAR_PULSE_MS ? ClearStart.erase(Cleared) : std::next(Cleared);
    }

    for (std::unordered_map<int, ULONGLONG>::iterator Cautioned = CautionStart.begin(); Cautioned != CautionStart.end();)
    {
        Cautioned = Now - Cautioned->second >= CLEAR_PULSE_MS ? CautionStart.erase(Cautioned) : std::next(Cautioned);
    }

    if (View.SetHighlights(BuildHighlights()) == true && IsWindowVisible() == TRUE)
    {
        Redraw();
    }
}

void UniverseMapWindow::PollIntel()
{
    PollLocation();

    if (Loaded == false || NodeSystems.empty() == true)
    {
        return;
    }

    Watcher.SetChannel(Channel);
    const ULONGLONG Now = ::GetTickCount64();
    bool SpottedInRange = false;
    bool AnyPriority = false;
    int ClosestJumps = INT_MAX;
    for (const ChatMessage& Message : Watcher.Poll())
    {
        const std::vector<int> Systems = IntelParser::FindSystems(Data, Message.Text, Candidates, IgnoreClear);
        if (IgnoreClear == true)
        {
            for (const int System : IntelParser::FindClearedSystems(Data, Message.Text, Candidates))
            {
                AlertStart.erase(System);
                CautionStart.erase(System);
                ClearStart[System] = Now;
            }
        }

        if (Systems.empty() == true)
        {
            continue;
        }

        const bool Priority = Keywords.empty() == false && IntelParser::ContainsKeyword(Message.Text, Keywords) == true;
        if (Priority == false && IntelParser::IsHarmlessReport(Ships, Message.Text) == true)
        {
            for (const int System : Systems)
            {
                RecordReport(Message, System, false);
                if (AlertStart.contains(System) == true)
                {
                    continue;
                }

                ClearStart.erase(System);
                CautionStart[System] = Now;
            }

            continue;
        }

        for (const int System : Systems)
        {
            AlertStart[System] = Now;
            ClearStart.erase(System);
            CautionStart.erase(System);
            ClosestJumps = std::min(ClosestJumps, SystemJumps[System]);
            RecordReport(Message, System, Priority);
        }

        SpottedInRange = true;
        AnyPriority = AnyPriority || Priority;
    }

    if (SpottedInRange == true && WorkedChannels.insert(TextUtil::ToLower(Channel)).second == true)
    {
        IntelChannelWorked.Emit(Channel);
    }

    if (SpottedInRange == true && SoundEnabled == true && IsWindowVisible() == TRUE && Now - LastAlertTick >= MIN_ALERT_GAP_MS)
    {
        LastAlertTick = Now;
        PlayReportSound(AnyPriority, ClosestJumps);
    }

    AnimateAlerts();
}

void UniverseMapWindow::PlayReportSound(const bool Priority, const int ClosestJumps)
{
    if (Priority == true)
    {
        PlayKeywordAlert();
        return;
    }

    const float Scale = ScaleVolumeByDistance == true ? DistanceVolumeScale(ClosestJumps, MaxJumps) : 1.0f;
    Sound.Play(SoundPath, static_cast<int>(static_cast<float>(SoundVolume) * Scale));
}

void UniverseMapWindow::RecordReport(const ChatMessage& Message, const int System, const bool Priority)
{
    SYSTEMTIME Now = {};
    ::GetLocalTime(&Now);
    char Clock[16] = {};
    std::snprintf(Clock, sizeof(Clock), "%02u:%02u:%02u", Now.wHour, Now.wMinute, Now.wSecond);

    IntelHistoryEntry Entry;
    Entry.Time = Clock;
    Entry.System = Data.Get(System).Name;
    Entry.Channel = Message.Channel;
    Entry.Sender = Message.Sender;
    Entry.Text = Message.Text;
    Entry.Jumps = SystemJumps[System];
    Entry.Priority = Priority;

    History.insert(History.begin(), std::move(Entry));
    if (History.size() > MAX_HISTORY)
    {
        History.resize(MAX_HISTORY);
    }

    HistoryRevision++;
}

void UniverseMapWindow::PollLocation()
{
    if (FollowLocation == false || Loaded == false)
    {
        return;
    }

    LocalWatcher.SetExactChannelMatch(true);
    LocalWatcher.SetIncludeSystemMessages(true);
    LocalWatcher.SetChannel("Local");

    if (FollowCheckPending == true)
    {
        FollowCheckPending = false;
        FollowSystem(LocalWatcher.FindCurrentSystem());
    }

    for (const ChatMessage& Message : LocalWatcher.Poll())
    {
        if (Message.Sender == "EVE System")
        {
            FollowSystem(ChatLogWatcher::ParseChannelChange(Message.Text));
        }
    }
}

void UniverseMapWindow::FollowSystem(const std::string& SystemName)
{
    if (SystemName.empty() == true)
    {
        return;
    }

    const int System = Data.Find(SystemName);
    if (System == UniverseData::NOT_FOUND || TextUtil::EqualsIgnoreCase(Data.Get(System).Name, Center) == true)
    {
        return;
    }

    Center = Data.Get(System).Name;
    RebuildNeighborhood();
    Redraw();
    HomeSystemFollowed.Emit(Center);
}

void UniverseMapWindow::Redraw()
{
    if (m_hWnd == nullptr || IsWindowVisible() == FALSE)
    {
        return;
    }

    const int PixelSize = GetPixelSize();
    const HDC ScreenDc = ::GetDC(nullptr);
    if (EnsureSurface(ScreenDc, PixelSize) == false)
    {
        ::ReleaseDC(nullptr, ScreenDc);
        return;
    }

    std::memset(SurfacePixels, 0, static_cast<size_t>(PixelSize) * static_cast<size_t>(PixelSize) * 4);
    {
        Gdiplus::Bitmap Canvas(PixelSize, PixelSize, PixelSize * 4, PixelFormat32bppPARGB, static_cast<BYTE*>(SurfacePixels));
        Gdiplus::Graphics Painter(&Canvas);
        const float Scale = static_cast<float>(::GetDpiForWindow(m_hWnd)) / 96.0f;
        View.Paint(Painter, static_cast<float>(PixelSize), static_cast<float>(PixelSize), Scale);

        if (MoveMode == true)
        {
            // The faint fill matters: fully transparent pixels let clicks through, so it makes the whole frame grabbable. Outside
            // the frame the window stays click-through.
            const float Thickness = 3.0f * Scale;
            const Gdiplus::RectF Bounds = View.GetMoveBounds(static_cast<float>(PixelSize), static_cast<float>(PixelSize), Scale);
            const Gdiplus::SolidBrush Tint(Gdiplus::Color(40, 80, 170, 255));
            const Gdiplus::Pen Frame(Gdiplus::Color(255, 80, 170, 255), Thickness);
            Painter.FillRectangle(&Tint, Bounds);
            Painter.DrawRectangle(&Frame, Bounds.X + Thickness / 2.0f, Bounds.Y + Thickness / 2.0f, Bounds.Width - Thickness, Bounds.Height - Thickness);
        }
    }

    SIZE WindowSize = {PixelSize, PixelSize};
    POINT Origin = {0, 0};
    BLENDFUNCTION Blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    ::UpdateLayeredWindow(m_hWnd, ScreenDc, nullptr, &WindowSize, SurfaceDc, &Origin, 0, &Blend, ULW_ALPHA);

    ::ReleaseDC(nullptr, ScreenDc);
}

bool UniverseMapWindow::EnsureSurface(const HDC ScreenDc, const int PixelSize)
{
    if (Surface != nullptr && SurfaceSize == PixelSize)
    {
        return true;
    }

    ReleaseSurface();

    BITMAPINFO Info = {};
    Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
    Info.bmiHeader.biWidth = PixelSize;
    Info.bmiHeader.biHeight = -PixelSize;
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
    SurfaceSize = PixelSize;
    return true;
}

void UniverseMapWindow::ReleaseSurface()
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
    SurfaceSize = 0;
}
