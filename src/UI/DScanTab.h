#pragma once

#include <string>

#include "Application/Signal.h"
#include "UI/ITabPage.h"
#include "UI/Widgets.h"
#include "Universe/DScanAnalyzer.h"

// Settings for the d-scan reader, and the details of the last scan it read
class DScanTab : public ITabPage
{
public:
    Signal<> ReadNowRequested;

    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

    // The result must outlive the tab; it is read every frame. Null until a scan has been read.
    void SetResult(const DScanResult* const NewResult);
    void SetStatus(const std::string& NewStatus);

private:
    static constexpr int MINIMUM_SECONDS = 5;
    static constexpr int MAXIMUM_SECONDS = 120;

    void DrawResult();

    const std::string Title = "D-Scan";
    const std::string Description = "Summarise a directional scan you copied from the game.";

    const DScanResult* Result = nullptr;
    std::string Status;
    bool AutoRead = false;
    int ShowSeconds = 30;
    bool MarksScanAge = true;
    SliderInputField SecondsField;
};
