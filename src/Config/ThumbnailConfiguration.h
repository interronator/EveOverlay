#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "Config/ClientLayout.h"
#include "Config/Color.h"
#include "Config/Geometry.h"
#include "Config/Hotkey.h"
#include "Config/ThumbnailArrangement.h"
#include "Config/ZoomAnchor.h"

class ThumbnailConfiguration
{
    friend class ConfigurationStorage;

public:
    bool MinimizeToTray = false;
    int MainWindowWidth = 0;
    int MainWindowHeight = 0;
    bool MainWindowMaximized = false;
    bool MainWindowAlwaysOnTop = false;
    bool LightTheme = false;
    int ThumbnailRefreshPeriod = 500;

    bool EnableCompatibilityMode = false;

    double ThumbnailOpacity = 0.5;

    bool HideActiveClientThumbnail = false;
    bool MinimizeInactiveClients = false;
    bool ShowThumbnailsAlwaysOnTop = true;

    bool HideThumbnailsOnLostFocus = false;
    int HideThumbnailsDelay = 2;

    Size ThumbnailSize{384, 216};
    Size ThumbnailMaximumSize{960, 540};
    Size ThumbnailMinimumSize{192, 108};

    bool EnableThumbnailSnap = true;
    bool MoveAllThumbnails = false;
    bool OrganizerEnabled = true;

    bool ThumbnailZoomEnabled = false;
    int ThumbnailZoomFactor = 2;
    ZoomAnchor ThumbnailZoomAnchor = ZoomAnchor::NW;

    bool ShowThumbnailOverlays = true;
    bool ShowThumbnailFrames = false;

    bool EnableActiveClientHighlight = false;
    Color ActiveClientHighlightColor = Color::FromRgb(0xADFF2F);
    int ActiveClientHighlightThickness = 3;

    std::string UniverseSystem = "Jita";
    // Layout cost grows with the square of the system count: past ten jumps a change freezes the app for over half a second
    static constexpr int UNIVERSE_MAX_JUMPS = 10;
    int UniverseJumps = 3;
    std::string UniverseIntelChannel = "Intel";
    std::vector<std::string> UniverseSavedChannels;
    bool UniverseMapAlwaysOnTop = true;
    int UniverseMapSize = 600;
    int UniverseMapRotation = 0;
    bool UniverseMapVisible = false;
    bool UniverseSmartMap = false;
    bool UniverseAlertSoundEnabled = true;
    int UniverseAlertVolume = 70;
    int UniverseAlertTimeout = 300;
    std::string UniverseAlertSoundPath;

    std::vector<LayoutPreset> LayoutPresets;

    std::string SyncerUserFilePath;
    std::map<long long, std::string> CharacterNames;
    std::map<long long, std::string> AccountNicknames;

    bool IsClientLayoutTrackingEnabled() const;
    void SetClientLayoutTrackingEnabled(const bool Enabled);
    bool IsPerClientThumbnailLayoutsEnabled() const;
    void SetPerClientThumbnailLayoutsEnabled(const bool Enabled);

    // Location for clients that are not manageable yet, e.g. sitting on the login screen
    Point GetDefaultThumbnailLocation() const;

    // Per-client layout first (when enabled and the active client is known), then the flat layout, then the default
    Point GetThumbnailLocation(const std::wstring& CurrentClient, const std::wstring& ActiveClient, const Point DefaultLocation) const;
    void SetThumbnailLocation(const std::wstring& CurrentClient, const std::wstring& ActiveClient, const Point Location);

    std::optional<ClientLayout> GetClientLayout(const std::wstring& CurrentClient) const;
    void SetClientLayout(const std::wstring& CurrentClient, const ClientLayout& Layout);

    Hotkey GetClientHotkey(const std::wstring& CurrentClient) const;
    void SetClientHotkey(const std::wstring& CurrentClient, const Hotkey& Value);

    bool IsPriorityClient(const std::wstring& CurrentClient) const;
    bool IsThumbnailDisabled(const std::wstring& CurrentClient) const;
    void ToggleThumbnail(const std::wstring& CurrentClient, const bool IsDisabled);

    void ApplyRestrictions();

private:
    // A value equal to either bound maps to that bound
    static int Restrict(const int Value, const int Minimum, const int Maximum);

    // A maximum of zero or less means unlimited; it becomes a large bound so every consumer can clamp against it
    static int NormalizeMaximum(const int Maximum, const int Minimum);

    static constexpr int UNLIMITED_THUMBNAIL_DIMENSION = 7680;

    bool ClientLayoutTrackingEnabled = false;
    bool PerClientThumbnailLayoutsEnabled = false;

    std::map<std::wstring, std::map<std::wstring, Point>> PerClientLayout;
    std::map<std::wstring, Point> FlatLayout;
    std::map<std::wstring, ClientLayout> ClientLayouts;
    std::map<std::wstring, std::string> ClientHotkeys;
    std::map<std::wstring, bool> DisabledThumbnails;
    std::vector<std::wstring> PriorityClients;
};
