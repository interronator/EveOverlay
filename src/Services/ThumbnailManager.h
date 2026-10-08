#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <Windows.h>

#include "Config/ConfigurationStorage.h"
#include "Config/ThumbnailConfiguration.h"
#include "Services/AlertSound.h"
#include "Services/GameLogWatcher.h"
#include "Services/GlobalHotkeyWindow.h"
#include "Services/IProcessMonitor.h"
#include "Services/IThumbnailManager.h"
#include "Services/IWindowManager.h"
#include "Services/TimerWindow.h"
#include "Views/IThumbnailView.h"
#include "Views/ThumbnailViewFactory.h"

class ThumbnailManager : public IThumbnailManager
{
public:
    ThumbnailManager(ThumbnailConfiguration& ConfigurationReference, ConfigurationStorage& StorageReference, IProcessMonitor& ProcessMonitorReference,
        IWindowManager& WindowManagerReference, IThumbnailViewFactory& ViewFactoryReference);

    ~ThumbnailManager() override;

    void Start() override;
    void Stop() override;
    void UpdateThumbnailsSize() override;
    void UpdateThumbnailFrames() override;
    void ArrangeThumbnails(const ThumbnailArrangement& Arrangement) override;
    void UpdateHotkeys() override;

    void CloseAllViews();
    size_t GetViewCount() const;

    // The character name in an "EVE - Name" window title; empty for a client still on the login screen
    static std::wstring GetCharacterName(const std::wstring& Title);

    // Whether the preview of this client is currently flashing because of an attack
    bool IsAttackAlertActive(const HWND Id) const;

private:
    static constexpr int ATTACK_BLINK_MS = 400;
    static constexpr int ATTACK_MINIMUM_THICKNESS = 4;
    static constexpr ULONGLONG ATTACK_SOUND_GAP_MS = 4000;

    static constexpr int WINDOW_POSITION_THRESHOLD_LOW = -10000;
    static constexpr int WINDOW_POSITION_THRESHOLD_HIGH = 31000;
    static constexpr int WINDOW_SIZE_THRESHOLD = 10;
    static constexpr int FORCED_REFRESH_CYCLE_THRESHOLD = 2;
    static constexpr int DEFAULT_LOCATION_CHANGE_NOTIFICATION_DELAY = 2;
    static constexpr int GROUP_MOVE_HIGHLIGHT_THICKNESS = 1;
    static constexpr const wchar_t* DEFAULT_CLIENT_TITLE = L"EVE";

    class EventSuppression
    {
    public:
        explicit EventSuppression(bool& FlagReference)
            : Flag(FlagReference)
            , Previous(FlagReference)
        {
            Flag = true;
        }

        EventSuppression(const EventSuppression&) = delete;
        EventSuppression& operator=(const EventSuppression&) = delete;

        ~EventSuppression()
        {
            Flag = Previous;
        }

    private:
        bool& Flag;
        const bool Previous;
    };

    struct ActiveClientInfo
    {
        HWND Handle;
        std::wstring Title;
    };

    struct LocationChange
    {
        HWND Handle = nullptr;
        std::wstring Title;
        std::wstring ActiveClientTitle;
        Point Location;
        int Delay = -1;
    };

    // Views of clients sitting on the login screen are not managed
    static bool IsManageableThumbnail(const IThumbnailView& View);

    static void DetachCallbacks(IThumbnailView& View);

    // Of all corner pairs close enough to dock, the offset that moves the least
    static Point TestViewPoints(const Point (&ViewPoints)[4], const Point (&TestPoints)[4], const int ThresholdX, const int ThresholdY);

    // Quick sanity check that the window is not minimized
    static bool IsValidWindowPosition(const int Left, const int Top, const int Width, const int Height);

    IThumbnailView* FindView(const HWND Id) const;
    void AttachCallbacks(IThumbnailView& View);
    void UpdateThumbnailsList();
    std::unique_ptr<IThumbnailView> CreateView(const ProcessInfo& Process);
    void RefreshThumbnails();
    void ApplyHighlight(IThumbnailView& View) const;
    void CycleClients(const size_t GroupIndex, const int Direction);
    void TogglePreviews();
    void MinimizeAllClients();
    std::vector<IThumbnailView*> GetCycleOrder(const CycleGroup& Group) const;
    void PollGameLogs();
    void ExpireAttackAlerts();
    void RaiseAttackAlert(const GameLogEvent& Event);
    void ApplyHighlightToAll();
    void ReleaseGroupMoveHighlight();
    void ProcessPendingLocationChange();
    void SetThumbnailsSize(const Size NewSize);
    void SwitchActiveClient(const HWND ForegroundClientHandle, const std::wstring& ForegroundClientTitle);
    void ThumbnailViewFocused(const HWND Id);
    void ThumbnailViewLostFocus(const HWND Id);
    void ReleaseHoverIfHovered(const HWND Id);
    bool CanActivateExternalApplication() const;

    // Activation runs on the UI thread: SetForegroundWindow is quick and the follow-up work needs the result anyway
    void ThumbnailActivated(const HWND Id);

    void ThumbnailDeactivated(const HWND Id, const bool SwitchOut);
    void ThumbnailViewResized(const HWND Id);
    void ThumbnailViewMoved(const HWND Id);

    // Returns where the thumbnail was before this move; a thumbnail seen for the first time has not moved
    Point RememberLocation(const HWND Id, const Point NewLocation);

    void MoveOtherThumbnails(const IThumbnailView& MovedView, const Point Delta);
    bool IsClientWindowActive(const HWND Window) const;
    void ThumbnailZoomIn(IThumbnailView& View);
    void ThumbnailZoomOut(IThumbnailView& View);

    // Moves the thumbnail onto a neighbouring one when two corners are close enough
    void SnapThumbnailView(IThumbnailView& View);

    void ApplyClientLayout(const HWND ClientHandle, const std::wstring& ClientTitle);
    void UpdateClientLayouts();

    // The new location is remembered at once but saved only after the thumbnail stops moving
    void EnqueueLocationChange(const IThumbnailView& View);

    std::optional<LocationChange> TryDequeueLocationChange();
    void RaiseThumbnailLocationUpdatedNotification(const std::wstring& Title);

    ThumbnailConfiguration& Configuration;
    ConfigurationStorage& Storage;
    IProcessMonitor& ProcessMonitorInstance;
    IWindowManager& WindowManagerInstance;
    IThumbnailViewFactory& ViewFactory;

    TimerWindow Timer;
    GlobalHotkeyWindow GlobalHotkeys;
    bool PreviewsHidden = false;
    GameLogWatcher GameLogs;
    AlertSound AttackSound;
    std::map<HWND, ULONGLONG> AttackAlerts;
    ULONGLONG LastAttackSoundTick = 0;
    bool GameLogsFollowed = false;
    std::vector<std::unique_ptr<IThumbnailView>> Views;
    std::map<HWND, Point> LastLocations;

    ActiveClientInfo ActiveClient;
    HWND ExternalApplication = nullptr;
    LocationChange EnqueuedLocationChange;

    bool IgnoreViewEvents = false;
    bool HoverEffectActive = false;
    HWND HoveredViewId = nullptr;
    bool GroupMoveActive = false;
    int GroupMoveIdleCycles = 0;
    int RefreshCycleCount = 0;
    int HideThumbnailsDelay = 0;
};
