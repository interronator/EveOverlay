#include <cstdio>
#include <filesystem>
#include <fstream>

#include "Config/ConfigurationStorage.h"

class Checker
{
public:
    void Expect(const bool Condition, const char* const Description)
    {
        if (Condition == true)
        {
            return;
        }

        FailureCount++;
        std::printf("FAIL: %s\n", Description);
    }

    int FailureCount = 0;
};

int wmain(const int ArgumentCount, wchar_t** const Arguments)
{
    Checker Check;
    const std::filesystem::path Directory = ArgumentCount > 1 ? Arguments[1] : L".";

    ThumbnailConfiguration Defaults;
    Check.Expect(Defaults.ThumbnailSize.Width == 384 && Defaults.ThumbnailSize.Height == 216, "default size");
    Check.Expect(Defaults.ActiveClientHighlightColor.ToString() == "GreenYellow", "default colour name");

    ThumbnailConfiguration Sample;
    ConfigurationStorage SampleStorage(Sample, Directory / L"sample.json");
    SampleStorage.Load();
    Check.Expect(Sample.ShowThumbnailsAlwaysOnTop == true && Sample.ThumbnailOpacity == 0.5, "sample values");
    SampleStorage.Save();

    ThumbnailConfiguration Clamped;
    Clamped.ThumbnailRefreshPeriod = 50;
    Clamped.ThumbnailOpacity = 0.29;
    Clamped.ThumbnailZoomFactor = 99;
    Clamped.ActiveClientHighlightThickness = 0;
    Clamped.ThumbnailSize = Size{5000, 10};
    Clamped.ApplyRestrictions();
    Check.Expect(Clamped.ThumbnailRefreshPeriod == 300, "refresh min");
    Check.Expect(Clamped.ThumbnailOpacity == 0.29, "opacity keeps whole percent");
    Check.Expect(Clamped.ThumbnailZoomFactor == 10, "zoom max");
    Check.Expect(Clamped.ActiveClientHighlightThickness == 1, "thickness min");
    Check.Expect(Clamped.ThumbnailSize.Width == 960 && Clamped.ThumbnailSize.Height == 108, "size clamp");

    const Hotkey Parsed = Hotkey::Parse("Control, Shift, F5");
    Check.Expect(Parsed.Control == true && Parsed.Shift == true && Parsed.Alt == false && Parsed.VirtualKey == VK_F5, "hotkey parse");
    Check.Expect(Parsed.ToString() == "Control, Shift, F5", "hotkey round trip");
    Check.Expect(Hotkey::Parse("Alt, D1").VirtualKey == '1', "hotkey digit");
    Check.Expect(Hotkey::Parse("Alt, D1").ToString() == "Alt, D1", "hotkey digit round trip");
    Check.Expect(Hotkey::Parse("garbage").IsNone() == true, "bad hotkey is none");

    Color ParsedColor;
    Check.Expect(Color::TryParse("red", &ParsedColor) == true && ParsedColor.Red == 255 && ParsedColor.Green == 0, "colour by name");
    Check.Expect(Color::TryParse("10, 20, 30", &ParsedColor) == true && ParsedColor.ToString() == "10, 20, 30", "colour rgb");
    Check.Expect(Color::TryParse("#80FF0000", &ParsedColor) == true && ParsedColor.Alpha == 0x80, "colour argb hex");

    ThumbnailConfiguration Layouts;
    Layouts.SetThumbnailLocation(L"Alpha", L"", Point{10, 20});
    Check.Expect(Layouts.GetThumbnailLocation(L"Alpha", L"Main", Point{1, 1}).X == 10, "flat layout");
    Layouts.SetPerClientThumbnailLayoutsEnabled(true);
    Layouts.SetThumbnailLocation(L"Alpha", L"", Point{99, 99});
    Check.Expect(Layouts.GetThumbnailLocation(L"Alpha", L"", Point{1, 1}).X == 10, "per-client needs active client");
    Layouts.SetThumbnailLocation(L"Alpha", L"Main", Point{30, 40});
    Check.Expect(Layouts.GetThumbnailLocation(L"Alpha", L"Main", Point{1, 1}).Y == 40, "per-client layout");
    Check.Expect(Layouts.GetThumbnailLocation(L"Beta", L"Main", Point{7, 7}).X == 7, "default location");
    Layouts.SetPerClientThumbnailLayoutsEnabled(false);
    Check.Expect(Layouts.GetThumbnailLocation(L"Alpha", L"Main", Point{1, 1}).X == 10, "disabling clears per-client");

    ThumbnailConfiguration Full;
    Full.MinimizeToTray = true;
    Full.SetClientLayoutTrackingEnabled(true);
    Full.SetPerClientThumbnailLayoutsEnabled(true);
    Full.SetThumbnailLocation(L"Päilot", L"Main – Char", Point{3, 4});
    Full.SetClientLayout(L"Alpha", ClientLayout{1, 2, 3, 4, true});
    Full.SetClientHotkey(L"Alpha", Hotkey::Parse("Control, F2"));
    Full.ToggleThumbnail(L"Beta", true);
    Full.ActiveClientHighlightColor = Color{255, 1, 2, 3};
    Full.LayoutPresets.push_back(LayoutPreset{"Five wide", 5, ThumbnailArrangement{GridShape{5, 1, true}, 8, Point{-20, 30}, {"Mierk", "", "Päilot"}}});
    Full.OrganizerSmartStart = false;
    Full.DScanCopyShipList = false;
    Full.OrganizerSlots = {"Päilot", "", "Mierk"};
    Full.UniverseIgnoreClear = false;
    Full.UniverseScaleVolumeByDistance = false;
    Full.UniverseFollowLocation = true;
    Full.UniverseKeywords = "bubble, camp";
    Full.UniverseKeywordSoundPath = "alarm.wav";
    Full.AttackAlertsEnabled = true;
    Full.AttackAlertOnDamage = false;
    Full.AttackAlertVolume = 33;
    Full.AttackAlertSeconds = 25;
    Full.AttackAlertSoundPath = "C:\\sounds\\horn.wav";
    Full.TogglePreviewsHotkey = "Control, F9";
    Full.MinimizeAllHotkey = "Alt, F10";
    CycleGroup Mining;
    Mining.Name = "Miners";
    Mining.Members = {L"EVE - First", L"EVE - Second"};
    Mining.Next = Hotkey::Parse("F13");
    Mining.Previous = Hotkey::Parse("Shift, F13");
    Full.CycleGroups.push_back(Mining);
    CycleGroup Everyone;
    Everyone.Name = "All";
    Everyone.Next = Hotkey::Parse("F14");
    Full.CycleGroups.push_back(Everyone);
    Full.MainCharacter = "Mierk";
    Full.UniverseLocationCharacter = "Päilot";
    Full.KnownCharacters = {"Mierk", "Päilot"};
    Full.HiddenCharacters = {"Old Alt"};
    Full.CharacterNames[1] = "Zed Alt";
    const std::string SignatureBefore = Full.GetHotkeySignature();
    ConfigurationStorage FullStorage(Full, Directory / L"full.json");
    FullStorage.Save();

    ThumbnailConfiguration Reloaded;
    ConfigurationStorage ReloadedStorage(Reloaded, Directory / L"full.json");
    ReloadedStorage.Load();
    Check.Expect(Reloaded.MinimizeToTray == true, "reload bool");
    Check.Expect(Reloaded.GetThumbnailLocation(L"Päilot", L"Main – Char", Point{0, 0}).Y == 4, "reload unicode per-client layout");
    Check.Expect(Reloaded.GetClientLayout(L"Alpha").has_value() == true && Reloaded.GetClientLayout(L"Alpha")->IsMaximized == true, "reload client layout");
    Check.Expect(Reloaded.GetClientLayout(L"Missing").has_value() == false, "missing client layout");
    Check.Expect(Reloaded.GetClientHotkey(L"Alpha").VirtualKey == VK_F2, "reload hotkey");
    Check.Expect(Reloaded.IsThumbnailDisabled(L"Beta") == true && Reloaded.IsThumbnailDisabled(L"Other") == false, "reload disabled");
    Check.Expect(Reloaded.ActiveClientHighlightColor == Color{255, 1, 2, 3}, "reload colour");
    Check.Expect(Reloaded.LayoutPresets.size() == 1 && Reloaded.LayoutPresets[0].Name == "Five wide" && Reloaded.LayoutPresets[0].ClientCount == 5, "reload preset identity");
    Check.Expect(Reloaded.LayoutPresets[0].Arrangement.Shape == GridShape{5, 1, true} && Reloaded.LayoutPresets[0].Arrangement.Gap == 8 && Reloaded.LayoutPresets[0].Arrangement.Origin.X == -20 && Reloaded.LayoutPresets[0].Arrangement.Shape.PartialRowFirst == true, "reload preset arrangement");

    Check.Expect(Reloaded.LayoutPresets[0].Arrangement.SlotCharacters == std::vector<std::string>{"Mierk", "", "Päilot"} && Reloaded.LayoutPresets[0].Arrangement.SmartStart == false, "reload preset character slots");
    Check.Expect(Reloaded.OrganizerSmartStart == false && Reloaded.OrganizerSlots == std::vector<std::string>{"Päilot", "", "Mierk"}, "reload organizer smart start and character slots");
    Check.Expect(Reloaded.DScanCopyShipList == false && Defaults.DScanCopyShipList == true, "the ship list copy setting survives a save and load");
    {
        std::ofstream(Directory / L"legacy.json") << "{\"UniverseUseJumpBridges\": true, \"MinimizeToTray\": true, \"UniverseSystem\": \"Amarr\"}";
        ThumbnailConfiguration Legacy;
        ConfigurationStorage LegacyStorage(Legacy, Directory / L"legacy.json");
        LegacyStorage.Load();
        Check.Expect(Legacy.MinimizeToTray == true && Legacy.UniverseSystem == "Amarr", "a config that still has the removed jump bridge setting loads normally");
        std::error_code RemoveError;
        std::filesystem::remove(Directory / L"legacy.json", RemoveError);
    }

    Check.Expect(Defaults.TimerWindowEnabled == false && Defaults.AttackAlertsEnabled == false, "the timer window and attack alerts are off by default, like their hidden tabs");
    Check.Expect(Defaults.OrganizerSmartStart == true && Defaults.OrganizerSlots.empty() == true, "smart start is on by default with no slots chosen");

    Check.Expect(Reloaded.UniverseIgnoreClear == false && Reloaded.UniverseScaleVolumeByDistance == false && Reloaded.UniverseFollowLocation == true, "reload intel switches");
    Check.Expect(Reloaded.UniverseKeywords == "bubble, camp" && Reloaded.UniverseKeywordSoundPath == "alarm.wav", "reload intel keywords");
    Check.Expect(Defaults.UniverseIgnoreClear == true && Defaults.UniverseFollowLocation == false && Defaults.AttackAlertsEnabled == false, "intel and attack alert defaults");
    Check.Expect(Reloaded.AttackAlertsEnabled == true && Reloaded.AttackAlertOnDamage == false && Reloaded.AttackAlertOnWarpDisruption == true, "reload attack alert switches");
    Check.Expect(Reloaded.AttackAlertVolume == 33 && Reloaded.AttackAlertSeconds == 25 && Reloaded.AttackAlertSoundPath == "C:\\sounds\\horn.wav", "reload attack alert values");
    Check.Expect(Reloaded.TogglePreviewsHotkey == "Control, F9" && Reloaded.MinimizeAllHotkey == "Alt, F10", "reload global hotkeys");
    Check.Expect(Reloaded.CycleGroups.size() == 2 && Reloaded.CycleGroups[0].Name == "Miners" && Reloaded.CycleGroups[0].Members.size() == 2 && Reloaded.CycleGroups[0].Members[1] == L"EVE - Second", "reload cycle group members");
    Check.Expect(Reloaded.CycleGroups[0].Next.VirtualKey == VK_F13 && Reloaded.CycleGroups[0].Previous.Shift == true && Reloaded.CycleGroups[1].Members.empty() == true, "reload cycle group hotkeys");
    Check.Expect(Reloaded.MainCharacter == "Mierk" && Reloaded.KnownCharacters.size() == 2 && Reloaded.KnownCharacters[1] == "Päilot", "reload main character and known characters");
    Check.Expect(Reloaded.UniverseLocationCharacter == "Päilot" && Defaults.UniverseLocationCharacter.empty() == true, "reload the character used for Local");
    Check.Expect(Defaults.MainCharacter.empty() == true && Defaults.KnownCharacters.empty() == true, "no main character by default");
    Check.Expect(ThumbnailConfiguration::GetCharacterName(L"EVE - Mierk") == "Mierk" && ThumbnailConfiguration::GetCharacterName(L"EVE").empty() == true, "character name from a client title");

    ThumbnailConfiguration Remember;
    Check.Expect(Remember.RememberCharacter(L"EVE") == false && Remember.KnownCharacters.empty() == true, "login screen client is not a character");
    Check.Expect(Remember.RememberCharacter(L"EVE - Mierk") == true && Remember.RememberCharacter(L"EVE - mierk") == false, "a character is remembered once, ignoring case");
    Remember.CharacterNames[2] = "alpha";
    Remember.SetClientHotkey(L"EVE - Zulu", Hotkey::Parse("F3"));
    Remember.MainCharacter = "Mierk";
    const std::vector<std::string> Selectable = Remember.GetSelectableCharacters();
    Check.Expect(Selectable.size() == 3 && Selectable[0] == "alpha" && Selectable[1] == "Mierk" && Selectable[2] == "Zulu", "selectable characters merge every source in name order");
    Check.Expect(Reloaded.GetSelectableCharacters().size() == 3, "reloaded config offers its known and synced characters");
    Check.Expect(Reloaded.HiddenCharacters.size() == 1 && Reloaded.HiddenCharacters[0] == "Old Alt" && Reloaded.IsCharacterHidden("old alt") == true, "reload removed characters");

    ThumbnailConfiguration Forget;
    Forget.KnownCharacters = {"Alpha", "Bravo"};
    Forget.CharacterNames[1] = "Alpha";
    Forget.MainCharacter = "Bravo";
    Forget.UniverseLocationCharacter = "Bravo";
    Check.Expect(Forget.ForgetCharacter("alpha") == false && Forget.GetSelectableCharacters() == std::vector<std::string>{"Bravo"}, "a removed character leaves the list even when a synced account has it");
    Check.Expect(Forget.ForgetCharacter("Bravo") == true && Forget.MainCharacter.empty() == true && Forget.UniverseLocationCharacter.empty() == true && Forget.GetSelectableCharacters().empty() == true, "removing the chosen character clears the choice");
    Check.Expect(Forget.RememberCharacter(L"EVE - Alpha") == true && Forget.IsCharacterHidden("Alpha") == false && Forget.GetSelectableCharacters() == std::vector<std::string>{"Alpha"}, "a removed character returns when its client opens again");
    Check.Expect(Forget.HiddenCharacters.size() == 1 && Forget.HiddenCharacters[0] == "Bravo", "only the characters still removed stay hidden");
    Check.Expect(Reloaded.GetHotkeySignature() == SignatureBefore, "hotkey signature survives a save and load");
    Reloaded.CycleGroups[0].Next = Hotkey::Parse("F15");
    Check.Expect(Reloaded.GetHotkeySignature() != SignatureBefore, "hotkey signature notices a changed hotkey");

    ThumbnailConfiguration Limits;
    Limits.AttackAlertVolume = 500;
    Limits.AttackAlertSeconds = 0;
    Limits.ApplyRestrictions();
    Check.Expect(Limits.AttackAlertVolume == 100 && Limits.AttackAlertSeconds == 3, "attack alert values are limited");

    std::printf(Check.FailureCount == 0 ? "ALL PASSED\n" : "%d FAILED\n", Check.FailureCount);
    return Check.FailureCount;
}
