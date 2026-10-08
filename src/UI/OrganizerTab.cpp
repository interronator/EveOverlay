#include "UI/OrganizerTab.h"

#include <algorithm>

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
}

void OrganizerTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.OrganizerEnabled = Enabled;
    Configuration.LayoutPresets = Presets;
}

void OrganizerTab::SetMoveAll(const bool NewValue)
{
    MoveAll = NewValue;
}

void OrganizerTab::SetDetectedCount(const int Count)
{
    DetectedCount = Count;
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

void OrganizerTab::DrawPlacementCard()
{
    Widgets::BeginCard("##Placement");
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
        ArrangeRequested.Emit(ThumbnailArrangement{SelectedShape, Gap, Point{OriginX, OriginY}});
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
    NewPreset.Arrangement = ThumbnailArrangement{SelectedShape, Gap, Point{OriginX, OriginY}};

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

    ArrangeRequested.Emit(Preset.Arrangement);
    LastApplied = "Applied preset \"" + Preset.Name + "\".";
}
