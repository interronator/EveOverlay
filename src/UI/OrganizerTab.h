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

    bool IsStandalone() const override;
    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

    void SetDetectedCount(const int Count);
    void SetCharacters(const std::vector<std::string>& NewCharacters);

    // The character in a client window title such as "EVE - Name"; a client still on the login screen has none
    void AddOpenClient(const std::wstring& ClientTitle);
    void RemoveOpenClient(const std::wstring& ClientTitle);

private:
    static constexpr int MINIMUM_CLIENTS = 1;
    static constexpr int MAXIMUM_CLIENTS = 40;
    static constexpr int MAXIMUM_GAP = 200;
    static constexpr int MINIMUM_COORDINATE = -10000;
    static constexpr int MAXIMUM_COORDINATE = 30000;
    static constexpr float SLOT_SPACING = 6.0f;
    static constexpr float SLOT_MINIMUM_WIDTH = 70.0f;
    static constexpr float SLOT_MAXIMUM_WIDTH = 150.0f;

    int GetClientCount() const;

    // With no client open there is nothing to detect, so the count is typed in and layouts can still be planned and saved
    bool UsesManualCount() const;

    void DrawCountCard();
    void DrawShapeCards(const int Count);
    void DrawSlotCard(const int Count);
    void DrawSlotChoices(const size_t Slot);
    void AssignSlot(const size_t Slot, const std::string& Name);

    // The spot another character already holds, or SlotCharacters.size() when there is none
    size_t FindSlotOf(const std::string& Name, const size_t IgnoredSlot) const;

    // Characters with an open client, plus whoever already holds this spot; everyone known when ShowAllCharacters is on
    std::vector<std::string> GetSlotOptions(const size_t Slot) const;
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
    int ManualCount = MINIMUM_CLIENTS;
    GridShape SelectedShape;
    int Gap = 0;
    int OriginX = 0;
    int OriginY = 0;
    bool SmartStart = true;
    std::vector<std::string> SlotCharacters;
    std::vector<std::string> Characters;
    std::vector<std::string> OpenCharacters;
    bool ShowAllCharacters = false;
    std::string LastApplied;
    std::vector<LayoutPreset> Presets;
    char PresetName[48] = {};
    NumberField CountField;
    NumberField GapField;
    NumberField OriginXField;
    NumberField OriginYField;
};
