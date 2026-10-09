#include "UI/OrganizerTab.h"

#include <algorithm>
#include <set>

#include "Config/TextUtil.h"
#include "UI/Theme.h"

const std::string& OrganizerTab::GetTitle() const
{
    return Title;
}

const std::string& OrganizerTab::GetDescription() const
{
    return Description;
}

void OrganizerTab::Draw()
{
    Widgets::BeginCard("##Enabled");
    if (Widgets::ToggleRow("Enable Thumbnail Organizer", Enabled) == true)
    {
        SettingsChanged.Emit();
    }

    Widgets::RowDivider();
    if (Widgets::ToggleRow("Move all thumbnails together", MoveAll) == true)
    {
        MoveAllChanged.Emit(MoveAll);
    }

    Widgets::EndCard();

    if (Enabled == false)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::TextUnformatted("Turn this on to arrange your previews into grids and save layout presets.");
        ImGui::PopStyleColor();
        return;
    }

    DrawCountCard();

    const int Count = GetClientCount();
    Widgets::SectionLabel("LAYOUT");
    DrawShapeCards(Count);

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
    Widgets::SectionLabel("WHO GOES WHERE");
    DrawSlotCard(Count);

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
    Widgets::SectionLabel("PLACEMENT");
    DrawPlacementCard();

    DrawApplyButton();

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(10.0f)));
    Widgets::SectionLabel("PRESETS");
    DrawPresets(Count);
}

void OrganizerTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    Enabled = Configuration.OrganizerEnabled;
    MoveAll = Configuration.MoveAllThumbnails;
    Presets = Configuration.LayoutPresets;
    SmartStart = Configuration.OrganizerSmartStart;
    SlotCharacters = Configuration.OrganizerSlots;
    Characters = Configuration.GetSelectableCharacters();
}

void OrganizerTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.OrganizerEnabled = Enabled;
    Configuration.LayoutPresets = Presets;
    Configuration.OrganizerSmartStart = SmartStart;
    Configuration.OrganizerSlots = SlotCharacters;
}

void OrganizerTab::SetMoveAll(const bool NewValue)
{
    MoveAll = NewValue;
}

void OrganizerTab::SetDetectedCount(const int Count)
{
    DetectedCount = Count;
}

void OrganizerTab::SetCharacters(const std::vector<std::string>& NewCharacters)
{
    Characters = NewCharacters;
}

void OrganizerTab::AddOpenClient(const std::wstring& ClientTitle)
{
    const std::string Name = ThumbnailConfiguration::GetCharacterName(ClientTitle);
    if (Name.empty() == true || std::find(OpenCharacters.begin(), OpenCharacters.end(), Name) != OpenCharacters.end())
    {
        return;
    }

    OpenCharacters.push_back(Name);
}

void OrganizerTab::RemoveOpenClient(const std::wstring& ClientTitle)
{
    const std::string Name = ThumbnailConfiguration::GetCharacterName(ClientTitle);
    OpenCharacters.erase(std::remove(OpenCharacters.begin(), OpenCharacters.end(), Name), OpenCharacters.end());
}

int OrganizerTab::GetClientCount() const
{
    return UsesManualCount() == true ? ManualCount : DetectedCount;
}

bool OrganizerTab::UsesManualCount() const
{
    return AutoDetect == false || DetectedCount < 1;
}

void OrganizerTab::DrawCountCard()
{
    Widgets::BeginCard("##Count");

    if (Widgets::ToggleRow("Detect open clients", AutoDetect) == true && AutoDetect == false && DetectedCount >= 1)
    {
        ManualCount = std::clamp(DetectedCount, MINIMUM_CLIENTS, MAXIMUM_CLIENTS);
    }

    Widgets::RowDivider();

    if (UsesManualCount() == true)
    {
        Widgets::NumberRow("Number of clients", CountField, ManualCount, MINIMUM_CLIENTS, MAXIMUM_CLIENTS, 1);
    }
    else
    {
        std::string Summary = std::to_string(DetectedCount) + (DetectedCount == 1 ? " client open" : " clients open");
        Widgets::InfoRow("Clients", Summary.c_str());
    }

    Widgets::EndCard();

    if (AutoDetect == true && DetectedCount < 1)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::TextUnformatted("No EVE clients are open, so you can plan a layout for any number of clients.");
        ImGui::PopStyleColor();
    }
}

