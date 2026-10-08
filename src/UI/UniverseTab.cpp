#include "UI/UniverseTab.h"

#include <cctype>
#include <cfloat>
#include <cstring>
#include <filesystem>

#include <Windows.h>
#include <commdlg.h>

#include "Config/TextUtil.h"
#include "Services/AlertSound.h"
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
    Changed = DrawSoundRow() == true || Changed == true;
    Widgets::EndCard();

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
    BundledSounds = AlertSound::GetBundledSounds();
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

std::string UniverseTab::SoundDisplayName(const std::filesystem::path& Sound)
{
    const std::string Stem = TextUtil::ToUtf8(Sound.stem().wstring());
    std::string Name;
    for (size_t Index = 0; Index < Stem.size(); Index++)
    {
        const bool StartsWord = Index > 0 && std::isupper(static_cast<unsigned char>(Stem[Index])) != 0 && std::islower(static_cast<unsigned char>(Stem[Index - 1])) != 0;
        if (StartsWord == true)
        {
            Name += ' ';
        }

        Name += Index == 0 ? static_cast<char>(std::toupper(static_cast<unsigned char>(Stem[Index]))) : Stem[Index];
    }

    return Name;
}

std::string UniverseTab::GetSoundLabel() const
{
    if (SoundPath.empty() == true)
    {
        return SoundDisplayName(AlertSound::GetDefaultSoundPath());
    }

    const std::filesystem::path Sound = TextUtil::FromUtf8(SoundPath);
    if (Sound.has_parent_path() == false)
    {
        return SoundDisplayName(Sound);
    }

    return "Custom: " + TextUtil::ToUtf8(Sound.filename().wstring());
}

bool UniverseTab::IsSoundSelected(const std::filesystem::path& Bundled) const
{
    const std::filesystem::path Selected = SoundPath.empty() == true ? AlertSound::GetDefaultSoundPath().filename() : std::filesystem::path(TextUtil::FromUtf8(SoundPath));
    return Selected.has_parent_path() == false && TextUtil::EqualsIgnoreCase(TextUtil::ToUtf8(Selected.wstring()), TextUtil::ToUtf8(Bundled.filename().wstring())) == true;
}

// A dropdown of the bundled sounds plus a custom file option, with a button to hear the current choice
bool UniverseTab::DrawSoundRow()
{
    const float ComboWidth = Theme::Px(190.0f);
    const float TestWidth = Theme::Px(70.0f);
    const float Gap = Theme::Px(6.0f);
    Widgets::RowLabel("Sound", ComboWidth + Gap + TestWidth);

    // SameLine after the label would otherwise pull the later controls up to the label's line instead of this one
    const float RowY = ImGui::GetCursorPosY();
    bool Changed = false;

    ImGui::SetNextItemWidth(ComboWidth);
    if (ImGui::BeginCombo("##Sound", GetSoundLabel().c_str()) == true)
    {
        if (ImGui::IsWindowAppearing() == true)
        {
            BundledSounds = AlertSound::GetBundledSounds();
        }

        for (const std::filesystem::path& Bundled : BundledSounds)
        {
            if (ImGui::Selectable(SoundDisplayName(Bundled).c_str(), IsSoundSelected(Bundled)) == true)
            {
                SoundPath = TextUtil::ToUtf8(Bundled.filename().wstring());
                Changed = true;
            }
        }

        ImGui::Separator();
        if (ImGui::Selectable("Custom file...") == true)
        {
            Changed = BrowseForSound() == true || Changed == true;
        }

        ImGui::EndCombo();
    }

    Widgets::HoverTip("Pick one of the built-in alert sounds, or choose your own wav, mp3 or wma file.");

    ImGui::SameLine(0.0f, Gap);
    ImGui::SetCursorPosY(RowY);
    const bool TestPressed = ImGui::Button("Test", ImVec2(TestWidth, 0.0f));
    Widgets::HoverTip("Play the alert sound at the current volume.");
    if (TestPressed == true)
    {
        TestSoundRequested.Emit();
    }

    Widgets::EndRow();
    return Changed;
}

bool UniverseTab::BrowseForSound()
{
    wchar_t FilePath[MAX_PATH] = {};
    OPENFILENAMEW Dialog = {};
    Dialog.lStructSize = sizeof(Dialog);
    Dialog.hwndOwner = ::GetActiveWindow();
    Dialog.lpstrFilter = L"Audio files (*.wav;*.mp3;*.wma)\0*.wav;*.mp3;*.wma\0All files (*.*)\0*.*\0";
    Dialog.lpstrFile = FilePath;
    Dialog.nMaxFile = MAX_PATH;
    Dialog.lpstrTitle = L"Choose an alert sound";
    Dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    if (::GetOpenFileNameW(&Dialog) == FALSE)
    {
        return false;
    }

    SoundPath = TextUtil::ToUtf8(FilePath);
    return true;
}

