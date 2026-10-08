#pragma once

#include <string>
#include <vector>

#include "Application/Signal.h"
#include "Config/ThumbnailArrangement.h"
#include "UI/ITabPage.h"
#include "UI/Widgets.h"

class OrganizerTab : public ITabPage
{
public:
    Signal<const ThumbnailArrangement&> ArrangeRequested;

    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

    void SetDetectedCount(const int Count);

private:
    static constexpr int MINIMUM_CLIENTS = 1;
    static constexpr int MAXIMUM_CLIENTS = 40;
    static constexpr int MAXIMUM_GAP = 200;
    static constexpr int MINIMUM_COORDINATE = -10000;
    static constexpr int MAXIMUM_COORDINATE = 30000;

    int GetClientCount() const;

    // With no client open there is nothing to detect, so the count is typed in and layouts can still be planned and saved
    bool UsesManualCount() const;

    void DrawCountCard();
    void DrawShapeCards(const int Count);
    void DrawPlacementCard();
    void DrawApplyButton();
    void DrawPresets(const int Count);
    void SavePreset(const int Count);
    void ApplyPreset(const size_t Index);

    const std::string Title = "Thumbnail Organizer";
    const std::string Description = "Arrange the previews of your open clients into a grid.";

    bool Enabled = true;
    bool AutoDetect = true;
    int DetectedCount = 0;
    int ManualCount = 4;
    GridShape SelectedShape;
    int Gap = 0;
    int OriginX = 0;
    int OriginY = 0;
    std::string LastApplied;
    std::vector<LayoutPreset> Presets;
    char PresetName[48] = {};
    NumberField CountField;
    NumberField GapField;
    NumberField OriginXField;
    NumberField OriginYField;
};
