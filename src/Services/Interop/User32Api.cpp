#include "Services/Interop/User32Api.h"

HWND User32Api::GetForegroundWindowHandle()
{
    return ::GetForegroundWindow();
}

void User32Api::SetForeground(const HWND Window)
{
    ::SetForegroundWindow(Window);
}

LONG_PTR User32Api::GetWindowStyle(const HWND Window)
{
    return ::GetWindowLongPtrW(Window, GWL_STYLE);
}

void User32Api::ShowAsync(const HWND Window, const int Command)
{
    ::ShowWindowAsync(Window, Command);
}

void User32Api::SendMinimizeCommand(const HWND Window)
{
    ::SendMessageW(Window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

bool User32Api::GetPlacement(const HWND Window, WINDOWPLACEMENT* const Placement)
{
    Placement->length = sizeof(WINDOWPLACEMENT);
    return ::GetWindowPlacement(Window, Placement) != FALSE;
}

void User32Api::SetPlacement(const HWND Window, const WINDOWPLACEMENT& Placement)
{
    ::SetWindowPlacement(Window, &Placement);
}

void User32Api::Move(const HWND Window, const int Left, const int Top, const int Width, const int Height)
{
    ::MoveWindow(Window, Left, Top, Width, Height, TRUE);
}

RECT User32Api::GetWindowRectangle(const HWND Window)
{
    RECT Rectangle = {};
    ::GetWindowRect(Window, &Rectangle);
    return Rectangle;
}

RECT User32Api::GetClientRectangle(const HWND Window)
{
    RECT Rectangle = {};
    ::GetClientRect(Window, &Rectangle);
    return Rectangle;
}

bool User32Api::IsMaximized(const HWND Window)
{
    return ::IsZoomed(Window) != FALSE;
}

bool User32Api::IsMinimized(const HWND Window)
{
    return ::IsIconic(Window) != FALSE;
}

HDC User32Api::AcquireDeviceContext(const HWND Window)
{
    return ::GetDC(Window);
}

void User32Api::ReleaseDeviceContext(const HWND Window, const HDC DeviceContext)
{
    ::ReleaseDC(Window, DeviceContext);
}

bool User32Api::RegisterHotkey(const HWND Window, const int Id, const UINT Modifiers, const UINT VirtualKey)
{
    return ::RegisterHotKey(Window, Id, Modifiers, VirtualKey) != FALSE;
}

void User32Api::UnregisterHotkey(const HWND Window, const int Id)
{
    ::UnregisterHotKey(Window, Id);
}
