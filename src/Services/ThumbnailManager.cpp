#include "Services/ThumbnailManager.h"

#include <algorithm>
#include <cstdlib>

ThumbnailManager::ThumbnailManager(ThumbnailConfiguration& ConfigurationReference, ConfigurationStorage& StorageReference, IProcessMonitor& ProcessMonitorReference,
    IWindowManager& WindowManagerReference, IThumbnailViewFactory& ViewFactoryReference)
    : Configuration(ConfigurationReference)
    , Storage(StorageReference)
    , ProcessMonitorInstance(ProcessMonitorReference)
    , WindowManagerInstance(WindowManagerReference)
    , ViewFactory(ViewFactoryReference)
    , ActiveClient{nullptr, DEFAULT_CLIENT_TITLE}
{
}

ThumbnailManager::~ThumbnailManager()
{
    CloseAllViews();
}

void ThumbnailManager::Start()
{
    HideThumbnailsDelay = Configuration.HideThumbnailsDelay;

    Timer.Start(static_cast<UINT>(Configuration.ThumbnailRefreshPeriod), [this]()
    {
        UpdateThumbnailsList();
        RefreshThumbnails();
    });

    RefreshThumbnails();
}

void ThumbnailManager::Stop()
{
    Timer.Stop();
}

void ThumbnailManager::UpdateThumbnailsSize()
{
    SetThumbnailsSize(Configuration.ThumbnailSize);
}

void ThumbnailManager::UpdateThumbnailFrames()
{
    const EventSuppression Suppression(IgnoreViewEvents);

    for (const std::unique_ptr<IThumbnailView>& View : Views)
    {
        View->SetFrames(Configuration.ShowThumbnailFrames);
    }
}

void ThumbnailManager::ArrangeThumbnails(const ThumbnailArrangement& Arrangement)
{
    std::vector<IThumbnailView*> Targets;
    for (const std::unique_ptr<IThumbnailView>& View : Views)
    {
        if (IsManageableThumbnail(*View) == false || Configuration.IsThumbnailDisabled(View->GetTitle()) == true)
        {
            continue;
        }

        Targets.push_back(View.get());
    }

    const std::vector<Point> Slots = ThumbnailArranger::GetLocations(Arrangement, Configuration.ThumbnailSize, Targets.size());

    std::vector<Point> CurrentLocations;
    for (const IThumbnailView* const Target : Targets)
    {
        CurrentLocations.push_back(Target->GetThumbnailLocation());
    }

    const std::vector<size_t> SlotForTarget = ThumbnailArranger::AssignSlots(CurrentLocations, Slots);

    {
        const EventSuppression Suppression(IgnoreViewEvents);

        for (size_t Index = 0; Index < Targets.size(); Index++)
        {
            const Point Location = Slots[SlotForTarget[Index]];
            Targets[Index]->SetThumbnailLocation(Location);
            Targets[Index]->Refresh(false);
            Configuration.SetThumbnailLocation(Targets[Index]->GetTitle(), ActiveClient.Title, Location);
        }
    }

    Storage.Save();
}

void ThumbnailManager::CloseAllViews()
{
    for (const std::unique_ptr<IThumbnailView>& View : Views)
    {
        DetachCallbacks(*View);
        View->UnregisterHotkey();
        View->Close();
    }

    Views.clear();
}

size_t ThumbnailManager::GetViewCount() const
{
    return Views.size();
}

IThumbnailView* ThumbnailManager::FindView(const HWND Id) const
{
    for (const std::unique_ptr<IThumbnailView>& View : Views)
    {
        if (View->GetId() == Id)
        {
            return View.get();
        }
    }

    return nullptr;
}

bool ThumbnailManager::IsManageableThumbnail(const IThumbnailView& View)
{
    return View.GetTitle() != DEFAULT_CLIENT_TITLE;
}