void UniverseTab::DrawRegionRow()
{
    const float FieldWidth = Theme::Px(220.0f);
    Widgets::RowLabel("Region", FieldWidth);
    ImGui::SetNextItemWidth(FieldWidth);

    const char* const Preview = SelectedRegion.empty() == true ? "All regions" : SelectedRegion.c_str();
    if (ImGui::BeginCombo("##Region", Preview) == true)
    {
        if (ImGui::Selectable("All regions", SelectedRegion.empty() == true) == true)
        {
            SelectedRegion.clear();
            FilterApplied = false;
        }

        if (Listing != nullptr)
        {
            for (const std::string& Region : Listing->RegionNames)
            {
                if (ImGui::Selectable(Region.c_str(), Region == SelectedRegion) == true)
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
    if (ImGui::BeginCombo("##System", SystemName) == true)
    {
        if (ImGui::IsWindowAppearing() == true)
        {
            Filter[0] = '\0';
            FilterApplied = false;
            ImGui::SetKeyboardFocusHere();
        }

        ImGui::SetNextItemWidth(-FLT_MIN);
        const bool Entered = ImGui::InputTextWithHint("##Filter", "Type to filter...", Filter, sizeof(Filter), ImGuiInputTextFlags_EnterReturnsTrue) == true;
        Widgets::HoverTip("Type part of a system name to narrow the list. Enter picks the first match.");

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

        if (ImGui::BeginChild("##Systems", ImVec2(0.0f, Theme::Px(LIST_HEIGHT)), ImGuiChildFlags_None) == true)
        {
            ImGuiListClipper Clipper;
            Clipper.Begin(static_cast<int>(Matches.size()));
            while (Clipper.Step() == true)
            {
                for (int Row = Clipper.DisplayStart; Row < Clipper.DisplayEnd; Row++)
                {
                    const std::string& Name = Listing->Names[static_cast<size_t>(Matches[static_cast<size_t>(Row)])];
                    const bool IsCurrent = TextUtil::EqualsIgnoreCase(Name, SystemName);
                    if (ImGui::Selectable(Name.c_str(), IsCurrent) == true)
                    {
                        Picked = SelectSystem(Matches[static_cast<size_t>(Row)]);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }

        ImGui::EndChild();
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

bool UniverseTab::DrawChannelRow()
{
    const float FieldWidth = Theme::Px(220.0f);
    Widgets::RowLabel("Intel channel", FieldWidth);
    ImGui::SetNextItemWidth(FieldWidth);

    bool Changed = false;
    const char* const Preview = IntelChannel[0] == '\0' ? "Choose or type a channel" : IntelChannel;
    if (ImGui::BeginCombo("##Channel", Preview) == true)
    {
        if (ImGui::IsWindowAppearing() == true)
        {
            NewChannel[0] = '\0';
            ImGui::SetKeyboardFocusHere();
        }

        ImGui::SetNextItemWidth(-FLT_MIN);
        const bool Entered = ImGui::InputTextWithHint("##NewChannel", "Type a channel name, then Enter", NewChannel, sizeof(NewChannel), ImGuiInputTextFlags_EnterReturnsTrue) == true;
        Widgets::HoverTip("Type a channel name and press Enter to use and remember it.");
        const std::string Typed = TextUtil::Trim(NewChannel);
        if (Entered == true && Typed.empty() == false)
        {
            CopyText(IntelChannel, sizeof(IntelChannel), Typed);
            Changed = true;
            ImGui::CloseCurrentPopup();
        }

        if (SavedChannels.empty() == true)
        {
            ImGui::TextDisabled("A channel is saved here once it reports a system on the map.");
        }

        int RemoveIndex = -1;
        const float DeleteWidth = ImGui::GetFrameHeight();
        for (size_t Index = 0; Index < SavedChannels.size(); Index++)
        {
            ImGui::PushID(static_cast<int>(Index));

            const float NameWidth = ImGui::GetContentRegionAvail().x - DeleteWidth - ImGui::GetStyle().ItemSpacing.x;
            const bool IsCurrent = TextUtil::EqualsIgnoreCase(SavedChannels[Index], IntelChannel);
            if (ImGui::Selectable(SavedChannels[Index].c_str(), IsCurrent, 0, ImVec2(NameWidth, 0.0f)) == true)
            {
                CopyText(IntelChannel, sizeof(IntelChannel), SavedChannels[Index]);
                Changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            const bool DeletePressed = ImGui::Button("x", ImVec2(DeleteWidth, 0.0f));
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

        ImGui::EndCombo();
    }

    Widgets::EndRow();
    return Changed;
}
