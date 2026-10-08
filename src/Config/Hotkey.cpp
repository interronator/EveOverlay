#include "Config/Hotkey.h"

#include "Config/TextUtil.h"

// The first entry per virtual key is the name written back to the config
const Hotkey::NamedKey Hotkey::NAMED_KEYS[] = {
    {"Back", VK_BACK}, {"Tab", VK_TAB}, {"Return", VK_RETURN}, {"Enter", VK_RETURN},
    {"Pause", VK_PAUSE}, {"Capital", VK_CAPITAL}, {"Escape", VK_ESCAPE}, {"Esc", VK_ESCAPE},
    {"Space", VK_SPACE}, {"PageUp", VK_PRIOR}, {"Prior", VK_PRIOR}, {"Next", VK_NEXT},
    {"PageDown", VK_NEXT}, {"End", VK_END}, {"Home", VK_HOME}, {"Left", VK_LEFT},
    {"Up", VK_UP}, {"Right", VK_RIGHT}, {"Down", VK_DOWN}, {"PrintScreen", VK_SNAPSHOT},
    {"Snapshot", VK_SNAPSHOT}, {"Insert", VK_INSERT}, {"Delete", VK_DELETE}, {"Multiply", VK_MULTIPLY},
    {"Add", VK_ADD}, {"Subtract", VK_SUBTRACT}, {"Decimal", VK_DECIMAL}, {"Divide", VK_DIVIDE},
    {"NumLock", VK_NUMLOCK}, {"Scroll", VK_SCROLL}, {"Oem1", VK_OEM_1}, {"OemSemicolon", VK_OEM_1},
    {"Oemplus", VK_OEM_PLUS}, {"Oemcomma", VK_OEM_COMMA}, {"OemMinus", VK_OEM_MINUS}, {"OemPeriod", VK_OEM_PERIOD},
    {"OemQuestion", VK_OEM_2}, {"Oem2", VK_OEM_2}, {"Oemtilde", VK_OEM_3}, {"Oem3", VK_OEM_3},
    {"OemOpenBrackets", VK_OEM_4}, {"Oem4", VK_OEM_4}, {"OemPipe", VK_OEM_5}, {"Oem5", VK_OEM_5},
    {"OemCloseBrackets", VK_OEM_6}, {"Oem6", VK_OEM_6}, {"OemQuotes", VK_OEM_7}, {"Oem7", VK_OEM_7},
};

bool Hotkey::IsNone() const
{
    return VirtualKey == 0;
}

UINT Hotkey::GetModifiers() const
{
    UINT Modifiers = 0;
    Modifiers |= Control == true ? MOD_CONTROL : 0;
    Modifiers |= Shift == true ? MOD_SHIFT : 0;
    Modifiers |= Alt == true ? MOD_ALT : 0;
    return Modifiers;
}

Hotkey Hotkey::Parse(const std::string& Text)
{
    Hotkey Result;

    std::string Normalized = Text;
    for (char& Character : Normalized)
    {
        Character = Character == '+' ? ',' : Character;
    }

    for (const std::string& Token : TextUtil::Split(Normalized, ','))
    {
        if (Token.empty() == true)
        {
            continue;
        }

        if (TextUtil::EqualsIgnoreCase(Token, "Control") == true || TextUtil::EqualsIgnoreCase(Token, "Ctrl") == true)
        {
            Result.Control = true;
            continue;
        }

        if (TextUtil::EqualsIgnoreCase(Token, "Shift") == true)
        {
            Result.Shift = true;
            continue;
        }

        if (TextUtil::EqualsIgnoreCase(Token, "Alt") == true)
        {
            Result.Alt = true;
            continue;
        }

        UINT Key = 0;
        if (TryParseKey(Token, &Key) == false)
        {
            return Hotkey();
        }

        Result.VirtualKey = Key;
    }

    return Result;
}

std::string Hotkey::ToString() const
{
    std::string Result;

    if (Control == true)
    {
        AppendPart(&Result, "Control");
    }

    if (Shift == true)
    {
        AppendPart(&Result, "Shift");
    }

    if (Alt == true)
    {
        AppendPart(&Result, "Alt");
    }

    AppendPart(&Result, VirtualKey == 0 ? "None" : GetKeyName(VirtualKey));
    return Result;
}

void Hotkey::AppendPart(std::string* const Result, const std::string& Part)
{
    if (Result->empty() == false)
    {
        *Result += ", ";
    }

    *Result += Part;
}

bool Hotkey::TryParseKey(const std::string& Token, UINT* const Key)
{
    if (Token.size() == 1 && Token[0] >= 'A' && Token[0] <= 'Z')
    {
        *Key = static_cast<UINT>(Token[0]);
        return true;
    }

    if (Token.size() == 1 && Token[0] >= 'a' && Token[0] <= 'z')
    {
        *Key = static_cast<UINT>(Token[0] - 'a' + 'A');
        return true;
    }

    if (Token.size() == 2 && (Token[0] == 'D' || Token[0] == 'd') && Token[1] >= '0' && Token[1] <= '9')
    {
        *Key = static_cast<UINT>(Token[1]);
        return true;
    }

    if (Token.size() >= 2 && (Token[0] == 'F' || Token[0] == 'f'))
    {
        int Number = 0;
        if (TextUtil::TryParseInt(Token.substr(1), &Number) == true && Number >= 1 && Number <= 24)
        {
            *Key = static_cast<UINT>(VK_F1 + Number - 1);
            return true;
        }
    }

    if (Token.size() == 7 && TextUtil::EqualsIgnoreCase(Token.substr(0, 6), "NumPad") == true && Token[6] >= '0' && Token[6] <= '9')
    {
        *Key = static_cast<UINT>(VK_NUMPAD0 + (Token[6] - '0'));
        return true;
    }

    for (const NamedKey& Named : NAMED_KEYS)
    {
        if (TextUtil::EqualsIgnoreCase(Token, Named.Name) == false)
        {
            continue;
        }

        *Key = Named.VirtualKey;
        return true;
    }

    // GetKeyName writes keys without a name as their virtual-key number
    int Number = 0;
    if (TextUtil::TryParseInt(Token, &Number) == true && Number > 0 && Number < MAXIMUM_VIRTUAL_KEY)
    {
        *Key = static_cast<UINT>(Number);
        return true;
    }

    return false;
}

std::string Hotkey::GetKeyName(const UINT Key)
{
    if (Key >= 'A' && Key <= 'Z')
    {
        return std::string(1, static_cast<char>(Key));
    }

    if (Key >= '0' && Key <= '9')
    {
        return std::string("D") + static_cast<char>(Key);
    }

    if (Key >= VK_F1 && Key <= VK_F24)
    {
        return "F" + std::to_string(Key - VK_F1 + 1);
    }

    if (Key >= VK_NUMPAD0 && Key <= VK_NUMPAD9)
    {
        return "NumPad" + std::to_string(Key - VK_NUMPAD0);
    }

    for (const NamedKey& Named : NAMED_KEYS)
    {
        if (Named.VirtualKey == Key)
        {
            return Named.Name;
        }
    }

    return std::to_string(Key);
}
