#include "UI/GeneralTab.h"

#include "UI/Widgets.h"

const std::string& GeneralTab::GetTitle() const
{
    return Title;
}

const std::string& GeneralTab::GetDescription() const
{
    return Description;
}

void GeneralTab::Draw()
{
    bool Changed = false;

    Widgets::BeginCard("##Behavior");
    Changed = Widgets::ToggleRow("Minimize to system tray", MinimizeToTray) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Keep this window on top", WindowOnTop) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Light theme", LightTheme) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Enable Tabs in Development", ShowDevelopingTabs) == true || Changed == true;
    Widgets::HoverTip("Shows the newer tabs that are still being worked on: Timers and Attack Alerts.");
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Track client locations", TrackClientLocations) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Unique layout for each EVE client", UniqueLayout) == true || Changed == true;
    Widgets::EndCard();

    Widgets::SectionLabel("PREVIEWS");
    Widgets::BeginCard("##Previews");
    Changed = Widgets::ToggleRow("Previews always on top", AlwaysOnTop) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Hide preview of active EVE client", HideActiveClient) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Hide previews when EVE client is not active", HideOnLostFocus) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Minimize inactive EVE clients", MinimizeInactive) == true || Changed == true;
    Widgets::EndCard();

    if (Changed == true)
    {
        SettingsChanged.Emit();
    }
}

void GeneralTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    MinimizeToTray = Configuration.MinimizeToTray;
    WindowOnTop = Configuration.MainWindowAlwaysOnTop;
    LightTheme = Configuration.LightTheme;
    ShowDevelopingTabs = Configuration.ShowDevelopingTabs;
    TrackClientLocations = Configuration.IsClientLayoutTrackingEnabled();
    HideActiveClient = Configuration.HideActiveClientThumbnail;
    MinimizeInactive = Configuration.MinimizeInactiveClients;
    AlwaysOnTop = Configuration.ShowThumbnailsAlwaysOnTop;
    HideOnLostFocus = Configuration.HideThumbnailsOnLostFocus;
    UniqueLayout = Configuration.IsPerClientThumbnailLayoutsEnabled();
}

void GeneralTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.MinimizeToTray = MinimizeToTray;
    Configuration.MainWindowAlwaysOnTop = WindowOnTop;
    Configuration.LightTheme = LightTheme;
    Configuration.ShowDevelopingTabs = ShowDevelopingTabs;
    Configuration.SetClientLayoutTrackingEnabled(TrackClientLocations);
    Configuration.HideActiveClientThumbnail = HideActiveClient;
    Configuration.MinimizeInactiveClients = MinimizeInactive;
    Configuration.ShowThumbnailsAlwaysOnTop = AlwaysOnTop;
    Configuration.HideThumbnailsOnLostFocus = HideOnLostFocus;
    Configuration.SetPerClientThumbnailLayoutsEnabled(UniqueLayout);
}
