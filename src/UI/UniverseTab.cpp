#include "UI/UniverseTab.h"

#include <cctype>
#include <cfloat>
#include <cstring>
#include <filesystem>

#include <Windows.h>
#include <commdlg.h>

#include "Config/TextUtil.h"
#include "Services/AlertSound.h"
#include "Universe/ChatLogWatcher.h"
#include "UI/Theme.h"

const std::string& UniverseTab::GetTitle() const
{
    return Title;
}

const std::string& UniverseTab::GetDescription() const
{
    return Description;
}

void UniverseTab::SetStatus(const std::string& NewStatus)
{
    Status = NewStatus;
}

void UniverseTab::SetClientsOpen(const bool IsOpen)
{
    ClientsOpen = IsOpen;
}

void UniverseTab::SetSystemListing(const SystemListing* const NewListing)
{
    if (NewListing == Listing && NewListing != nullptr && NewListing->Names.size() == LowerNames.size())
    {
        return;
    }

    Listing = NewListing;
    LowerNames.clear();
    if (NewListing != nullptr)
    {
        for (const std::string& Name : NewListing->Names)
        {
            LowerNames.push_back(TextUtil::ToLower(Name));
        }
    }

    FilterApplied = false;
}

void UniverseTab::Draw()
{
    bool Changed = false;

    Widgets::BeginCard("##UniverseMap");

    Changed = Widgets::ToggleRow("Show map", MapVisible) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Smart map (only show reported routes)", SmartMap) == true || Changed == true;
    Widgets::RowDivider();
    DrawRegionRow();
    Widgets::RowDivider();
    Changed = DrawSystemRow() == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Follow my location (from Local chat)", FollowLocation) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Count jump bridges as one jump", UseJumpBridges) == true || Changed == true;
    Widgets::RowDivider();
    Widgets::RowLabel("Jump bridge list", Theme::Px(190.0f));
    if (ImGui::Button("Open Jump Bridges.txt", ImVec2(Theme::Px(190.0f), 0.0f)) == true)
    {
        OpenJumpBridgesRequested.Emit();
    }

    Widgets::EndRow();
    Widgets::RowDivider();
    Changed = Widgets::NumberRow("Jumps", JumpsField, Jumps, MINIMUM_JUMPS, MAXIMUM_JUMPS, 1) == true || Changed == true;
    Widgets::RowDivider();
    Changed = DrawChannelRow() == true || Changed == true;
    Widgets::RowDivider();
    const SliderInputField::Result SizeResult = Widgets::SliderInputRow("Map size", SizeField, MapSize, MINIMUM_SIZE, MAXIMUM_SIZE);
    if (SizeResult.Changed == true)
    {
        SizePreviewed.Emit(MapSize);
    }

    Changed = SizeResult.Committed == true || Changed == true;
    Widgets::RowDivider();
    const SliderInputField::Result RotationResult = Widgets::SliderInputRow("Map rotation (degrees)", RotationField, MapRotation, 0, MAXIMUM_ROTATION);
    if (RotationResult.Changed == true)
    {
        RotationPreviewed.Emit(MapRotation);
    }

    Changed = RotationResult.Committed == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Map always on top", AlwaysOnTop) == true || Changed == true;
    Widgets::EndCard();

    Widgets::SectionLabel("INTEL ALERT");
    Widgets::BeginCard("##UniverseSound");
    Changed = Widgets::SliderInputRow("Red pulse duration (seconds)", TimeoutField, AlertTimeout, MINIMUM_TIMEOUT, MAXIMUM_TIMEOUT).Committed == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Play a sound when a system in range is reported", SoundEnabled) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::SliderInputRow("Volume", VolumeField, SoundVolume, 0, 100).Committed == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Quieter sound for distant systems", ScaleVolume) == true || Changed == true;
    Widgets::RowDivider();
    bool TestPressed = false;
    Changed = AlertSoundPicker.Draw("Sound", "##Sound", SoundPath, false, TestPressed) == true || Changed == true;
    if (TestPressed == true)
    {
        TestSoundRequested.Emit();
    }

    Widgets::EndCard();

    Widgets::SectionLabel("FILTERING AND PRIORITY");
    Widgets::BeginCard("##UniverseFilter");
    Changed = Widgets::ToggleRow("Ignore clear reports and questions", IgnoreClear) == true || Changed == true;
    Widgets::RowDivider();

    const float KeywordWidth = Theme::Px(260.0f);
    Widgets::RowLabel("Priority keywords", KeywordWidth);
    ImGui::SetNextItemWidth(KeywordWidth);
    ImGui::InputTextWithHint("##Keywords", "e.g. bubble, camp, cyno", Keywords, sizeof(Keywords));
    Changed = ImGui::IsItemDeactivatedAfterEdit() == true || Changed == true;
    Widgets::EndRow();
    Widgets::RowDivider();
    Changed = AlertSoundPicker.Draw("Priority sound", "##KeywordSound", KeywordSoundPath, true, TestPressed) == true || Changed == true;
    if (TestPressed == true)
    {
        TestKeywordSoundRequested.Emit();
    }
    Widgets::EndCard();

    Widgets::SectionLabel("RECENT REPORTS");
    DrawHistory();

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(10.0f)));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextWrapped("%s", Status.c_str());
    if (MapVisible == true && ClientsOpen == false)
    {
        ImGui::TextWrapped("Waiting for an EVE client to open before showing the map.");
    }

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(4.0f)));
    ImGui::TextWrapped("The map is drawn with a transparent background. Hold Alt to show a frame around the map, then click and drag inside it to move it. Systems named in the intel channel flash red, slowing down until the red pulse duration runs out.");
    ImGui::PopStyleColor();

    if (Changed == true)
    {
        SettingsChanged.Emit();
    }
}

void UniverseTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    CopyText(SystemName, sizeof(SystemName), Configuration.UniverseSystem);
    CopyText(IntelChannel, sizeof(IntelChannel), Configuration.UniverseIntelChannel);
    SavedChannels = Configuration.UniverseSavedChannels;
    Jumps = Configuration.UniverseJumps;
    MapSize = Configuration.UniverseMapSize;
    MapRotation = Configuration.UniverseMapRotation;
    AlwaysOnTop = Configuration.UniverseMapAlwaysOnTop;
    MapVisible = Configuration.UniverseMapVisible;
    SmartMap = Configuration.UniverseSmartMap;
    SoundEnabled = Configuration.UniverseAlertSoundEnabled;
    SoundVolume = Configuration.UniverseAlertVolume;
    AlertTimeout = Configuration.UniverseAlertTimeout;
    SoundPath = Configuration.UniverseAlertSoundPath;
    IgnoreClear = Configuration.UniverseIgnoreClear;
    ScaleVolume = Configuration.UniverseScaleVolumeByDistance;
    FollowLocation = Configuration.UniverseFollowLocation;
    UseJumpBridges = Configuration.UniverseUseJumpBridges;
    CopyText(Keywords, sizeof(Keywords), Configuration.UniverseKeywords);
    KeywordSoundPath = Configuration.UniverseKeywordSoundPath;
}

void UniverseTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.UniverseSystem = TextUtil::Trim(SystemName);
    Configuration.UniverseIntelChannel = TextUtil::Trim(IntelChannel);
    Configuration.UniverseSavedChannels = SavedChannels;
    Configuration.UniverseJumps = Jumps;
    Configuration.UniverseMapSize = MapSize;
    Configuration.UniverseMapRotation = MapRotation;
    Configuration.UniverseMapAlwaysOnTop = AlwaysOnTop;
    Configuration.UniverseMapVisible = MapVisible;
    Configuration.UniverseSmartMap = SmartMap;
    Configuration.UniverseAlertSoundEnabled = SoundEnabled;
    Configuration.UniverseAlertVolume = SoundVolume;
    Configuration.UniverseAlertTimeout = AlertTimeout;
    Configuration.UniverseAlertSoundPath = SoundPath;
    Configuration.UniverseIgnoreClear = IgnoreClear;
    Configuration.UniverseScaleVolumeByDistance = ScaleVolume;
    Configuration.UniverseFollowLocation = FollowLocation;
    Configuration.UniverseUseJumpBridges = UseJumpBridges;
    Configuration.UniverseKeywords = TextUtil::Trim(Keywords);
    Configuration.UniverseKeywordSoundPath = KeywordSoundPath;
}