void ThumbnailManager::AttachCallbacks(IThumbnailView& View)
{
    View.ThumbnailResized = [this](const HWND Id)
    {
        ThumbnailViewResized(Id);
    };
    View.ThumbnailMoved = [this](const HWND Id)
    {
        ThumbnailViewMoved(Id);
    };
    View.ThumbnailFocused = [this](const HWND Id)
    {
        ThumbnailViewFocused(Id);
    };
    View.ThumbnailLostFocus = [this](const HWND Id)
    {
        ThumbnailViewLostFocus(Id);
    };
    View.ThumbnailActivated = [this](const HWND Id)
    {
        ThumbnailActivated(Id);
    };
    View.ThumbnailDeactivated = [this](const HWND Id, const bool SwitchOut)
    {
        ThumbnailDeactivated(Id, SwitchOut);
    };
}

void ThumbnailManager::DetachCallbacks(IThumbnailView& View)
{
    View.ThumbnailResized = nullptr;
    View.ThumbnailMoved = nullptr;
    View.ThumbnailFocused = nullptr;
    View.ThumbnailLostFocus = nullptr;
    View.ThumbnailActivated = nullptr;
    View.ThumbnailDeactivated = nullptr;
}

void ThumbnailManager::UpdateThumbnailsList()
{
    const ProcessUpdate Update = ProcessMonitorInstance.GetUpdatedProcesses();

    std::vector<std::wstring> ViewsAdded;
    std::vector<std::wstring> ViewsRemoved;

    for (const ProcessInfo& Process : Update.Added)
    {
        std::unique_ptr<IThumbnailView> View = ViewFactory.Create(Process.Handle, Process.Title, Configuration.ThumbnailSize);
        View->SetOverlayEnabled(Configuration.ShowThumbnailOverlays);
        View->SetFrames(Configuration.ShowThumbnailFrames);
        // Size limits go after the frames, otherwise the window gets resized needlessly
        View->SetSizeLimitations(Configuration.ThumbnailMinimumSize, Configuration.ThumbnailMaximumSize);
        View->SetTopMost(Configuration.ShowThumbnailsAlwaysOnTop);

        const Point Location = IsManageableThumbnail(*View) == true
            ? Configuration.GetThumbnailLocation(View->GetTitle(), ActiveClient.Title, View->GetThumbnailLocation())
            : Configuration.GetDefaultThumbnailLocation();
        View->SetThumbnailLocation(Location);
        LastLocations[View->GetId()] = View->GetThumbnailLocation();

        AttachCallbacks(*View);

        View->RegisterHotkey(Configuration.GetClientHotkey(View->GetTitle()));

        ApplyClientLayout(View->GetId(), View->GetTitle());

        if (View->GetTitle() != DEFAULT_CLIENT_TITLE)
        {
            ViewsAdded.push_back(View->GetTitle());
        }

        Views.push_back(std::move(View));
    }

    for (const ProcessInfo& Process : Update.Updated)
    {
        IThumbnailView* const View = FindView(Process.Handle);
        if (View == nullptr)
        {
            continue;
        }

        if (Process.Title == View->GetTitle())
        {
            continue;
        }

        if (View->GetTitle() != DEFAULT_CLIENT_TITLE)
        {
            ViewsRemoved.push_back(View->GetTitle());
        }

        View->SetTitle(Process.Title);

        if (View->GetId() == ActiveClient.Handle)
        {
            ActiveClient.Title = Process.Title;
        }

        if (View->GetTitle() != DEFAULT_CLIENT_TITLE)
        {
            ViewsAdded.push_back(View->GetTitle());
        }

        View->RegisterHotkey(Configuration.GetClientHotkey(Process.Title));

        ApplyClientLayout(View->GetId(), View->GetTitle());
    }

    for (const ProcessInfo& Process : Update.Removed)
    {
        for (std::vector<std::unique_ptr<IThumbnailView>>::iterator Current = Views.begin(); Current != Views.end(); ++Current)
        {
            if ((*Current)->GetId() != Process.Handle)
            {
                continue;
            }

            IThumbnailView& View = **Current;
            if (View.GetTitle() != DEFAULT_CLIENT_TITLE)
            {
                ViewsRemoved.push_back(View.GetTitle());
            }

            if (HoverEffectActive == true && HoveredViewId == Process.Handle)
            {
                HoverEffectActive = false;
                HoveredViewId = nullptr;
            }

            if (ActiveClient.Handle == Process.Handle)
            {
                ActiveClient = ActiveClientInfo{nullptr, DEFAULT_CLIENT_TITLE};
            }

            if (ExternalApplication == Process.Handle)
            {
                ExternalApplication = nullptr;
            }

            View.UnregisterHotkey();
            DetachCallbacks(View);
            View.Close();

            LastLocations.erase(Process.Handle);
            Views.erase(Current);
            break;
        }
    }

    if (ViewsAdded.empty() == false || ViewsRemoved.empty() == false)
    {
        ThumbnailListUpdated.Emit(ViewsAdded, ViewsRemoved);
    }
}

