#include "UI/HotkeysTab.h"

#include <algorithm>
#include <cstring>
#include <set>

#include "Config/TextUtil.h"
#include "UI/Theme.h"

const std::string& HotkeysTab::GetTitle() const
{
    return Title;
}

const std::string& HotkeysTab::GetDescription() const
{
    return Description;
}

std::string HotkeysTab::GetDisplayName(const std::wstring& ClientTitle)
{
    constexpr const wchar_t* PREFIX = L"EVE - ";
    const size_t PrefixLength = std::char_traits<wchar_t>::length(PREFIX);
    if (ClientTitle.size() > PrefixLength && ClientTitle.compare(0, PrefixLength, PREFIX) == 0)
    {
        return TextUtil::ToUtf8(ClientTitle.substr(PrefixLength));
    }

    return TextUtil::ToUtf8(ClientTitle);
}

std::vector<std::wstring> HotkeysTab::GetKnownClients() const
{
    std::set<std::wstring> Known(OpenClients.begin(), OpenClients.end());
    for (const std::pair<const std::wstring, Hotkey>& Entry : ClientHotkeys)
    {
        Known.insert(Entry.first);
    }

    for (const GroupEditor& Editor : Groups)
    {
        Known.insert(Editor.Group.Members.begin(), Editor.Group.Members.end());
    }

    std::vector<std::wstring> Clients;
    for (const std::wstring& Client : Known)
    {
        if (GetDisplayName(Client) != "EVE" && Client.empty() == false)
        {
            Clients.push_back(Client);
        }
    }

    std::sort(Clients.begin(), Clients.end(), [](const std::wstring& Left, const std::wstring& Right)
    {
        return TextUtil::CompareIgnoreCase(GetDisplayName(Left), GetDisplayName(Right)) < 0;
    });

    return Clients;
}

void HotkeysTab::AddClient(const std::wstring& ClientTitle)
{
    if (std::find(OpenClients.begin(), OpenClients.end(), ClientTitle) == OpenClients.end())
    {
        OpenClients.push_back(ClientTitle);
    }
}

void HotkeysTab::RemoveClient(const std::wstring& ClientTitle)
{
    OpenClients.erase(std::remove(OpenClients.begin(), OpenClients.end(), ClientTitle), OpenClients.end());
}

void HotkeysTab::Draw()
{
    bool Changed = false;

    Widgets::SectionLabel("EACH CLIENT");
    Changed = DrawClientHotkeys() == true || Changed == true;

    Widgets::SectionLabel("ALL CLIENTS");
    Changed = DrawGlobalHotkeys() == true || Changed == true;

    Widgets::SectionLabel("CYCLE GROUPS");
    Changed = DrawGroups() == true || Changed == true;

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextWrapped("Click a hotkey box and press the keys. Escape cancels, Backspace clears. One key press does one thing: these never send input to more than one client at a time.");
    ImGui::PopStyleColor();

    if (Changed == true)
    {
        SettingsChanged.Emit();
    }
}

bool HotkeysTab::DrawClientHotkeys()
{
    bool Changed = false;
    const std::vector<std::wstring> Clients = GetKnownClients();

    Widgets::BeginCard("##ClientHotkeys");
    if (Clients.empty() == true)
    {
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::TextUnformatted("Log in a character and it will be listed here.");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
    }

    for (size_t Index = 0; Index < Clients.size(); Index++)
    {
        if (Index > 0)
        {
            Widgets::RowDivider();
        }

        ImGui::PushID(static_cast<int>(Index));
        const std::string Name = GetDisplayName(Clients[Index]);
        Widgets::RowLabel(Name.c_str(), Theme::Px(FIELD_WIDTH));
        Hotkey& Value = ClientHotkeys[Clients[Index]];
        Changed = HotkeyField::Draw("##ClientHotkey", Value, Theme::Px(FIELD_WIDTH)) == true || Changed == true;
        Widgets::EndRow();
        ImGui::PopID();
    }

    Widgets::EndCard();
    return Changed;
}

bool HotkeysTab::DrawGlobalHotkeys()
{
    bool Changed = false;
    Widgets::BeginCard("##GlobalHotkeys");

    Widgets::RowLabel("Show or hide all previews", Theme::Px(FIELD_WIDTH));
    Changed = HotkeyField::Draw("##TogglePreviews", TogglePreviews, Theme::Px(FIELD_WIDTH)) == true || Changed == true;
    Widgets::EndRow();
    Widgets::RowDivider();

    Widgets::RowLabel("Minimize all clients", Theme::Px(FIELD_WIDTH));
    Changed = HotkeyField::Draw("##MinimizeAll", MinimizeAll, Theme::Px(FIELD_WIDTH)) == true || Changed == true;
    Widgets::EndRow();

    Widgets::EndCard();
    return Changed;
}

