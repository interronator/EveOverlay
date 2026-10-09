#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#include "Application/Signal.h"
#include "Config/ThumbnailConfiguration.h"
#include "Universe/ChatLogWatcher.h"
#include "Universe/IntelHistory.h"
#include "Services/AlertSound.h"
#include "Universe/ShipCatalog.h"
#include "Universe/SystemListing.h"
#include "Universe/UniverseData.h"
#include "Universe/UniverseMapOptions.h"
#include "Views/UniverseMapView.h"

// A borderless, per-pixel transparent overlay. Only the parts of the map that are drawn receive the mouse, so they can be
// grabbed to drag the map; everywhere else clicks fall through to whatever is underneath. All settings come from the main page.
class UniverseMapWindow : public CWindowImpl<UniverseMapWindow, CWindow, CWinTraits<WS_POPUP, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE>>
{
public:
    DECLARE_WND_CLASS_EX(L"EveOverlayUniverseMapWindow", 0, 0)

    BEGIN_MSG_MAP(UniverseMapWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_NCHITTEST, OnHitTest)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
        MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
        MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
    END_MSG_MAP()

    // Emitted the first time a channel reports a system on the map, so the settings page can remember it
    Signal<const std::string&> IntelChannelWorked;

    // Emitted with the new home system when "follow my location" moves the map
    Signal<const std::string&> HomeSystemFollowed;

    // The sound is quieter for systems further from home: full volume at home, falling to this share at the edge of the map
    static constexpr float FAR_VOLUME_SHARE = 0.4f;
    static constexpr size_t MAX_HISTORY = 200;

    static float DistanceVolumeScale(const int Jumps, const int MaxJumps);

    UniverseMapWindow(std::filesystem::path DataPath, std::filesystem::path SettingsFilePath, std::filesystem::path JumpBridgesPath);
    UniverseMapWindow(const UniverseMapWindow&) = delete;
    UniverseMapWindow& operator=(const UniverseMapWindow&) = delete;

    ~UniverseMapWindow();

    void Configure(const UniverseMapOptions& Options);

    // Also used by the settings page's test button, so it ignores the enabled flag
    void PlayAlert();
    void PlayKeywordAlert();

    // Live preview while the size slider is dragged; the value is saved separately once the drag ends
    void SetMapSize(const int MapSize);

    // Live preview while the rotation slider is dragged
    void SetMapRotation(const int Degrees);

    void ShowMap();
    void HideMap();
    const std::string& GetStatus() const;

    // Empty until the data has loaded
    const SystemListing& GetSystemListing() const;

    // Newest report first; the revision changes whenever the list does, so a viewer knows when to look again
    const std::vector<IntelHistoryEntry>& GetHistory() const;
    unsigned GetHistoryRevision() const;
    void ClearHistory();

private:
    static constexpr const wchar_t* SETTINGS_SECTION = L"UniverseMap";
    static constexpr UINT_PTR POLL_TIMER_ID = 1;
    static constexpr UINT_PTR MOVE_TIMER_ID = 2;
    static constexpr UINT MOVE_POLL_INTERVAL_MS = 50;
    static constexpr ULONGLONG MIN_ALERT_GAP_MS = 3000;
    static constexpr UINT POLL_INTERVAL_MS = 1000;
    static constexpr UINT_PTR ANIMATION_TIMER_ID = 3;
    static constexpr UINT ANIMATION_INTERVAL_MS = 33;
    static constexpr double FLASH_START_HZ = 3.0;
    static constexpr double FLASH_END_HZ = 0.4;
    static constexpr ULONGLONG CLEAR_PULSE_MS = 5000;
    static constexpr double CLEAR_PULSE_HZ = 1.5;
    static constexpr int MAX_JUMPS = ThumbnailConfiguration::UNIVERSE_MAX_JUMPS;
    static constexpr int MINIMUM_SIZE = 200;

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


