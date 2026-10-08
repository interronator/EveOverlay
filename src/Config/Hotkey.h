#pragma once

#include <string>

#include <Windows.h>

// Reads and writes the System.Windows.Forms.Keys invariant text form, e.g. "Control, Shift, F5"
class Hotkey
{
public:
    bool Control = false;
    bool Shift = false;
    bool Alt = false;
    UINT VirtualKey = 0;

    bool IsNone() const;
    UINT GetModifiers() const;
    static Hotkey Parse(const std::string& Text);
    std::string ToString() const;

private:
    struct NamedKey
    {
        const char* Name;
        UINT VirtualKey;
    };

    static void AppendPart(std::string* const Result, const std::string& Part);
    static bool TryParseKey(const std::string& Token, UINT* const Key);
    static std::string GetKeyName(const UINT Key);

    static constexpr int MAXIMUM_VIRTUAL_KEY = 255;

    static const NamedKey NAMED_KEYS[];
};
