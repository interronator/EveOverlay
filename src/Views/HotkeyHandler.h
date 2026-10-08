#pragma once

#include <Windows.h>

#include "Config/Hotkey.h"

class HotkeyHandler
{
public:
    HotkeyHandler(const HWND TargetWindow, const Hotkey& Key);
    HotkeyHandler(const HotkeyHandler&) = delete;
    HotkeyHandler& operator=(const HotkeyHandler&) = delete;

    ~HotkeyHandler();

    bool Register();
    void Unregister();
    bool IsRegistered() const;
    int GetId() const;

private:
    static constexpr int MAX_ID = 0xBFFF;

    static int AllocateId();

    HWND const Target;
    Hotkey const Binding;
    int const HotkeyId;
    bool Registered = false;
};
