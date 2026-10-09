#include "UI/CharacterPicker.h"

#include "Config/TextUtil.h"
#include "UI/Widgets.h"

bool CharacterPicker::Draw(const char* const Id, const char* const NoneLabel, const float Width, std::string& Selected, const std::vector<std::string>& Characters, std::string& Removed)
{
    ImGui::SetNextItemWidth(Width);

    const std::string Preview = Selected.empty() == true ? NoneLabel : Selected;
    if (ImGui::BeginCombo(Id, Preview.c_str()) == false)
    {
        return false;
    }

    bool Changed = false;
    if (Widgets::DropdownOption(NoneLabel, Selected.empty() == true) == true)
    {
        Selected.clear();
        Changed = true;
    }

    if (Characters.empty() == true)
    {
        ImGui::TextDisabled("Open an EVE client to add its character here");
    }

    const float DeleteWidth = Widgets::GetDropdownOptionHeight();
    for (const std::string& Character : Characters)
    {
        ImGui::PushID(Character.c_str());

        const float NameWidth = ImGui::GetContentRegionAvail().x - DeleteWidth - ImGui::GetStyle().ItemSpacing.x;
        if (Widgets::DropdownOption(Character.c_str(), TextUtil::EqualsIgnoreCase(Character, Selected) == true, NameWidth) == true)
        {
            Selected = Character;
            Changed = true;
        }

        ImGui::SameLine();
        const bool DeletePressed = ImGui::Button("x", ImVec2(DeleteWidth, DeleteWidth));
        Widgets::HoverTip("Remove this character from the list. It comes back when its client is opened again.");
        if (DeletePressed == true)
        {
            Removed = Character;
        }

        ImGui::PopID();
    }

    ImGui::EndCombo();
    return Changed;
}
