#pragma once

#include <string>
#include <vector>

#include "Application/Signal.h"
#include "UI/ITabPage.h"

class GeneralTab : public ITabPage
{
public:
    Signal<const std::string&> CharacterRemoved;

    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

    void SetCharacters(const std::vector<std::string>& NewCharacters);
    void SetMainCharacter(const std::string& Name);

private:
    static constexpr float CHARACTER_COMBO_WIDTH = 230.0f;

    bool DrawMainCharacterRow();

    const std::string Title = "General";
    const std::string Description = "How the app and the preview windows behave.";

    std::string MainCharacter;
    std::vector<std::string> Characters;
    bool MinimizeToTray = false;
    bool WindowOnTop = false;
    bool LightTheme = false;
    bool ShowDevelopingTabs = false;
    bool TrackClientLocations = false;
    bool HideActiveClient = false;
    bool MinimizeInactive = false;
    bool AlwaysOnTop = false;
    bool HideOnLostFocus = false;
    bool UniqueLayout = false;
};
