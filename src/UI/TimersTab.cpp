#include "UI/TimersTab.h"

#include "Config/TextUtil.h"
#include "UI/Theme.h"

const std::string& TimersTab::GetTitle() const
{
    return Title;
}

const std::string& TimersTab::GetDescription() const
{
    return Description;
}

void TimersTab::SetTimers(const CountdownTimers* const NewTimers)
{
    Timers = NewTimers;
}

void TimersTab::Draw()
{
    bool Changed = false;

    Changed = DrawOverlayCard() == true || Changed == true;

    Widgets::SectionLabel("QUICK TIMER");
    Changed = DrawQuickCard() == true || Changed == true;

    Widgets::SectionLabel("START A TIMER");
    DrawCustomCard();

    Widgets::SectionLabel("RUNNING");
    DrawRunningCard();

    Widgets::SectionLabel("WHEN A TIMER RUNS OUT");
    Changed = DrawSoundCard() == true || Changed == true;

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(10.0f)));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextWrapped("The timer window floats over the game while an EVE client is open. Hold Alt and drag it to move it. The d-scan age counts up from your last scan and turns amber after 30 seconds and red after two minutes.");
    ImGui::PopStyleColor();

    if (Changed == true)
    {
        SettingsChanged.Emit();
    }
}

bool TimersTab::DrawOverlayCard()
{
    bool Changed = false;
    Widgets::BeginCard("##TimerOverlay");
    Changed = Widgets::ToggleRow("Show the timer window", WindowEnabled) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Show d-scan age", ShowScanAge) == true || Changed == true;
    Widgets::RowDivider();

    Widgets::RowLabel("Mark d-scan taken", Theme::Px(FIELD_WIDTH));
    Changed = HotkeyField::Draw("##ScanHotkey", ScanHotkey, Theme::Px(FIELD_WIDTH)) == true || Changed == true;
    Widgets::EndRow();
    Widgets::RowDivider();

    Widgets::RowLabel("Scan age", Theme::Px(FIELD_WIDTH));
    const float HalfWidth = (Theme::Px(FIELD_WIDTH) - Theme::Px(6.0f)) * 0.5f;
    if (ImGui::Button("Mark now", ImVec2(HalfWidth, 0.0f)) == true)
    {
        ScanMarkRequested.Emit();
    }

    ImGui::SameLine(0.0f, Theme::Px(6.0f));
    if (ImGui::Button("Clear", ImVec2(HalfWidth, 0.0f)) == true)
    {
        ScanClearRequested.Emit();
    }

    Widgets::EndRow();
    Widgets::EndCard();
    return Changed;
}

bool TimersTab::DrawQuickCard()
{
    bool Changed = false;
    Widgets::BeginCard("##TimerQuick");
    Widgets::RowLabel("Start quick timer", Theme::Px(FIELD_WIDTH));
    Changed = HotkeyField::Draw("##QuickHotkey", QuickHotkey, Theme::Px(FIELD_WIDTH)) == true || Changed == true;
    Widgets::EndRow();
    Widgets::RowDivider();
    Changed = Widgets::SliderInputRow("Quick timer length (seconds)", QuickField, QuickSeconds, 5, 600).Committed == true || Changed == true;
    Widgets::EndCard();
    return Changed;
}

void TimersTab::DrawCustomCard()
{
    Widgets::BeginCard("##TimerCustom");
    Widgets::RowLabel("Timer name", Theme::Px(FIELD_WIDTH));
    ImGui::SetNextItemWidth(Theme::Px(FIELD_WIDTH));
    ImGui::InputText("##CustomLabel", CustomLabel, sizeof(CustomLabel));
    Widgets::EndRow();
    Widgets::RowDivider();
    Widgets::NumberRow("Minutes", MinutesField, CustomMinutes, 0, MAXIMUM_MINUTES, 1);
    Widgets::RowDivider();
    Widgets::NumberRow("Seconds", SecondsField, CustomSeconds, 0, 59, 1);
    Widgets::RowDivider();

    Widgets::RowLabel("Start", Theme::Px(FIELD_WIDTH));
    const int TotalSeconds = CustomMinutes * 60 + CustomSeconds;
    ImGui::BeginDisabled(TotalSeconds <= 0);
    if (ImGui::Button("Start timer", ImVec2(Theme::Px(FIELD_WIDTH), 0.0f)) == true)
    {
        const std::string Label = TextUtil::Trim(CustomLabel);
        StartRequested.Emit(Label.empty() == true ? std::string("Timer") : Label, TotalSeconds);
    }

    ImGui::EndDisabled();
    Widgets::EndRow();
    Widgets::EndCard();
}

void TimersTab::DrawRunningCard()
{
    Widgets::BeginCard("##TimerRunning");
    const bool Empty = Timers == nullptr || Timers->GetTimers().empty() == true;
    if (Empty == true)
    {
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::TextUnformatted("No timers running.");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
        Widgets::EndCard();
        return;
    }

    const ULONGLONG Now = ::GetTickCount64();
    int CancelId = 0;
    bool First = true;
    for (const CountdownTimer& Timer : Timers->GetTimers())
    {
        if (First == false)
        {
            Widgets::RowDivider();
        }

        First = false;
        ImGui::PushID(Timer.Id);
        const std::string Value = Timer.Finished == true ? std::string("done") : CountdownTimers::FormatDuration(Timers->GetRemaining(Timer, Now));
        const float ButtonWidth = Theme::Px(80.0f);
        Widgets::RowLabel((Timer.Label + "  -  " + Value).c_str(), ButtonWidth);
        if (ImGui::Button("Cancel", ImVec2(ButtonWidth, 0.0f)) == true)
        {
            CancelId = Timer.Id;
        }

        Widgets::EndRow();
        ImGui::PopID();
    }

    Widgets::EndCard();
    if (CancelId != 0)
    {
        CancelRequested.Emit(CancelId);
    }
}

bool TimersTab::DrawSoundCard()
{
    bool Changed = false;
    Widgets::BeginCard("##TimerSound");
    Changed = Widgets::ToggleRow("Play a sound when a timer ends", SoundEnabled) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::SliderInputRow("Timer volume", VolumeField, Volume, 0, 100).Committed == true || Changed == true;
    Widgets::RowDivider();
    bool TestPressed = false;
    Changed = Picker.Draw("Timer sound", "##TimerSoundPicker", SoundPath, false, TestPressed) == true || Changed == true;
    if (TestPressed == true)
    {
        TestSoundRequested.Emit();
    }

    Widgets::EndCard();
    return Changed;
}

void TimersTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    WindowEnabled = Configuration.TimerWindowEnabled;
    ShowScanAge = Configuration.TimerShowScanAge;
    ScanHotkey = Hotkey::Parse(Configuration.ScanHotkey);
    QuickHotkey = Hotkey::Parse(Configuration.QuickTimerHotkey);
    QuickSeconds = Configuration.QuickTimerSeconds;
    SoundEnabled = Configuration.TimerSoundEnabled;
    Volume = Configuration.TimerVolume;
    SoundPath = Configuration.TimerSoundPath;
}

void TimersTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.TimerWindowEnabled = WindowEnabled;
    Configuration.TimerShowScanAge = ShowScanAge;
    Configuration.ScanHotkey = ScanHotkey.ToString();
    Configuration.QuickTimerHotkey = QuickHotkey.ToString();
    Configuration.QuickTimerSeconds = QuickSeconds;
    Configuration.TimerSoundEnabled = SoundEnabled;
    Configuration.TimerVolume = Volume;
    Configuration.TimerSoundPath = SoundPath;
}