void ThumbnailManager::RefreshThumbnails()
{
    const HWND ForegroundWindow = WindowManagerInstance.GetForegroundWindowHandle();

    // The foreground window can be null while a window is losing activation; the system state is undefined then
    if (ForegroundWindow == nullptr)
    {
        return;
    }

    std::wstring ForegroundTitle;

    // The foreground window is one of the clients or their thumbnails
    const bool IsClientWindow = IsClientWindowActive(ForegroundWindow);
    const bool IsMainWindowActive = ProcessMonitorInstance.GetMainProcess().Handle == ForegroundWindow;

    if (ForegroundWindow == ActiveClient.Handle)
    {
        ForegroundTitle = ActiveClient.Title;
    }
    else
    {
        // Only reached by an Alt+Tab switch between clients
        const IThumbnailView* const ForegroundView = FindView(ForegroundWindow);
        if (ForegroundView != nullptr)
        {
            ForegroundTitle = ForegroundView->GetTitle();
        }
        else if (IsClientWindow == false)
        {
            ExternalApplication = ForegroundWindow;
        }
    }

    // Switching out to a non-EVE window (like a thumbnail) must not minimize the clients
    if (ForegroundTitle.empty() == false)
    {
        SwitchActiveClient(ForegroundWindow, ForegroundTitle);
    }

    bool HideAllThumbnails = Configuration.HideThumbnailsOnLostFocus == true && (IsClientWindow == false && IsMainWindowActive == false);

    // Wait a few cycles before hiding all previews
    if (HideAllThumbnails == true)
    {
        HideThumbnailsDelay--;
        if (HideThumbnailsDelay > 0)
        {
            HideAllThumbnails = false;
        }
        else
        {
            HideThumbnailsDelay = 0;
        }
    }
    else
    {
        HideThumbnailsDelay = Configuration.HideThumbnailsDelay;
    }

    RefreshCycleCount++;

    bool ForceRefresh = false;
    if (RefreshCycleCount >= FORCED_REFRESH_CYCLE_THRESHOLD)
    {
        RefreshCycleCount = 0;
        ForceRefresh = true;
    }

    const EventSuppression Suppression(IgnoreViewEvents);

    ReleaseGroupMoveHighlight();

    // No need to touch the thumbnails while one of them is highlighted
    if (HoverEffectActive == false)
    {
        ProcessPendingLocationChange();
    }

    for (const std::unique_ptr<IThumbnailView>& ViewPointer : Views)
    {
        IThumbnailView& View = *ViewPointer;

        if (HideAllThumbnails == true || Configuration.IsThumbnailDisabled(View.GetTitle()) == true)
        {
            if (View.IsActive() == true)
            {
                ReleaseHoverIfHovered(View.GetId());
                View.Hide();
            }

            continue;
        }

        if (Configuration.HideActiveClientThumbnail == true && View.GetId() == ActiveClient.Handle)
        {
            if (View.IsActive() == true)
            {
                ReleaseHoverIfHovered(View.GetId());
                View.Hide();
            }

            continue;
        }

        if (HoverEffectActive == false)
        {
            // Thumbnails with the default caption are not moved
            if (IsManageableThumbnail(View) == true)
            {
                const Point CurrentLocation = View.GetThumbnailLocation();
                const Point WantedLocation = Configuration.GetThumbnailLocation(View.GetTitle(), ActiveClient.Title, CurrentLocation);
                if (WantedLocation.X != CurrentLocation.X || WantedLocation.Y != CurrentLocation.Y)
                {
                    View.SetThumbnailLocation(WantedLocation);
                }
            }

            View.SetOpacity(Configuration.ThumbnailOpacity);
            View.SetTopMost(Configuration.ShowThumbnailsAlwaysOnTop);
        }

        View.SetOverlayEnabled(Configuration.ShowThumbnailOverlays);

        ApplyHighlight(View);

        if (View.IsActive() == false)
        {
            View.Show();
            continue;
        }

        View.Refresh(ForceRefresh);
    }
}