void OrganizerTab::DrawShapeCards(const int Count)
{
    const std::vector<GridShape> Shapes = ThumbnailArranger::GetShapes(Count);
    if (std::find(Shapes.begin(), Shapes.end(), SelectedShape) == Shapes.end())
    {
        SelectedShape = ThumbnailArranger::GetDefaultShape(Count);
    }

    const float Spacing = ImGui::GetStyle().ItemSpacing.x;
    const float CardWidth = Widgets::GetLayoutCardWidth();
    const int PerRow = std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x + Spacing) / (CardWidth + Spacing)));

    for (size_t Index = 0; Index < Shapes.size(); Index++)
    {
        if (Index % static_cast<size_t>(PerRow) != 0)
        {
            ImGui::SameLine();
        }

        const std::string CardId = std::to_string(Shapes[Index].Columns) + "x" + std::to_string(Shapes[Index].Rows) + (Shapes[Index].PartialRowFirst == true ? "t" : "");
        if (Widgets::LayoutCard(CardId.c_str(), Shapes[Index], Count, Shapes[Index] == SelectedShape) == true)
        {
            SelectedShape = Shapes[Index];
        }
    }
}

void OrganizerTab::DrawSlotCard(const int Count)
{
    if (SlotCharacters.size() < static_cast<size_t>(Count))
    {
        SlotCharacters.resize(static_cast<size_t>(Count));
    }

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextWrapped("Pick which character takes each spot. Spots left on Auto go to whichever client is closest. Press Apply layout to use it.");
    ImGui::PopStyleColor();

    const float Spacing = Theme::Px(SLOT_SPACING);
    const int Columns = std::max(1, SelectedShape.Columns);
    const float CellWidth = std::clamp((ImGui::GetContentRegionAvail().x - Spacing * static_cast<float>(Columns - 1)) / static_cast<float>(Columns), Theme::Px(SLOT_MINIMUM_WIDTH), Theme::Px(SLOT_MAXIMUM_WIDTH));
    const float CellHeight = ImGui::GetFrameHeight() * 1.5f;
    const ImVec2 GridStart = ImGui::GetCursorPos();

    int LastRow = 0;
    for (int Index = 0; Index < Count; Index++)
    {
        const ThumbnailArranger::CellPosition Position = ThumbnailArranger::GetCellPosition(Index, SelectedShape, Count);
        LastRow = std::max(LastRow, Position.Row);
        ImGui::SetCursorPos(ImVec2(GridStart.x + Position.Column * (CellWidth + Spacing), GridStart.y + static_cast<float>(Position.Row) * (CellHeight + Spacing)));

        const std::string& Assigned = SlotCharacters[static_cast<size_t>(Index)];
        ImGui::PushID(Index);
        if (Assigned.empty() == true)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        }

        const std::string Label = std::to_string(Index + 1) + "  " + (Assigned.empty() == true ? "Auto" : Assigned) + "##Slot";
        const bool Pressed = ImGui::Button(Label.c_str(), ImVec2(CellWidth, CellHeight));
        if (Assigned.empty() == true)
        {
            ImGui::PopStyleColor();
        }

        Widgets::HoverTip("Choose who goes in this spot. Spots are numbered left to right, top to bottom.");
        if (Pressed == true)
        {
            ImGui::OpenPopup("##SlotChoices");
        }

        DrawSlotChoices(static_cast<size_t>(Index));
        ImGui::PopID();
    }

    ImGui::SetCursorPos(ImVec2(GridStart.x, GridStart.y + static_cast<float>(LastRow + 1) * (CellHeight + Spacing)));
    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    if (ImGui::Button("Reset all to Auto") == true)
    {
        SlotCharacters.assign(SlotCharacters.size(), std::string());
        SettingsChanged.Emit();
    }
}

size_t OrganizerTab::FindSlotOf(const std::string& Name, const size_t IgnoredSlot) const
{
    for (size_t Slot = 0; Slot < SlotCharacters.size(); Slot++)
    {
        if (Slot != IgnoredSlot && TextUtil::EqualsIgnoreCase(SlotCharacters[Slot], Name) == true)
        {
            return Slot;
        }
    }

    return SlotCharacters.size();
}

