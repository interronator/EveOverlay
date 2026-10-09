#include "UI/DScanTab.h"

#include "UI/Theme.h"

const std::string& DScanTab::GetTitle() const
{
    return Title;
}

const std::string& DScanTab::GetDescription() const
{
    return Description;
}

void DScanTab::SetResult(const DScanResult* const NewResult)
{
    Result = NewResult;
}

void DScanTab::SetStatus(const std::string& NewStatus)
{
    Status = NewStatus;
}

void DScanTab::Draw()
{
    bool Changed = false;

    Widgets::BeginCard("##DScanSettings");
    Changed = Widgets::ToggleRow("Read copied d-scans automatically", AutoRead) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::SliderInputRow("Show the summary for (seconds)", SecondsField, ShowSeconds, MINIMUM_SECONDS, MAXIMUM_SECONDS).Committed == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Start the scan age when a d-scan is read", MarksScanAge) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Copy the ship list to the clipboard", CopyShipList) == true || Changed == true;
    Widgets::RowDivider();

    const float ButtonWidth = Theme::Px(190.0f);
    Widgets::RowLabel("Clipboard", ButtonWidth);
    if (ImGui::Button("Read clipboard now", ImVec2(ButtonWidth, 0.0f)) == true)
    {
        ReadNowRequested.Emit();
    }

    Widgets::EndRow();
    Widgets::RowDivider();

    Widgets::RowLabel("Ship list", ButtonWidth);
    if (ImGui::Button("Copy ship list now", ImVec2(ButtonWidth, 0.0f)) == true)
    {
        CopyListRequested.Emit();
    }

    Widgets::EndRow();
    Widgets::EndCard();

    Widgets::SectionLabel("LAST SCAN");
    DrawResult();

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(10.0f)));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextWrapped("In the game's directional scanner, select the results, copy them with Ctrl+C, and a summary appears over the game. The text is only looked at on this computer, and only when it looks like a scan. Automatic reading is off until you switch it on.");
    ImGui::PopStyleColor();

    if (Changed == true)
    {
        SettingsChanged.Emit();
    }
}

void DScanTab::DrawResult()
{
    Widgets::BeginCard("##DScanResult");
    if (Result == nullptr)
    {
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::TextWrapped("%s", Status.empty() == true ? "No scan read yet." : Status.c_str());
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
        Widgets::EndCard();
        return;
    }

    const std::string Summary = std::to_string(Result->Lines) + " objects, " + std::to_string(Result->Recognized) + " recognised";
    Widgets::InfoRow("Scan", Summary.c_str());
    for (const DScanClassCount& Entry : Result->Classes)
    {
        Widgets::RowDivider();
        Widgets::InfoRow(DScanAnalyzer::GetClassLabel(Entry.Class), std::to_string(Entry.Count).c_str());
    }

    for (const DScanFlag& Flag : Result->Flags)
    {
        Widgets::RowDivider();
        ImGui::PushStyleColor(ImGuiCol_Text, Flag.Severity >= 80 ? ImVec4(1.0f, 0.45f, 0.35f, 1.0f) : ImVec4(1.0f, 0.75f, 0.35f, 1.0f));
        const std::string Name = (Flag.Count > 1 ? std::to_string(Flag.Count) + "x " : std::string()) + Flag.TypeName + " (" + Flag.Group + ")";
        const std::string Detail = Flag.Reason + ", nearest " + DScanAnalyzer::FormatDistance(Flag.NearestMeters);
        Widgets::InfoRow(Name.c_str(), Detail.c_str());
        ImGui::PopStyleColor();
    }

    Widgets::EndCard();

    if (Result->Ships.empty() == true)
    {
        return;
    }

    Widgets::SectionLabel("SHIPS ON SCAN");
    Widgets::BeginCard("##DScanShips");
    for (size_t Index = 0; Index < Result->Ships.size(); Index++)
    {
        if (Index > 0)
        {
            Widgets::RowDivider();
        }

        const DScanShipCount& Ship = Result->Ships[Index];
        const std::string Name = Ship.TypeName + " (" + Ship.Group + ")";
        Widgets::InfoRow(Name.c_str(), std::to_string(Ship.Count).c_str());
    }

    Widgets::EndCard();
}

void DScanTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    AutoRead = Configuration.DScanAutoRead;
    ShowSeconds = Configuration.DScanShowSeconds;
    MarksScanAge = Configuration.DScanMarksScanAge;
    CopyShipList = Configuration.DScanCopyShipList;
}

void DScanTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.DScanAutoRead = AutoRead;
    Configuration.DScanShowSeconds = ShowSeconds;
    Configuration.DScanMarksScanAge = MarksScanAge;
    Configuration.DScanCopyShipList = CopyShipList;
}