    // Position in the flash cycle after ElapsedSeconds. The flash rate falls linearly from FLASH_START_HZ to FLASH_END_HZ over
    // the alert, so the phase is the integral of that rate: it stays continuous while the flashing slows down.
    static float FlashCycles(const double ElapsedSeconds, const double TotalSeconds);

    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnTimer(UINT, WPARAM Parameter, LPARAM, BOOL& Handled);

    // Only reachable while Alt is held, because the window is click-through otherwise
    LRESULT OnHitTest(UINT, WPARAM, LPARAM, BOOL&);

    LRESULT OnEnterSizeMove(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMouseActivate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnExitSizeMove(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL&);

    // Alt can only be polled: the game owns the keyboard focus, so this window never sees the key
    void UpdateMoveMode();

    int ReadSetting(const wchar_t* const Key, const int Fallback) const;
    void WriteSetting(const wchar_t* const Key, const int Value) const;
    int GetPixelSize() const;
    void ApplyWindowState();
    void RestorePosition();
    void SavePosition() const;
    void RebuildNeighborhood();
    void LoadUniverse();

    // The links are part of the loaded data, so a change to the list or to the setting needs the data loaded again
    void ReloadIfBridgesChanged();
    std::vector<UniverseMapView::NodeAlert> BuildHighlights() const;
    void AnimateAlerts();
    void PollIntel();
    void PollLocation();
    void FollowSystem(const std::string& SystemName);
    void RecordReport(const ChatMessage& Message, const int System, const bool Priority);
    void PlayReportSound(const bool Priority, const int ClosestJumps);

    // GDI+ draws straight into a premultiplied 32-bit surface, which UpdateLayeredWindow composites with per-pixel alpha
    void Redraw();

    // The pixel surface is kept between frames and only rebuilt when the map size changes
    bool EnsureSurface(const HDC ScreenDc, const int PixelSize);
    void ReleaseSurface();

    HDC SurfaceDc = nullptr;
    HBITMAP Surface = nullptr;
    HGDIOBJ SurfaceOriginalBitmap = nullptr;
    void* SurfacePixels = nullptr;
    int SurfaceSize = 0;

    GdiplusSession Gdi;
    std::filesystem::path CsvPath;
    std::filesystem::path SettingsPath;
    UniverseData Data;
    bool Loaded = false;
    UniverseMapView View;
    ChatLogWatcher Watcher;
    ChatLogWatcher LocalWatcher;
    SystemListing Listing;
    std::vector<int> NodeSystems;
    std::unordered_map<int, int> SystemJumps;
    std::vector<IntelHistoryEntry> History;
    unsigned HistoryRevision = 0;
    std::vector<std::string> Keywords;
    std::wstring KeywordSoundPath;
    bool IgnoreClear = true;
    bool ScaleVolumeByDistance = true;
    bool FollowLocation = false;
    std::string LocationCharacter;
    bool FollowCheckPending = false;
    bool UseJumpBridges = true;
    bool BridgesApplied = false;
    std::filesystem::path BridgesPath;
    std::filesystem::file_time_type BridgesStamp;
    int BridgeCount = 0;
    std::unordered_set<int> Candidates;
    std::unordered_set<std::string> WorkedChannels;
    std::unordered_map<int, ULONGLONG> AlertStart;
    std::unordered_map<int, ULONGLONG> ClearStart;
    std::unordered_map<int, ULONGLONG> CautionStart;
    ULONGLONG AlertDurationMs = 300000;
    std::string Center = "Jita";
    std::string Channel = "Intel";
    std::string Status;
    int MaxJumps = 3;
    int Size = 600;
    bool TopMost = true;
    bool SoundEnabled = true;
    int SoundVolume = 70;
    std::wstring SoundPath;
    AlertSound Sound;
    ULONGLONG LastAlertTick = 0;
    bool MoveMode = false;
    bool Dragging = false;
};
