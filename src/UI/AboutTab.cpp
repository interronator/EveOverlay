#include "UI/AboutTab.h"

#include "Config/TextUtil.h"
#include "UI/AppInfo.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

AboutTab::AboutTab()
    : Name(TextUtil::ToUtf8(AppInfo::NAME))
    , Version(TextUtil::ToUtf8(AppInfo::VERSION))
{
}

const std::string& AboutTab::GetTitle() const
{
    return Title;
}

const std::string& AboutTab::GetDescription() const
{
    return Description;
}

void AboutTab::Draw()
{
    Widgets::BeginCard("##About");
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(8.0f)));

    ImGui::PushFont(Theme::GetBoldFont(), 20.0f);
    ImGui::TextUnformatted(Name.c_str());
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT);
    ImGui::PushFont(Theme::GetBoldFont(), 20.0f);
    ImGui::TextUnformatted(Version.c_str());
    ImGui::PopFont();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
    ImGui::TextWrapped("Live previews and a fast task switcher for your EVE Online clients.");
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(10.0f)));

    Widgets::SectionLabel("INSPIRED BY EVE-O PREVIEW");
    ImGui::TextWrapped("Eve Overlay is a native C++ rewrite inspired by EVE-O Preview and is not the original project.");
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(10.0f)));

    Widgets::SectionLabel("THE PROGRAM DOES NOT");
    DrawBullet("modify the EVE Online interface");
    DrawBullet("display a modified EVE Online interface");
    DrawBullet("broadcast any keyboard or mouse events");
    DrawBullet("interact with EVE Online except for resizing it or bringing it to the foreground");
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(8.0f)));
    Widgets::EndCard();

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextUnformatted("Credit to the previous EVE-O Preview maintainer: Phrynohyas Tig-Rah");
    ImGui::PopStyleColor();
}

void AboutTab::LoadFromConfiguration(const ThumbnailConfiguration&)
{
}

void AboutTab::StoreToConfiguration(ThumbnailConfiguration&) const
{
}

void AboutTab::DrawBullet(const char* const Text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT);
    ImGui::TextUnformatted("\xE2\x80\xA2");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextWrapped("%s", Text);
}
