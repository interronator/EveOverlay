#include "UI/ClientsTab.h"

#include "Config/TextUtil.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

const std::string& ClientsTab::GetTitle() const
{
    return Title;
}

const std::string& ClientsTab::GetDescription() const
{
    return Description;
}

void ClientsTab::Draw()
{
    if (Clients.empty() == true)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::TextUnformatted("No EVE clients detected.");
        ImGui::PopStyleColor();
        return;
    }

    Widgets::BeginCard("##Clients");
    for (size_t Index = 0; Index < Clients.size(); Index++)
    {
        if (Index > 0)
        {
            Widgets::RowDivider();
        }

        ImGui::PushID(static_cast<int>(Index));
        bool Hidden = Clients[Index].Disabled;
        const bool Toggled = Widgets::ToggleRow(Clients[Index].Label.c_str(), Hidden);
        Widgets::HoverTip("Turn on to keep this client's preview hidden.");
        if (Toggled == true)
        {
            SetThumbnailDisabled(Index, Hidden);
        }

        ImGui::PopID();
    }

    Widgets::EndCard();

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextUnformatted("Enabled switches force the preview of that client to stay hidden.");
    ImGui::PopStyleColor();
}

void ClientsTab::LoadFromConfiguration(const ThumbnailConfiguration&)
{
}

void ClientsTab::StoreToConfiguration(ThumbnailConfiguration&) const
{
}

void ClientsTab::AddThumbnail(const std::wstring& ClientTitle, const bool IsDisabled)
{
    Clients.push_back(Client{ClientTitle, TextUtil::ToUtf8(ClientTitle), IsDisabled});
}

void ClientsTab::RemoveThumbnail(const std::wstring& ClientTitle)
{
    for (std::vector<Client>::iterator Current = Clients.begin(); Current != Clients.end(); ++Current)
    {
        if (Current->Title != ClientTitle)
        {
            continue;
        }

        Clients.erase(Current);
        return;
    }
}

void ClientsTab::SetThumbnailDisabled(const size_t Index, const bool IsDisabled)
{
    if (Index >= Clients.size() || Clients[Index].Disabled == IsDisabled)
    {
        return;
    }

    Clients[Index].Disabled = IsDisabled;
    ThumbnailStateChanged.Emit(Clients[Index].Title, IsDisabled);
}

bool ClientsTab::IsThumbnailDisabled(const size_t Index) const
{
    return Index < Clients.size() && Clients[Index].Disabled == true;
}

int ClientsTab::GetEnabledThumbnailCount() const
{
    int Count = 0;
    for (const Client& Current : Clients)
    {
        if (Current.Disabled == false)
        {
            Count++;
        }
    }

    return Count;
}

int ClientsTab::GetThumbnailCount() const
{
    return static_cast<int>(Clients.size());
}