bool HotkeysTab::DrawGroups()
{
    bool Changed = false;
    int RemoveIndex = -1;
    for (size_t Index = 0; Index < Groups.size(); Index++)
    {
        ImGui::PushID(static_cast<int>(Index));
        bool Removed = false;
        Changed = DrawGroup(Index, Removed) == true || Changed == true;
        if (Removed == true)
        {
            RemoveIndex = static_cast<int>(Index);
        }

        ImGui::PopID();
    }

    if (RemoveIndex >= 0)
    {
        Groups.erase(Groups.begin() + RemoveIndex);
        Changed = true;
    }

    ImGui::BeginDisabled(Groups.size() >= MAXIMUM_GROUPS);
    if (ImGui::Button("Add a cycle group") == true)
    {
        GroupEditor Editor;
        const std::string Name = "Group " + std::to_string(Groups.size() + 1);
        Editor.Group.Name = Name;
        strncpy_s(Editor.NameBuffer, sizeof(Editor.NameBuffer), Name.c_str(), _TRUNCATE);
        Groups.push_back(Editor);
        Changed = true;
    }

    ImGui::EndDisabled();
    Widgets::HoverTip("A group has its own next and previous hotkeys. It steps through its clients in the order their previews sit on screen: left to right, top to bottom.");
    return Changed;
}

bool HotkeysTab::DrawGroup(const size_t Index, bool& Removed)
{
    GroupEditor& Editor = Groups[Index];
    bool Changed = false;

    Widgets::BeginCard("##Group");

    Widgets::RowLabel("Group name", Theme::Px(FIELD_WIDTH));
    ImGui::SetNextItemWidth(Theme::Px(FIELD_WIDTH));
    ImGui::InputText("##GroupName", Editor.NameBuffer, sizeof(Editor.NameBuffer));
    if (ImGui::IsItemDeactivatedAfterEdit() == true)
    {
        Editor.Group.Name = TextUtil::Trim(Editor.NameBuffer);
        Changed = true;
    }

    Widgets::EndRow();
    Widgets::RowDivider();

    Widgets::RowLabel("Next client", Theme::Px(FIELD_WIDTH));
    Changed = HotkeyField::Draw("##Next", Editor.Group.Next, Theme::Px(FIELD_WIDTH)) == true || Changed == true;
    Widgets::EndRow();
    Widgets::RowDivider();

    Widgets::RowLabel("Previous client", Theme::Px(FIELD_WIDTH));
    Changed = HotkeyField::Draw("##Previous", Editor.Group.Previous, Theme::Px(FIELD_WIDTH)) == true || Changed == true;
    Widgets::EndRow();
    Widgets::RowDivider();

    const std::string Summary = Editor.Group.Members.empty() == true ? "Clients: all open clients" : "Clients: " + std::to_string(Editor.Group.Members.size()) + " chosen";
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
    if (ImGui::TreeNodeEx((Summary + "###Members").c_str(), ImGuiTreeNodeFlags_None) == true)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::TextUnformatted("Tick none to include every open client.");
        ImGui::PopStyleColor();

        for (const std::wstring& Client : GetKnownClients())
        {
            std::vector<std::wstring>& Members = Editor.Group.Members;
            std::vector<std::wstring>::iterator Found = std::find(Members.begin(), Members.end(), Client);
            bool Included = Found != Members.end();
            if (ImGui::Checkbox(GetDisplayName(Client).c_str(), &Included) == false)
            {
                continue;
            }

            if (Included == true)
            {
                Members.push_back(Client);
            }
            else
            {
                Members.erase(Found);
            }

            Changed = true;
        }

        ImGui::TreePop();
    }

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(4.0f)));
    if (ImGui::Button("Remove this group") == true)
    {
        Removed = true;
    }

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
    Widgets::EndCard();
    return Changed;
}

void HotkeysTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    ClientHotkeys.clear();
    for (const std::wstring& ClientTitle : Configuration.GetClientHotkeyTitles())
    {
        ClientHotkeys[ClientTitle] = Configuration.GetClientHotkey(ClientTitle);
    }

    Groups.clear();
    for (const CycleGroup& Group : Configuration.CycleGroups)
    {
        GroupEditor Editor;
        Editor.Group = Group;
        strncpy_s(Editor.NameBuffer, sizeof(Editor.NameBuffer), Group.Name.c_str(), _TRUNCATE);
        Groups.push_back(std::move(Editor));
    }

    TogglePreviews = Hotkey::Parse(Configuration.TogglePreviewsHotkey);
    MinimizeAll = Hotkey::Parse(Configuration.MinimizeAllHotkey);
}

void HotkeysTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    for (const std::pair<const std::wstring, Hotkey>& Entry : ClientHotkeys)
    {
        Configuration.SetClientHotkey(Entry.first, Entry.second);
    }

    Configuration.CycleGroups.clear();
    for (const GroupEditor& Editor : Groups)
    {
        Configuration.CycleGroups.push_back(Editor.Group);
    }

    Configuration.TogglePreviewsHotkey = TogglePreviews.ToString();
    Configuration.MinimizeAllHotkey = MinimizeAll.ToString();
}