void ThumbnailManager::ApplyHighlight(IThumbnailView& View) const
{
    const bool IsActiveHighlight = Configuration.EnableActiveClientHighlight == true && View.GetId() == ActiveClient.Handle;
    const int Thickness = (IsActiveHighlight == true && GroupMoveActive == false) ? Configuration.ActiveClientHighlightThickness : GROUP_MOVE_HIGHLIGHT_THICKNESS;
    View.SetHighlight(GroupMoveActive == true || IsActiveHighlight == true, Configuration.ActiveClientHighlightColor, Thickness);
}

void ThumbnailManager::ReleaseGroupMoveHighlight()
{
    if (GroupMoveActive == false)
    {
        return;
    }

    GroupMoveIdleCycles--;
    if (GroupMoveIdleCycles > 0)
    {
        return;
    }

    GroupMoveActive = false;
    ApplyHighlightToAll();
}

void ThumbnailManager::ApplyHighlightToAll()
{
    for (const std::unique_ptr<IThumbnailView>& View : Views)
    {
        ApplyHighlight(*View);
    }
}

void ThumbnailManager::ProcessPendingLocationChange()
{
    const std::optional<LocationChange> Change = TryDequeueLocationChange();
    if (Change.has_value() == false)
    {
        return;
    }

    IThumbnailView* const View = FindView(Change->Handle);
    if (Change->ActiveClientTitle == ActiveClient.Title && View != nullptr)
    {
        SnapThumbnailView(*View);
        RaiseThumbnailLocationUpdatedNotification(View->GetTitle());
        return;
    }

    RaiseThumbnailLocationUpdatedNotification(Change->Title);
}

void ThumbnailManager::SetThumbnailsSize(const Size NewSize)
{
    const EventSuppression Suppression(IgnoreViewEvents);

    for (const std::unique_ptr<IThumbnailView>& View : Views)
    {
        View->SetThumbnailSize(NewSize);
        View->Refresh(false);
    }
}

void ThumbnailManager::SwitchActiveClient(const HWND ForegroundClientHandle, const std::wstring& ForegroundClientTitle)
{
    if (ActiveClient.Handle == ForegroundClientHandle)
    {
        return;
    }

    if (Configuration.MinimizeInactiveClients == true && Configuration.IsPriorityClient(ActiveClient.Title) == false)
    {
        WindowManagerInstance.MinimizeWindow(ActiveClient.Handle, false);
    }

    ActiveClient = ActiveClientInfo{ForegroundClientHandle, ForegroundClientTitle};
}

void ThumbnailManager::ThumbnailViewFocused(const HWND Id)
{
    if (HoverEffectActive == true && HoveredViewId == Id)
    {
        return;
    }

    IThumbnailView* const View = FindView(Id);
    if (View == nullptr)
    {
        return;
    }

    ReleaseHoverIfHovered(HoveredViewId);

    HoverEffectActive = true;
    HoveredViewId = Id;

    View->SetTopMost(true);
    View->SetOpacity(1.0);

    if (Configuration.ThumbnailZoomEnabled == true)
    {
        ThumbnailZoomIn(*View);
    }
}

void ThumbnailManager::ReleaseHoverIfHovered(const HWND Id)
{
    if (HoverEffectActive == false || HoveredViewId != Id)
    {
        return;
    }

    ThumbnailViewLostFocus(Id);
}

bool ThumbnailManager::CanActivateExternalApplication() const
{
    return ExternalApplication != nullptr && ::IsWindow(ExternalApplication) != FALSE;
}

void ThumbnailManager::ThumbnailViewLostFocus(const HWND Id)
{
    if (HoverEffectActive == false || HoveredViewId != Id)
    {
        return;
    }

    IThumbnailView* const View = FindView(Id);
    if (View == nullptr)
    {
        return;
    }

    if (Configuration.ThumbnailZoomEnabled == true)
    {
        ThumbnailZoomOut(*View);
    }

    View->SetOpacity(Configuration.ThumbnailOpacity);

    HoverEffectActive = false;
    HoveredViewId = nullptr;
}

