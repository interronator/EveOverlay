#include "UI/HotkeyField.h"

#include "UI/Theme.h"

ImGuiID HotkeyField::Capturing = 0;
int HotkeyField::CaptureStartFrame = 0;

bool HotkeyField::IsCapturing()
{
    return Capturing != 0;
}

std::string HotkeyField::Describe(const Hotkey& Value)
{
    if (Value.IsNone() == true)
    {
        return "None";
    }

    std::string Text = Value.ToString();
    std::string Result;
    for (size_t Index = 0; Index < Text.size(); Index++)
    {
        if (Text[Index] == ',' && Index + 1 < Text.size() && Text[Index + 1] == ' ')
        {
            Result += " + ";
            Index++;
            continue;
        }

        Result += Text[Index];
    }

    return Result;
}

bool HotkeyField::IsModifierKey(const UINT VirtualKey)
{
    return VirtualKey == VK_SHIFT || VirtualKey == VK_CONTROL || VirtualKey == VK_MENU || VirtualKey == VK_LSHIFT || VirtualKey == VK_RSHIFT
        || VirtualKey == VK_LCONTROL || VirtualKey == VK_RCONTROL || VirtualKey == VK_LMENU || VirtualKey == VK_RMENU || VirtualKey == VK_LWIN || VirtualKey == VK_RWIN;
}

bool HotkeyField::IsPressed(const UINT VirtualKey)
{
    return (::GetAsyncKeyState(static_cast<int>(VirtualKey)) & 0x8000) != 0;
}

bool HotkeyField::Draw(const char* const Id, Hotkey& Value, const float Width)
{
    const ImGuiID FieldId = ImGui::GetID(Id);
    const bool Listening = Capturing == FieldId;
    const std::string Label = Listening == true ? std::string("Press keys...") : Describe(Value);

    if (Listening == true)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::WithAlpha(Theme::ACCENT, 0.45f));
    }

    const bool Clicked = ImGui::Button((Label + "###" + Id).c_str(), ImVec2(Width, 0.0f));
    if (Listening == true)
    {
        ImGui::PopStyleColor();
    }

    if (Clicked == true && Listening == false)
    {
        Capturing = FieldId;
        CaptureStartFrame = ImGui::GetFrameCount();
        return false;
    }

    if (Listening == false || ImGui::GetFrameCount() == CaptureStartFrame)
    {
        return false;
    }

    if (Clicked == true || (ImGui::IsMouseClicked(ImGuiMouseButton_Left) == true && ImGui::IsItemHovered() == false))
    {
        Capturing = 0;
        return false;
    }

    if (IsPressed(VK_ESCAPE) == true)
    {
        Capturing = 0;
        return false;
    }

    if (IsPressed(VK_BACK) == true || IsPressed(VK_DELETE) == true)
    {
        Capturing = 0;
        const bool WasSet = Value.IsNone() == false;
        Value = Hotkey();
        return WasSet;
    }

    for (UINT VirtualKey = 0x08; VirtualKey < 0xFF; VirtualKey++)
    {
        if (IsModifierKey(VirtualKey) == true || IsPressed(VirtualKey) == false)
        {
            continue;
        }

        Hotkey Captured;
        Captured.Control = IsPressed(VK_CONTROL);
        Captured.Shift = IsPressed(VK_SHIFT);
        Captured.Alt = IsPressed(VK_MENU);
        Captured.VirtualKey = VirtualKey;
        Capturing = 0;
        Value = Captured;
        return true;
    }

    return false;
}
