#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "Application/Signal.h"
#include "UI/ITabPage.h"
#include "UI/SoundPicker.h"
#include "UI/Widgets.h"
#include "Universe/IntelHistory.h"
#include "Universe/SystemListing.h"

// Every setting of the universe map lives here; the map window itself only draws
class UniverseTab : public ITabPage
{
public:
    Signal<int> SizePreviewed;
    Signal<int> RotationPreviewed;
    Signal<> TestSoundRequested;
    Signal<> TestKeywordSoundRequested;
    Signal<> HistoryClearRequested;
    Signal<const std::string&> CharacterRemoved;

    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;

    void SetStatus(const std::string& NewStatus);
    void SetClientsOpen(const bool IsOpen);

    void SetCharacters(const std::vector<std::string>& NewCharacters);
    void SetMainCharacter(const std::string& Name);
    void SetLocationCharacter(const std::string& Name);

    // The list must outlive the tab; it is read every frame
    void SetHistory(const std::vector<IntelHistoryEntry>* const NewHistory);

    // Used when the map follows the pilot to another system; the change is saved like any other
    void SetSystem(const std::string& Name);

    // The listing must outlive the tab; it is re-read only when its size changes
    void SetSystemListing(const SystemListing* const NewListing);

    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

    // Remembers a channel that has reported a system on the map so it can be picked again later
    void AddSavedChannel(const std::string& Channel);

    // Used by the tray menu; the setting is saved so the map comes back next session
    void NotifyMapVisible(const bool Visible);

private:
    static constexpr int MINIMUM_JUMPS = 0;
    static constexpr int MAXIMUM_JUMPS = ThumbnailConfiguration::UNIVERSE_MAX_JUMPS;
    static constexpr int MINIMUM_SIZE = 300;
    static constexpr int MAXIMUM_SIZE = 1600;
    static constexpr int MAXIMUM_ROTATION = 359;
    static constexpr int MINIMUM_TIMEOUT = 10;
    static constexpr int MAXIMUM_TIMEOUT = 1800;
    static constexpr float LIST_HEIGHT = 240.0f;
    static constexpr float POPUP_MAX_HEIGHT = 340.0f;
    static constexpr float HISTORY_HEIGHT = 180.0f;
    static constexpr std::chrono::hours FOUND_CHANNEL_AGE{24 * 30};


    // Truncates instead of aborting when the text is longer than the buffer, which a hand-edited config could cause
    static void CopyText(char* const Destination, const size_t DestinationSize, const std::string& Text);

    // Names starting with the typed text come first, then names merely containing it
    void RebuildFilter();

    // Newest reports first, with the keyword matches in the alert colour
    void DrawHistory();

    // Narrows the system dropdown to one region; it is a view filter only and is not saved
    void DrawRegionRow();

    // A dropdown of every known system with a search box at the top of the list; returns true when a system was picked
    bool DrawSystemRow();
    bool DrawLocationCharacterRow();

    bool SelectSystem(const int NameIndex);

    // A dropdown of channels that have worked before, each with a delete button, plus a box to type a new channel name
    bool DrawChannelRow();

    // Whether Name is one of the comma separated channels in the channel box
    bool IsChannelWatched(const std::string& Name) const;

    // Adds the channel to the box, or takes it out again if it is already there
    void ToggleChannel(const std::string& Name);

    const std::string Title = "Intel Watcher";
    const std::string Description = "Choose a system and how far around it to show, with live intel alerts.";

    char SystemName[64] = "Jita";
    char IntelChannel[64] = "Intel";
    char NewChannel[64] = {};
    std::vector<std::string> SavedChannels;
    std::vector<std::string> FoundChannels;
    char Filter[64] = {};
    std::string AppliedFilter;
    bool FilterApplied = false;
    const SystemListing* Listing = nullptr;
    std::string SelectedRegion;
    std::vector<std::string> LowerNames;
    std::vector<int> Matches;
    int Jumps = 3;
    int MapSize = 600;
    int MapRotation = 0;
    bool AlwaysOnTop = true;
    NumberField JumpsField;
    SliderInputField SizeField;
    SliderInputField RotationField;
    SliderInputField VolumeField;
    SliderInputField TimeoutField;
    bool MapVisible = false;
    bool SmartMap = false;
    bool ClientsOpen = false;
    bool SoundEnabled = true;
    int SoundVolume = 70;
    int AlertTimeout = 300;
    std::string SoundPath;
    bool IgnoreClear = true;
    bool ScaleVolume = true;
    bool FollowLocation = false;
    std::string LocationCharacter;
    std::string MainCharacter;
    std::vector<std::string> Characters;
    char Keywords[256] = {};
    std::string KeywordSoundPath;
    const std::vector<IntelHistoryEntry>* History = nullptr;
    SoundPicker AlertSoundPicker;
    std::string Status;
};
