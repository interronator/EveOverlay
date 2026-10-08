#include "Config/ThumbnailConfiguration.h"

#include <algorithm>
#include <cmath>

bool ThumbnailConfiguration::IsClientLayoutTrackingEnabled() const
{
    return ClientLayoutTrackingEnabled;
}

void ThumbnailConfiguration::SetClientLayoutTrackingEnabled(const bool Enabled)
{
    if (Enabled == false)
    {
        ClientLayouts.clear();
    }

    ClientLayoutTrackingEnabled = Enabled;
}

bool ThumbnailConfiguration::IsPerClientThumbnailLayoutsEnabled() const
{
    return PerClientThumbnailLayoutsEnabled;
}

void ThumbnailConfiguration::SetPerClientThumbnailLayoutsEnabled(const bool Enabled)
{
    if (Enabled == false)
    {
        PerClientLayout.clear();
    }

    PerClientThumbnailLayoutsEnabled = Enabled;
}

Point ThumbnailConfiguration::GetDefaultThumbnailLocation() const
{
    return Point{5, 5};
}

Point ThumbnailConfiguration::GetThumbnailLocation(const std::wstring& CurrentClient, const std::wstring& ActiveClient, const Point DefaultLocation) const
{
    if (PerClientThumbnailLayoutsEnabled == true && ActiveClient.empty() == false)
    {
        const std::map<std::wstring, std::map<std::wstring, Point>>::const_iterator Layout = PerClientLayout.find(ActiveClient);
        if (Layout != PerClientLayout.end())
        {
            const std::map<std::wstring, Point>::const_iterator Location = Layout->second.find(CurrentClient);
            if (Location != Layout->second.end())
            {
                return Location->second;
            }
        }
    }

    const std::map<std::wstring, Point>::const_iterator Location = FlatLayout.find(CurrentClient);
    if (Location == FlatLayout.end())
    {
        return DefaultLocation;
    }

    return Location->second;
}

void ThumbnailConfiguration::SetThumbnailLocation(const std::wstring& CurrentClient, const std::wstring& ActiveClient, const Point Location)
{
    if (PerClientThumbnailLayoutsEnabled == false)
    {
        FlatLayout[CurrentClient] = Location;
        return;
    }

    if (ActiveClient.empty() == true)
    {
        return;
    }

    PerClientLayout[ActiveClient][CurrentClient] = Location;
}

std::optional<ClientLayout> ThumbnailConfiguration::GetClientLayout(const std::wstring& CurrentClient) const
{
    const std::map<std::wstring, ClientLayout>::const_iterator Layout = ClientLayouts.find(CurrentClient);
    if (Layout == ClientLayouts.end())
    {
        return std::nullopt;
    }

    return Layout->second;
}

void ThumbnailConfiguration::SetClientLayout(const std::wstring& CurrentClient, const ClientLayout& Layout)
{
    ClientLayouts[CurrentClient] = Layout;
}

Hotkey ThumbnailConfiguration::GetClientHotkey(const std::wstring& CurrentClient) const
{
    const std::map<std::wstring, std::string>::const_iterator Entry = ClientHotkeys.find(CurrentClient);
    if (Entry == ClientHotkeys.end())
    {
        return Hotkey();
    }

    return Hotkey::Parse(Entry->second);
}

void ThumbnailConfiguration::SetClientHotkey(const std::wstring& CurrentClient, const Hotkey& Value)
{
    ClientHotkeys[CurrentClient] = Value.ToString();
}

bool ThumbnailConfiguration::IsPriorityClient(const std::wstring& CurrentClient) const
{
    for (const std::wstring& Client : PriorityClients)
    {
        if (Client == CurrentClient)
        {
            return true;
        }
    }

    return false;
}

bool ThumbnailConfiguration::IsThumbnailDisabled(const std::wstring& CurrentClient) const
{
    const std::map<std::wstring, bool>::const_iterator Entry = DisabledThumbnails.find(CurrentClient);
    if (Entry == DisabledThumbnails.end())
    {
        return false;
    }

    return Entry->second;
}

void ThumbnailConfiguration::ToggleThumbnail(const std::wstring& CurrentClient, const bool IsDisabled)
{
    DisabledThumbnails[CurrentClient] = IsDisabled;
}

void ThumbnailConfiguration::ApplyRestrictions()
{
    ThumbnailRefreshPeriod = Restrict(ThumbnailRefreshPeriod, 300, 1000);
    MainWindowWidth = Restrict(MainWindowWidth, 0, 10000);
    MainWindowHeight = Restrict(MainWindowHeight, 0, 10000);
    ThumbnailMinimumSize = Size{std::max(ThumbnailMinimumSize.Width, 1), std::max(ThumbnailMinimumSize.Height, 1)};
    ThumbnailMaximumSize = Size{
        NormalizeMaximum(ThumbnailMaximumSize.Width, ThumbnailMinimumSize.Width),
        NormalizeMaximum(ThumbnailMaximumSize.Height, ThumbnailMinimumSize.Height)};
    ThumbnailSize = Size{
        Restrict(ThumbnailSize.Width, ThumbnailMinimumSize.Width, ThumbnailMaximumSize.Width),
        Restrict(ThumbnailSize.Height, ThumbnailMinimumSize.Height, ThumbnailMaximumSize.Height)};
    // Rounded rather than truncated: 0.29 * 100.0 is 28.999..., which would lose a percent on every reload
    ThumbnailOpacity = Restrict(static_cast<int>(std::lround(ThumbnailOpacity * 100.0)), 20, 100) / 100.0;
    ThumbnailZoomFactor = Restrict(ThumbnailZoomFactor, 2, 10);
    ActiveClientHighlightThickness = Restrict(ActiveClientHighlightThickness, 1, 6);
    UniverseJumps = Restrict(UniverseJumps, 0, UNIVERSE_MAX_JUMPS);
    UniverseMapSize = Restrict(UniverseMapSize, 300, 1600);
    UniverseMapRotation = Restrict(UniverseMapRotation, 0, 359);
    UniverseAlertVolume = Restrict(UniverseAlertVolume, 0, 100);

    for (LayoutPreset& Preset : LayoutPresets)
    {
        Preset.ClientCount = Restrict(Preset.ClientCount, 1, 40);
        Preset.Arrangement.Shape.Columns = Restrict(Preset.Arrangement.Shape.Columns, 1, 40);
        Preset.Arrangement.Shape.Rows = Restrict(Preset.Arrangement.Shape.Rows, 1, 40);
        Preset.Arrangement.Gap = Restrict(Preset.Arrangement.Gap, 0, 200);
    }
    UniverseAlertTimeout = Restrict(UniverseAlertTimeout, 10, 1800);
}

int ThumbnailConfiguration::Restrict(const int Value, const int Minimum, const int Maximum)
{
    if (Value <= Minimum)
    {
        return Minimum;
    }

    if (Value >= Maximum)
    {
        return Maximum;
    }

    return Value;
}

int ThumbnailConfiguration::NormalizeMaximum(const int Maximum, const int Minimum)
{
    if (Maximum <= 0)
    {
        return std::max(UNLIMITED_THUMBNAIL_DIMENSION, Minimum);
    }

    return std::max(Maximum, Minimum);
}
