#pragma once

#include <string>

#include "Application/Signal.h"
#include "UI/ITabPage.h"
#include "UI/SoundPicker.h"
#include "UI/Widgets.h"

// Settings for the alerts raised from the game logs: a preview flashes when its character is being attacked
class AlertsTab : public ITabPage
{
public:
    Signal<> TestSoundRequested;

    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

private:
    static constexpr int MINIMUM_SECONDS = 3;
    static constexpr int MAXIMUM_SECONDS = 120;

    const std::string Title = "Attack Alerts";
    const std::string Description = "Flash a preview when its character is being shot at or tackled.";

    bool Enabled = false;
    bool OnDamage = true;
    bool OnWarpDisruption = true;
    bool SoundEnabled = true;
    int Volume = 70;
    int Seconds = 10;
    std::string SoundPath;
    SliderInputField VolumeField;
    SliderInputField SecondsField;
    SoundPicker Picker;
};
