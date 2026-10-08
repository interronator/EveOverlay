#include "UI/SoundPicker.h"

#include <cctype>

#include <Windows.h>
#include <commdlg.h>

#include <imgui.h>

#include "Config/TextUtil.h"
#include "Services/AlertSound.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

std::string SoundPicker::DisplayName(const std::filesystem::path& Sound)
{
    const std::string Stem = TextUtil::ToUtf8(Sound.stem().wstring());
    std::string Name;
    for (size_t Index = 0; Index < Stem.size(); Index++)
    {
        const bool StartsWord = Index > 0 && std::isupper(static_cast<unsigned char>(Stem[Index])) != 0 && std::islower(static_cast<unsigned char>(Stem[Index - 1])) != 0;
        if (StartsWord == true)
        {
            Name += ' ';
        }

        Name += Index == 0 ? static_cast<char>(std::toupper(static_cast<unsigned char>(Stem[Index]))) : Stem[Index];
    }

    return Name;
}

std::string SoundPicker::GetLabel(const std::string& Path, const bool IsOptional)
{
    if (Path.empty() == true)
    {
        return IsOptional == true ? "Same as alert sound" : DisplayName(AlertSound::GetDefaultSoundPath());
    }

    const std::filesystem::path Sound = TextUtil::FromUtf8(Path);
    if (Sound.has_parent_path() == false)
    {
        return DisplayName(Sound);
    }

    return "Custom: " + TextUtil::ToUtf8(Sound.filename().wstring());
}

bool SoundPicker::IsSelected(const std::string& Path, const std::filesystem::path& Bundled)
{
    const std::filesystem::path Selected = Path.empty() == true ? AlertSound::GetDefaultSoundPath().filename() : std::filesystem::path(TextUtil::FromUtf8(Path));
    return Selected.has_parent_path() == false && TextUtil::EqualsIgnoreCase(TextUtil::ToUtf8(Selected.wstring()), TextUtil::ToUtf8(Bundled.filename().wstring())) == true;
}

bool SoundPicker::Draw(const char* const Label, const char* const Id, std::string& Path, const bool IsOptional, bool& TestPressed)
{
    const float ComboWidth = Theme::Px(190.0f);
    const float TestWidth = Theme::Px(70.0f);
    const float Gap = Theme::Px(6.0f);
    Widgets::RowLabel(Label, ComboWidth + Gap + TestWidth);

    // SameLine after the label would otherwise pull the later controls up to the label's line instead of this one
    const float RowY = ImGui::GetCursorPosY();
    bool Changed = false;

    ImGui::PushID(Id);
    ImGui::SetNextItemWidth(ComboWidth);
    if (ImGui::BeginCombo("##Combo", GetLabel(Path, IsOptional).c_str()) == true)
    {
        if (ImGui::IsWindowAppearing() == true)
        {
            BundledSounds = AlertSound::GetBundledSounds();
        }

        if (IsOptional == true && Widgets::DropdownOption("Same as alert sound", Path.empty() == true) == true)
        {
            Path.clear();
            Changed = true;
        }

        for (const std::filesystem::path& Bundled : BundledSounds)
        {
            const bool Selected = (IsOptional == false || Path.empty() == false) && IsSelected(Path, Bundled) == true;
            if (Widgets::DropdownOption(DisplayName(Bundled).c_str(), Selected) == true)
            {
                Path = TextUtil::ToUtf8(Bundled.filename().wstring());
                Changed = true;
            }
        }

        ImGui::Separator();
        if (Widgets::DropdownOption("Custom file...", false) == true)
        {
            Changed = Browse(Path) == true || Changed == true;
        }

        ImGui::EndCombo();
    }

    Widgets::HoverTip("Pick one of the built-in alert sounds, or choose your own wav, mp3 or wma file.");

    ImGui::SameLine(0.0f, Gap);
    ImGui::SetCursorPosY(RowY);
    TestPressed = ImGui::Button("Test", ImVec2(TestWidth, 0.0f));
    Widgets::HoverTip("Play this sound at the current volume.");

    ImGui::PopID();
    Widgets::EndRow();
    return Changed;
}

bool SoundPicker::Browse(std::string& Path)
{
    wchar_t FilePath[MAX_PATH] = {};
    OPENFILENAMEW Dialog = {};
    Dialog.lStructSize = sizeof(Dialog);
    Dialog.hwndOwner = ::GetActiveWindow();
    Dialog.lpstrFilter = L"Audio files (*.wav;*.mp3;*.wma)\0*.wav;*.mp3;*.wma\0All files (*.*)\0*.*\0";
    Dialog.lpstrFile = FilePath;
    Dialog.nMaxFile = MAX_PATH;
    Dialog.lpstrTitle = L"Choose an alert sound";
    Dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    if (::GetOpenFileNameW(&Dialog) == FALSE)
    {
        return false;
    }

    Path = TextUtil::ToUtf8(FilePath);
    return true;
}