std::vector<std::string> OrganizerTab::GetSlotOptions(const size_t Slot) const
{
    if (ShowAllCharacters == true)
    {
        return Characters;
    }

    std::vector<std::string> Options = OpenCharacters;
    const std::string& Holder = SlotCharacters[Slot];
    const bool HolderListed = std::find_if(Options.begin(), Options.end(), [&Holder](const std::string& Option)
    {
        return TextUtil::EqualsIgnoreCase(Option, Holder) == true;
    }) != Options.end();
    if (Holder.empty() == false && HolderListed == false)
    {
        Options.push_back(Holder);
    }

    std::sort(Options.begin(), Options.end(), [](const std::string& Left, const std::string& Right)
    {
        return TextUtil::CompareIgnoreCase(Left, Right) < 0;
    });

    return Options;
}

void OrganizerTab::DrawSlotChoices(const size_t Slot)
{
    if (ImGui::BeginPopup("##SlotChoices") == false)
    {
        return;
    }

    ImGui::TextDisabled("Spot %d", static_cast<int>(Slot + 1));
    ImGui::Checkbox("Show all characters", &ShowAllCharacters);
    Widgets::HoverTip("Off: only characters with an open client. On: every character the app knows, so you can plan a layout ahead.");
    ImGui::Separator();

    if (Widgets::DropdownOption("Auto", SlotCharacters[Slot].empty() == true) == true)
    {
        AssignSlot(Slot, std::string());
        ImGui::CloseCurrentPopup();
    }

    const std::vector<std::string> Options = GetSlotOptions(Slot);
    if (Options.empty() == true)
    {
        ImGui::TextDisabled("No clients are open. Tick Show all characters to plan ahead.");
    }

    for (const std::string& Character : Options)
    {
        std::string Label = Character;
        const size_t HeldBy = FindSlotOf(Character, Slot);
        if (HeldBy < SlotCharacters.size())
        {
            Label += "  (spot " + std::to_string(HeldBy + 1) + ")";
        }

        ImGui::PushID(Character.c_str());
        if (Widgets::DropdownOption(Label.c_str(), TextUtil::EqualsIgnoreCase(SlotCharacters[Slot], Character) == true) == true)
        {
            AssignSlot(Slot, Character);
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopID();
    }

    ImGui::EndPopup();
}

// A character can only be in one spot, so picking one that is already placed swaps the two
void OrganizerTab::AssignSlot(const size_t Slot, const std::string& Name)
{
    if (Name.empty() == false)
    {
        const size_t HeldBy = FindSlotOf(Name, Slot);
        if (HeldBy < SlotCharacters.size())
        {
            SlotCharacters[HeldBy] = SlotCharacters[Slot];
        }
    }

    SlotCharacters[Slot] = Name;
    SettingsChanged.Emit();
}

void OrganizerTab::DrawPlacementCard()
{
    Widgets::BeginCard("##Placement");
    if (Widgets::ToggleRow("Start where my previews are", SmartStart) == true)
    {
        SettingsChanged.Emit();
    }

    Widgets::RowDivider();
    Widgets::NumberRow("Spacing between previews", GapField, Gap, 0, MAXIMUM_GAP, 2);
    Widgets::RowDivider();
    Widgets::NumberRow("Start X", OriginXField, OriginX, MINIMUM_COORDINATE, MAXIMUM_COORDINATE, 10);
    Widgets::RowDivider();
    Widgets::NumberRow("Start Y", OriginYField, OriginY, MINIMUM_COORDINATE, MAXIMUM_COORDINATE, 10);
    Widgets::EndCard();
}

void OrganizerTab::DrawApplyButton()
{
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::WithAlpha(Theme::ACCENT, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::ACCENT);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::Shade(Theme::ACCENT, -0.08f));
    ImGui::BeginDisabled(DetectedCount < 1);
    const bool Pressed = ImGui::Button("Apply layout", ImVec2(Theme::Px(140.0f), 0.0f));
    Widgets::HoverTip("Move the open previews into the selected layout. Needs at least one open client.");
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);

    if (Pressed == true)
    {
        ArrangeRequested.Emit(ThumbnailArrangement{SelectedShape, Gap, Point{OriginX, OriginY}, SlotCharacters, SmartStart});
        LastApplied = "Applied " + std::to_string(SelectedShape.Columns) + " x " + std::to_string(SelectedShape.Rows) + " layout.";
    }

    if (LastApplied.empty() == true)
    {
        return;
    }

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextUnformatted(LastApplied.c_str());
    ImGui::PopStyleColor();
}

