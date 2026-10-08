#pragma once

#include <string>

#include "UI/ITabPage.h"

class GeneralTab : public ITabPage
{
public:
    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

private:
    const std::string Title = "General";
    const std::string Description = "How the app and the preview windows behave.";

    bool MinimizeToTray = false;
    bool WindowOnTop = false;
    bool LightTheme = false;
    bool TrackClientLocations = false;
    bool HideActiveClient = false;
    bool MinimizeInactive = false;
    bool AlwaysOnTop = false;
    bool HideOnLostFocus = false;
    bool UniqueLayout = false;
};