void ThumbnailManager::ThumbnailActivated(const HWND Id)
{
    IThumbnailView* const View = FindView(Id);
    if (View == nullptr)
    {
        return;
    }

    WindowManagerInstance.ActivateWindow(View->GetId());

    SwitchActiveClient(View->GetId(), View->GetTitle());
    UpdateClientLayouts();
    RefreshThumbnails();
}

void ThumbnailManager::ThumbnailDeactivated(const HWND Id, const bool SwitchOut)
{
    if (SwitchOut == true)
    {
        if (CanActivateExternalApplication() == true)
        {
            WindowManagerInstance.ActivateWindow(ExternalApplication);
        }

        return;
    }

    const IThumbnailView* const View = FindView(Id);
    if (View == nullptr)
    {
        return;
    }

    WindowManagerInstance.MinimizeWindow(View->GetId(), true);
    RefreshThumbnails();
}

void ThumbnailManager::ThumbnailViewResized(const HWND Id)
{
    if (IgnoreViewEvents == true)
    {
        return;
    }

    IThumbnailView* const View = FindView(Id);
    if (View == nullptr)
    {
        return;
    }

    const Size NewSize = View->GetThumbnailSize();
    SetThumbnailsSize(NewSize);

    View->Refresh(false);

    ThumbnailActiveSizeUpdated.Emit(NewSize);
}

void ThumbnailManager::ThumbnailViewMoved(const HWND Id)
{
    IThumbnailView* const View = FindView(Id);
    if (View == nullptr)
    {
        return;
    }

    const Point NewLocation = View->GetThumbnailLocation();
    const Point PreviousLocation = RememberLocation(Id, NewLocation);

    if (IgnoreViewEvents == true)
    {
        return;
    }

    View->Refresh(false);

    if (Configuration.MoveAllThumbnails == true)
    {
        MoveOtherThumbnails(*View, Point{NewLocation.X - PreviousLocation.X, NewLocation.Y - PreviousLocation.Y});
    }

    EnqueueLocationChange(*View);
}

Point ThumbnailManager::RememberLocation(const HWND Id, const Point NewLocation)
{
    const std::map<HWND, Point>::const_iterator Entry = LastLocations.find(Id);
    const Point Previous = Entry == LastLocations.end() ? NewLocation : Entry->second;
    LastLocations[Id] = NewLocation;
    return Previous;
}

void ThumbnailManager::MoveOtherThumbnails(const IThumbnailView& MovedView, const Point Delta)
{
    if (Delta.X == 0 && Delta.Y == 0)
    {
        return;
    }

    const EventSuppression Suppression(IgnoreViewEvents);

    GroupMoveIdleCycles = DEFAULT_LOCATION_CHANGE_NOTIFICATION_DELAY;
    if (GroupMoveActive == false)
    {
        GroupMoveActive = true;
        ApplyHighlightToAll();
    }

    for (const std::unique_ptr<IThumbnailView>& View : Views)
    {
        if (View->GetId() == MovedView.GetId())
        {
            continue;
        }

        const Point Current = View->GetThumbnailLocation();
        const Point Moved{Current.X + Delta.X, Current.Y + Delta.Y};
        View->SetThumbnailLocation(Moved);
        View->Refresh(false);

        if (IsManageableThumbnail(*View) == true)
        {
            Configuration.SetThumbnailLocation(View->GetTitle(), ActiveClient.Title, Moved);
        }
    }
}

bool ThumbnailManager::IsClientWindowActive(const HWND Window) const
{
    if (Window == nullptr)
    {
        return false;
    }

    for (const std::unique_ptr<IThumbnailView>& View : Views)
    {
        if (View->IsKnownHandle(Window) == true)
        {
            return true;
        }
    }

    return false;
}

void ThumbnailManager::ThumbnailZoomIn(IThumbnailView& View)
{
    const EventSuppression Suppression(IgnoreViewEvents);

    View.ZoomIn(Configuration.ThumbnailZoomAnchor, Configuration.ThumbnailZoomFactor);
    View.Refresh(false);
}

