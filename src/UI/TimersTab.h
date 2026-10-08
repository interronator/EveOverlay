#pragma once

#include <string>

#include "Application/Signal.h"
#include "Config/Hotkey.h"
#include "Services/CountdownTimers.h"
#include "UI/HotkeyField.h"
#include "UI/ITabPage.h"
#include "UI/SoundPicker.h"
#include "UI/Widgets.h"

// Countdowns you start yourself, and the age of your last directional scan
class TimersTab : public ITabPage
{
public:
    // Label, seconds
    Signal<const std::string&, int> StartRequested;
    Signal<int> CancelRequested;
    Signal<> ScanMarkRequested;
    Signal<> ScanClearRequested;
    Signal<> TestSoundRequested;

    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

    // The timers must outlive the tab; they are read every frame
    void SetTimers(const CountdownTimers* const NewTimers);

private:
    static constexpr float FIELD_WIDTH = 190.0f;
    static constexpr int MAXIMUM_QUICK_SECONDS = 86400;
    static constexpr int MAXIMUM_MINUTES = 1440;

    bool DrawOverlayCard();
    bool DrawQuickCard();
    void DrawCustomCard();
    void DrawRunningCard();
    bool DrawSoundCard();

    const std::string Title = "Timers";
    const std::string Description = "Countdowns and the age of your last d-scan.";

    const CountdownTimers* Timers = nullptr;
    bool WindowEnabled = true;
    bool ShowScanAge = true;
    Hotkey ScanHotkey;
    Hotkey QuickHotkey;
    int QuickSeconds = 60;
    bool SoundEnabled = true;
    int Volume = 70;
    std::string SoundPath;

    char CustomLabel[48] = "Timer";
    int CustomMinutes = 5;
    int CustomSeconds = 0;
    NumberField MinutesField;
    NumberField SecondsField;
    SliderInputField QuickField;
    SliderInputField VolumeField;
    SoundPicker Picker;
};
