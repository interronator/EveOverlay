#pragma once

#include <string>

#include <imgui.h>

#include "Config/Hotkey.h"

// A button showing a hotkey. Click it and press the new combination; Escape cancels and Backspace or Delete clears it.
class HotkeyField
{
public:
    // Returns true when the hotkey changed
    static bool Draw(const char* const Id, Hotkey& Value, const float Width);

    static std::string Describe(const Hotkey& Value);

    // True while a field is waiting for keys, which the window needs to know to keep redrawing
    static bool IsCapturing();

private:
    static bool IsModifierKey(const UINT VirtualKey);
    static bool IsPressed(const UINT VirtualKey);

    // Only one field listens at a time
    static ImGuiID Capturing;
    static int CaptureStartFrame;

    // The last frame the listening field was drawn in; a field that stops being drawn (its tab was left) ends the capture
    static int LastListenFrame;
};