void ThumbnailManager::ThumbnailZoomOut(IThumbnailView& View)
{
    const EventSuppression Suppression(IgnoreViewEvents);

    View.ZoomOut();
    View.Refresh(false);
}

void ThumbnailManager::SnapThumbnailView(IThumbnailView& View)
{
    if (Configuration.EnableThumbnailSnap == false || Configuration.MoveAllThumbnails == true)
    {
        return;
    }

    // Only borderless thumbnails can be docked
    if (Configuration.ShowThumbnailFrames == true)
    {
        return;
    }

    const Size ViewSize = View.GetThumbnailSize();
    const int Width = ViewSize.Width;
    const int Height = ViewSize.Height;

    const Point ViewLocation = View.GetThumbnailLocation();
    const Point ViewPoints[4] = {
        Point{ViewLocation.X, ViewLocation.Y}, Point{ViewLocation.X + Width, ViewLocation.Y},
        Point{ViewLocation.X, ViewLocation.Y + Height}, Point{ViewLocation.X + Width, ViewLocation.Y + Height}};

    const int ThresholdX = std::max(20, Width / 10);
    const int ThresholdY = std::max(20, Height / 10);

    Point BestDelta{0, 0};
    long long BestDistance = -1;

    for (const std::unique_ptr<IThumbnailView>& TestViewPointer : Views)
    {
        const IThumbnailView& TestView = *TestViewPointer;
        if (View.GetId() == TestView.GetId() || TestView.IsActive() == false)
        {
            continue;
        }

        const Size TestSize = TestView.GetThumbnailSize();
        const Point TestLocation = TestView.GetThumbnailLocation();
        const Point TestPoints[4] = {
            Point{TestLocation.X, TestLocation.Y}, Point{TestLocation.X + TestSize.Width, TestLocation.Y},
            Point{TestLocation.X, TestLocation.Y + TestSize.Height}, Point{TestLocation.X + TestSize.Width, TestLocation.Y + TestSize.Height}};

        const Point Delta = TestViewPoints(ViewPoints, TestPoints, ThresholdX, ThresholdY);
        if (Delta.X == 0 && Delta.Y == 0)
        {
            continue;
        }

        const long long Distance = static_cast<long long>(Delta.X) * Delta.X + static_cast<long long>(Delta.Y) * Delta.Y;
        if (BestDistance >= 0 && Distance >= BestDistance)
        {
            continue;
        }

        BestDelta = Delta;
        BestDistance = Distance;
    }

    if (BestDistance < 0)
    {
        return;
    }

    const Point NewLocation{ViewLocation.X + BestDelta.X, ViewLocation.Y + BestDelta.Y};
    View.SetThumbnailLocation(NewLocation);
    Configuration.SetThumbnailLocation(View.GetTitle(), ActiveClient.Title, NewLocation);
}

Point ThumbnailManager::TestViewPoints(const Point (&ViewPoints)[4], const Point (&TestPoints)[4], const int ThresholdX, const int ThresholdY)
{
    // Not all 4x4 corner combinations make sense
    struct CornerPair
    {
        int ViewOffset;
        int TestOffset;
    };

    const CornerPair PAIRS[] = {{0, 3}, {0, 2}, {1, 2}, {0, 1}, {0, 0}, {1, 0}, {2, 1}, {2, 0}, {3, 0}};

    Point Best{0, 0};
    long long BestDistance = -1;
    for (const CornerPair& Pair : PAIRS)
    {
        const Point ViewPoint = ViewPoints[Pair.ViewOffset];
        const Point TestPoint = TestPoints[Pair.TestOffset];

        const int DeltaX = TestPoint.X - ViewPoint.X;
        const int DeltaY = TestPoint.Y - ViewPoint.Y;
        if (std::abs(DeltaX) > ThresholdX || std::abs(DeltaY) > ThresholdY)
        {
            continue;
        }

        const long long Distance = static_cast<long long>(DeltaX) * DeltaX + static_cast<long long>(DeltaY) * DeltaY;
        if (BestDistance >= 0 && Distance >= BestDistance)
        {
            continue;
        }

        Best = Point{DeltaX, DeltaY};
        BestDistance = Distance;
    }

    return Best;
}

