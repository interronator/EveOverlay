#include "UI/AlertsTab.h"

#include "UI/Theme.h"

const std::string& AlertsTab::GetTitle() const
{
    return Title;
}

const std::string& AlertsTab::GetDescription() const
{
    return Description;
}

void AlertsTab::Draw()
{
    bool Changed = false;

    Widgets::BeginCard("##AttackAlerts");
    Changed = Widgets::ToggleRow("Flash a preview when its character is attacked", Enabled) == true || Changed == true;
    Widgets::EndCard();

    ImGui::BeginDisabled(Enabled == false);

    Widgets::SectionLabel("WHAT COUNTS AS AN ATTACK");
    Widgets::BeginCard("##AttackKinds");
    Changed = Widgets::ToggleRow("Incoming damage", OnDamage) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Warp scramble or disruption", OnWarpDisruption) == true || Changed == true;
    Widgets::EndCard();

    Widgets::SectionLabel("HOW YOU ARE TOLD");
    Widgets::BeginCard("##AttackSound");
    Changed = Widgets::SliderInputRow("Flash duration (seconds)", SecondsField, Seconds, MINIMUM_SECONDS, MAXIMUM_SECONDS).Committed == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Play a sound for attacks", SoundEnabled) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::SliderInputRow("Attack volume", VolumeField, Volume, 0, 100).Committed == true || Changed == true;
    Widgets::RowDivider();
    bool TestPressed = false;
    Changed = Picker.Draw("Attack sound", "##AttackSoundPicker", SoundPath, false, TestPressed) == true || Changed == true;
    if (TestPressed == true)
    {
        TestSoundRequested.Emit();
    }

    Widgets::EndCard();

    ImGui::EndDisabled();

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(10.0f)));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextWrapped("Attacks are read from the game logs EVE writes to Documents\\EVE\\logs\\Gamelogs, so nothing needs to be installed in the game. The client you are playing is never flashed, because you can already see it. The preview border blinks red until the flash duration runs out.");
    ImGui::PopStyleColor();

    if (Changed == true)
    {
        SettingsChanged.Emit();
    }
}

void AlertsTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    Enabled = Configuration.AttackAlertsEnabled;
    OnDamage = Configuration.AttackAlertOnDamage;
    OnWarpDisruption = Configuration.AttackAlertOnWarpDisruption;
    SoundEnabled = Configuration.AttackAlertSoundEnabled;
    Volume = Configuration.AttackAlertVolume;
    Seconds = Configuration.AttackAlertSeconds;
    SoundPath = Configuration.AttackAlertSoundPath;
}

void AlertsTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.AttackAlertsEnabled = Enabled;
    Configuration.AttackAlertOnDamage = OnDamage;
    Configuration.AttackAlertOnWarpDisruption = OnWarpDisruption;
    Configuration.AttackAlertSoundEnabled = SoundEnabled;
    Configuration.AttackAlertVolume = Volume;
    Configuration.AttackAlertSeconds = Seconds;
    Configuration.AttackAlertSoundPath = SoundPath;
}