void OrganizerTab::DrawPresets(const int Count)
{
    ImGui::SetNextItemWidth(Theme::Px(220.0f));
    ImGui::InputTextWithHint("##PresetName", "Preset name", PresetName, sizeof(PresetName));
    Widgets::HoverTip("Name for the preset. Saving with an existing name replaces that preset.");
    ImGui::SameLine();

    const bool CanSave = Count >= 1 && PresetName[0] != '\0';
    ImGui::BeginDisabled(CanSave == false);
    const bool SavePressed = ImGui::Button("Save current as preset");
    Widgets::HoverTip("Store the client count, layout, spacing and start position under this name.");
    if (SavePressed == true)
    {
        SavePreset(Count);
    }

    ImGui::EndDisabled();

    if (Presets.empty() == true)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::TextUnformatted("Presets remember a client count, layout and placement, so you can apply them without planning again.");
        ImGui::PopStyleColor();
        return;
    }

    Widgets::BeginCard("##Presets");
    int DeleteIndex = -1;
    for (size_t Index = 0; Index < Presets.size(); Index++)
    {
        if (Index > 0)
        {
            Widgets::RowDivider();
        }

        const LayoutPreset& Preset = Presets[Index];
        const std::string Label = Preset.Name + "  -  " + std::to_string(Preset.ClientCount) + (Preset.ClientCount == 1 ? " client, " : " clients, ")
            + std::to_string(Preset.Arrangement.Shape.Columns) + " x " + std::to_string(Preset.Arrangement.Shape.Rows);
        const float ButtonWidth = Theme::Px(64.0f);

        ImGui::PushID(static_cast<int>(Index));
        Widgets::RowLabel(Label.c_str(), ButtonWidth * 2.0f + Theme::Px(8.0f));
        const bool ApplyPressed = ImGui::Button("Apply", ImVec2(ButtonWidth, 0.0f));
        Widgets::HoverTip("Load this preset into the controls above and arrange any open previews with it.");
        if (ApplyPressed == true)
        {
            ApplyPreset(Index);
        }

        ImGui::SameLine(0.0f, Theme::Px(8.0f));
        const bool DeletePressed = ImGui::Button("Delete", ImVec2(ButtonWidth, 0.0f));
        Widgets::HoverTip("Remove this preset.");
        if (DeletePressed == true)
        {
            DeleteIndex = static_cast<int>(Index);
        }

        Widgets::EndRow();
        ImGui::PopID();
    }

    Widgets::EndCard();

    if (DeleteIndex >= 0)
    {
        Presets.erase(Presets.begin() + DeleteIndex);
        SettingsChanged.Emit();
    }
}

void OrganizerTab::SavePreset(const int Count)
{
    LayoutPreset NewPreset;
    NewPreset.Name = PresetName;
    NewPreset.ClientCount = std::clamp(Count, MINIMUM_CLIENTS, MAXIMUM_CLIENTS);
    NewPreset.Arrangement = ThumbnailArrangement{SelectedShape, Gap, Point{OriginX, OriginY}, SlotCharacters, false};

    bool Replaced = false;
    for (LayoutPreset& Existing : Presets)
    {
        if (Existing.Name != NewPreset.Name)
        {
            continue;
        }

        Existing = NewPreset;
        Replaced = true;
        break;
    }

    if (Replaced == false)
    {
        Presets.push_back(NewPreset);
    }

    PresetName[0] = '\0';
    SettingsChanged.Emit();
}

void OrganizerTab::ApplyPreset(const size_t Index)
{
    const LayoutPreset Preset = Presets[Index];

    AutoDetect = false;
    ManualCount = Preset.ClientCount;
    SelectedShape = Preset.Arrangement.Shape;
    Gap = Preset.Arrangement.Gap;
    OriginX = Preset.Arrangement.Origin.X;
    OriginY = Preset.Arrangement.Origin.Y;
    SlotCharacters = Preset.Arrangement.SlotCharacters;

    ArrangeRequested.Emit(Preset.Arrangement);
    LastApplied = "Applied preset \"" + Preset.Name + "\".";
}