void ThumbnailManager::ApplyClientLayout(const HWND ClientHandle, const std::wstring& ClientTitle)
{
    if (Configuration.IsClientLayoutTrackingEnabled() == false)
    {
        return;
    }

    // Clients that are not logged in yet have no layout
    if (ClientTitle == DEFAULT_CLIENT_TITLE)
    {
        return;
    }

    const std::optional<ClientLayout> Layout = Configuration.GetClientLayout(ClientTitle);
    if (Layout.has_value() == false)
    {
        return;
    }

    if (Layout->IsMaximized == true)
    {
        WindowManagerInstance.MaximizeWindow(ClientHandle);
        return;
    }

    WindowManagerInstance.MoveWindow(ClientHandle, Layout->X, Layout->Y, Layout->Width, Layout->Height);
}

void ThumbnailManager::UpdateClientLayouts()
{
    if (Configuration.IsClientLayoutTrackingEnabled() == false)
    {
        return;
    }

    for (const std::unique_ptr<IThumbnailView>& View : Views)
    {
        if (View->GetTitle() == DEFAULT_CLIENT_TITLE)
        {
            continue;
        }

        const RECT Position = WindowManagerInstance.GetWindowPosition(View->GetId());
        const int Width = std::abs(Position.right - Position.left);
        const int Height = std::abs(Position.bottom - Position.top);

        const bool IsMaximized = WindowManagerInstance.IsWindowMaximized(View->GetId());

        if (IsMaximized == false && IsValidWindowPosition(Position.left, Position.top, Width, Height) == false)
        {
            continue;
        }

        Configuration.SetClientLayout(View->GetTitle(), ClientLayout{Position.left, Position.top, Width, Height, IsMaximized});
    }
}

bool ThumbnailManager::IsValidWindowPosition(const int Left, const int Top, const int Width, const int Height)
{
    return Left > WINDOW_POSITION_THRESHOLD_LOW && Left < WINDOW_POSITION_THRESHOLD_HIGH
        && Top > WINDOW_POSITION_THRESHOLD_LOW && Top < WINDOW_POSITION_THRESHOLD_HIGH
        && Width > WINDOW_SIZE_THRESHOLD && Height > WINDOW_SIZE_THRESHOLD;
}

void ThumbnailManager::EnqueueLocationChange(const IThumbnailView& View)
{
    const std::wstring ActiveClientTitle = ActiveClient.Title;
    Configuration.SetThumbnailLocation(View.GetTitle(), ActiveClientTitle, View.GetThumbnailLocation());

    if (EnqueuedLocationChange.Handle == nullptr)
    {
        EnqueuedLocationChange = LocationChange{View.GetId(), View.GetTitle(), ActiveClientTitle, View.GetThumbnailLocation(), DEFAULT_LOCATION_CHANGE_NOTIFICATION_DELAY};
        return;
    }

    if (EnqueuedLocationChange.Handle == View.GetId() && EnqueuedLocationChange.ActiveClientTitle == ActiveClientTitle)
    {
        EnqueuedLocationChange.Delay = DEFAULT_LOCATION_CHANGE_NOTIFICATION_DELAY;
        return;
    }

    RaiseThumbnailLocationUpdatedNotification(EnqueuedLocationChange.Title);
    EnqueuedLocationChange = LocationChange{View.GetId(), View.GetTitle(), ActiveClientTitle, View.GetThumbnailLocation(), DEFAULT_LOCATION_CHANGE_NOTIFICATION_DELAY};
}

std::optional<ThumbnailManager::LocationChange> ThumbnailManager::TryDequeueLocationChange()
{
    if (EnqueuedLocationChange.Handle == nullptr)
    {
        return std::nullopt;
    }

    EnqueuedLocationChange.Delay--;
    if (EnqueuedLocationChange.Delay > 0)
    {
        return std::nullopt;
    }

    const LocationChange Change = EnqueuedLocationChange;
    EnqueuedLocationChange = LocationChange();
    return Change;
}

void ThumbnailManager::RaiseThumbnailLocationUpdatedNotification(const std::wstring& Title)
{
    if (Title.empty() == true || Title == DEFAULT_CLIENT_TITLE)
    {
        return;
    }

    Storage.Save();
}
