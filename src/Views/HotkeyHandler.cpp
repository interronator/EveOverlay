#include "Views/HotkeyHandler.h"

#include "Application/Logger.h"
#include "Services/Interop/User32Api.h"

HotkeyHandler::HotkeyHandler(const HWND TargetWindow, const Hotkey& Key)
    : Target(TargetWindow)
    , Binding(Key)
    , HotkeyId(AllocateId())
{
}

HotkeyHandler::~HotkeyHandler()
{
    Unregister();
}

bool HotkeyHandler::Register()
{
    if (Registered == true || Binding.IsNone() == true)
    {
        return false;
    }

    Registered = User32Api::RegisterHotkey(Target, HotkeyId, Binding.GetModifiers() | MOD_NOREPEAT, Binding.VirtualKey);
    if (Registered == false)
    {
        Logger::Warning("Could not register hotkey " + Binding.ToString() + ", Windows error " + std::to_string(::GetLastError()));
    }

    return Registered;
}

void HotkeyHandler::Unregister()
{
    if (Registered == false)
    {
        return;
    }

    User32Api::UnregisterHotkey(Target, HotkeyId);
    Registered = false;
}

bool HotkeyHandler::IsRegistered() const
{
    return Registered;
}

int HotkeyHandler::GetId() const
{
    return HotkeyId;
}

int HotkeyHandler::AllocateId()
{
    static int NextId = 0;
    const int Id = NextId;
    NextId = (NextId + 1) & MAX_ID;
    return Id;
}