void UniverseTab::SetHistory(const std::vector<IntelHistoryEntry>* const NewHistory)
{
    History = NewHistory;
}

void UniverseTab::SetSystem(const std::string& Name)
{
    if (TextUtil::EqualsIgnoreCase(Name, SystemName) == true)
    {
        return;
    }

    CopyText(SystemName, sizeof(SystemName), Name);
    SettingsChanged.Emit();
}

void UniverseTab::DrawHistory()
{
    Widgets::BeginCard("##UniverseHistory");

    const bool IsEmpty = History == nullptr || History->empty() == true;
    if (IsEmpty == true)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
        ImGui::TextWrapped("Systems reported in your intel channel appear here once the map is showing.");
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
        ImGui::PopStyleColor();
        Widgets::EndCard();
        return;
    }

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
    if (ImGui::Button("Clear list") == true)
    {
        HistoryClearRequested.Emit();
    }

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(4.0f)));
    if (ImGui::BeginChild("##HistoryList", ImVec2(0.0f, Theme::Px(HISTORY_HEIGHT)), ImGuiChildFlags_None) == true)
    {
        for (const IntelHistoryEntry& Entry : *History)
        {
            const std::string Distance = Entry.Jumps == 0 ? "home" : std::to_string(Entry.Jumps) + (Entry.Jumps == 1 ? " jump" : " jumps");
            const std::string Line = Entry.Time + "  " + Entry.System + " (" + Distance + ")  " + Entry.Sender + ": " + Entry.Text;
            ImGui::PushStyleColor(ImGuiCol_Text, Entry.Priority == true ? Theme::ACCENT : ImGui::GetStyleColorVec4(ImGuiCol_Text));
            ImGui::TextWrapped("%s", Line.c_str());
            ImGui::PopStyleColor();
        }
    }

    ImGui::EndChild();
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
    Widgets::EndCard();
}

void UniverseTab::AddSavedChannel(const std::string& Channel)
{
    const std::string Trimmed = TextUtil::Trim(Channel);
    if (Trimmed.empty() == true)
    {
        return;
    }

    for (const std::string& Saved : SavedChannels)
    {
        if (TextUtil::EqualsIgnoreCase(Saved, Trimmed) == true)
        {
            return;
        }
    }

    SavedChannels.push_back(Trimmed);
    SettingsChanged.Emit();
}

void UniverseTab::NotifyMapVisible(const bool Visible)
{
    if (MapVisible == Visible)
    {
        return;
    }

    MapVisible = Visible;
    SettingsChanged.Emit();
}

void UniverseTab::CopyText(char* const Destination, const size_t DestinationSize, const std::string& Text)
{
    strncpy_s(Destination, DestinationSize, Text.c_str(), _TRUNCATE);
}

void UniverseTab::RebuildFilter()
{
    const std::string Needle = TextUtil::ToLower(TextUtil::Trim(Filter));
    std::vector<int> Prefixed;
    std::vector<int> Contained;
    for (size_t Index = 0; Index < LowerNames.size(); Index++)
    {
        if (SelectedRegion.empty() == false && Listing->Regions[Index] != SelectedRegion)
        {
            continue;
        }

        const size_t Position = LowerNames[Index].find(Needle);
        if (Position == std::string::npos)
        {
            continue;
        }

        (Position == 0 ? Prefixed : Contained).push_back(static_cast<int>(Index));
    }

    Matches = std::move(Prefixed);
    Matches.insert(Matches.end(), Contained.begin(), Contained.end());
    AppliedFilter = Filter;
    FilterApplied = true;
}

