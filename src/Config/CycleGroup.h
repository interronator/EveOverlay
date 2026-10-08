#pragma once

#include <string>
#include <vector>

#include "Config/Hotkey.h"

// A set of clients that one pair of hotkeys steps through, in the order their previews sit on screen. An empty member list means
// every open client.
struct CycleGroup
{
    std::string Name;
    std::vector<std::wstring> Members;
    Hotkey Next;
    Hotkey Previous;
};