void UniverseTab::DrawRegionRow()
{
    const float FieldWidth = Theme::Px(220.0f);
    Widgets::RowLabel("Region", FieldWidth);
    ImGui::SetNextItemWidth(FieldWidth);

    const char* const Preview = SelectedRegion.empty() == true ? "All regions" : SelectedRegion.c_str();
    if (ImGui::BeginCombo("##Region", Preview) == true)
    {
        if (Widgets::DropdownOption("All regions", SelectedRegion.empty() == true) == true)
        {
            SelectedRegion.clear();
            FilterApplied = false;
        }

        if (Listing != nullptr)
        {
            for (const std::string& Region : Listing->RegionNames)
            {
                if (Widgets::DropdownOption(Region.c_str(), Region == SelectedRegion) == true)
                {
                    SelectedRegion = Region;
                    FilterApplied = false;
                }
            }
        }

        ImGui::EndCombo();
    }

    Widgets::EndRow();
}

bool UniverseTab::DrawSystemRow()
{
    const float FieldWidth = Theme::Px(220.0f);
    Widgets::RowLabel("System", FieldWidth);
    ImGui::SetNextItemWidth(FieldWidth);

    // Combo popups default to a height of eight rows, which clips the fixed-height list below and cuts off its last row
    ImGui::SetNextWindowSizeConstraints(ImVec2(FieldWidth, 0.0f), ImVec2(FLT_MAX, Theme::Px(POPUP_MAX_HEIGHT)));

    bool Picked = false;
    float ListHeight = 0.0f;
    if (Widgets::BeginDropdownCombo("##System", SystemName, Theme::Px(LIST_HEIGHT), ListHeight) == true)
    {
        if (ImGui::IsWindowAppearing() == true)
        {
            Filter[0] = '\0';
            FilterApplied = false;
        }

        const bool Entered = Widgets::DropdownFilter("##Filter", "Type to filter...", "Type part of a system name to narrow the list. Enter picks the first match.", Filter, sizeof(Filter));

        if (FilterApplied == false || AppliedFilter != Filter)
        {
            RebuildFilter();
        }

        if (Matches.empty() == true)
        {
            ImGui::TextDisabled("No matching systems");
        }

        if (Entered == true && Matches.empty() == false)
        {
            Picked = SelectSystem(Matches.front());
            ImGui::CloseCurrentPopup();
        }

        if (Widgets::BeginDropdownList("##Systems", ListHeight) == true)
        {
            const bool HasTypedFilter = Filter[0] != '\0';
            ImGuiListClipper Clipper;
            Clipper.Begin(static_cast<int>(Matches.size()), Widgets::GetDropdownOptionHeight() + ImGui::GetStyle().ItemSpacing.y);
            while (Clipper.Step() == true)
            {
                for (int Row = Clipper.DisplayStart; Row < Clipper.DisplayEnd; Row++)
                {
                    const std::string& Name = Listing->Names[static_cast<size_t>(Matches[static_cast<size_t>(Row)])];
                    const bool IsCurrent = TextUtil::EqualsIgnoreCase(Name, SystemName);
                    const bool IsEnterTarget = Row == 0 && HasTypedFilter == true;
                    if (Widgets::DropdownOption(Name.c_str(), IsCurrent == true || IsEnterTarget == true, 0.0f, IsEnterTarget) == true)
                    {
                        Picked = SelectSystem(Matches[static_cast<size_t>(Row)]);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }

        Widgets::EndDropdownList();
        ImGui::EndCombo();
    }

    Widgets::EndRow();
    return Picked;
}

bool UniverseTab::SelectSystem(const int NameIndex)
{
    const std::string& Name = Listing->Names[static_cast<size_t>(NameIndex)];
    if (TextUtil::EqualsIgnoreCase(Name, SystemName) == true)
    {
        return false;
    }

    strcpy_s(SystemName, Name.c_str());
    return true;
}

bool UniverseTab::IsChannelWatched(const std::string& Name) const
{
    for (const std::string& Part : TextUtil::Split(IntelChannel, ','))
    {
        if (TextUtil::EqualsIgnoreCase(TextUtil::Trim(Part), Name) == true)
        {
            return true;
        }
    }

    return false;
}

void UniverseTab::ToggleChannel(const std::string& Name)
{
    std::vector<std::string> Watched;
    bool WasWatched = false;
    for (const std::string& Part : TextUtil::Split(IntelChannel, ','))
    {
        const std::string Trimmed = TextUtil::Trim(Part);
        if (Trimmed.empty() == true)
        {
            continue;
        }

        if (TextUtil::EqualsIgnoreCase(Trimmed, Name) == true)
        {
            WasWatched = true;
            continue;
        }

        Watched.push_back(Trimmed);
    }

    if (WasWatched == false)
    {
        Watched.push_back(Name);
    }

    std::string Joined;
    for (const std::string& Part : Watched)
    {
        Joined += Joined.empty() == true ? Part : ", " + Part;
    }

    CopyText(IntelChannel, sizeof(IntelChannel), Joined);
}

bool UniverseTab::DrawChannelRow()
{
    const float FieldWidth = Theme::Px(220.0f);
    Widgets::RowLabel("Intel channel", FieldWidth);
    ImGui::SetNextItemWidth(FieldWidth);
    ImGui::SetNextWindowSizeConstraints(ImVec2(FieldWidth, 0.0f), ImVec2(FLT_MAX, Theme::Px(POPUP_MAX_HEIGHT)));

    bool Changed = false;
    const char* const Preview = IntelChannel[0] == '\0' ? "Choose or type a channel" : IntelChannel;
    float ListHeight = 0.0f;
    if (Widgets::BeginDropdownCombo("##Channel", Preview, Theme::Px(LIST_HEIGHT), ListHeight) == true)
    {
        if (ImGui::IsWindowAppearing() == true)
        {
            NewChannel[0] = '\0';
            FoundChannels = ChatLogWatcher::FindChannels(ChatLogWatcher::GetDefaultDirectory(), FOUND_CHANNEL_AGE);
        }

        const bool Entered = Widgets::DropdownFilter("##NewChannel", "Type a channel name, then Enter", "Type a channel name and press Enter to use and remember it.", NewChannel, sizeof(NewChannel));
        const std::string Typed = TextUtil::Trim(NewChannel);
        if (Entered == true && Typed.empty() == false)
        {
            CopyText(IntelChannel, sizeof(IntelChannel), Typed);
            Changed = true;
            ImGui::CloseCurrentPopup();
        }

        if (Widgets::BeginDropdownList("##Channels", ListHeight, true) == true)
        {
            if (SavedChannels.empty() == true)
            {
                ImGui::TextDisabled("A channel is saved here once it reports a system on the map.");
            }

            int RemoveIndex = -1;
            const float DeleteWidth = Widgets::GetDropdownOptionHeight();
            for (size_t Index = 0; Index < SavedChannels.size(); Index++)
            {
                ImGui::PushID(static_cast<int>(Index));

                const float NameWidth = ImGui::GetContentRegionAvail().x - DeleteWidth - ImGui::GetStyle().ItemSpacing.x;
                const bool IsCurrent = TextUtil::EqualsIgnoreCase(SavedChannels[Index], IntelChannel);
                if (Widgets::DropdownOption(SavedChannels[Index].c_str(), IsCurrent, NameWidth) == true)
                {
                    CopyText(IntelChannel, sizeof(IntelChannel), SavedChannels[Index]);
                    Changed = true;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();
                const bool DeletePressed = ImGui::Button("x", ImVec2(DeleteWidth, DeleteWidth));
                Widgets::HoverTip("Forget this channel.");
                if (DeletePressed == true)
                {
                    RemoveIndex = static_cast<int>(Index);
                }

                ImGui::PopID();
            }

            if (RemoveIndex >= 0)
            {
                SavedChannels.erase(SavedChannels.begin() + RemoveIndex);
                Changed = true;
            }

            ImGui::Separator();
            ImGui::TextDisabled("Found in your chat logs (tick to watch)");
            if (FoundChannels.empty() == true)
            {
                ImGui::TextDisabled("None. Open the channel in the game first.");
            }

            for (const std::string& Found : FoundChannels)
            {
                if (TextUtil::EqualsIgnoreCase(Found, "Local") == true)
                {
                    continue;
                }

                bool Watched = IsChannelWatched(Found);
                if (ImGui::Checkbox(Found.c_str(), &Watched) == true)
                {
                    ToggleChannel(Found);
                    Changed = true;
                }
            }
        }

        Widgets::EndDropdownList();
        ImGui::EndCombo();
    }

    Widgets::EndRow();
    return Changed;
}
