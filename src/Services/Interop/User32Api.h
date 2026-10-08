#pragma once

#include <Windows.h>

class User32Api
{
public:
    static HWND GetForegroundWindowHandle();
    static void SetForeground(const HWND Window);
    static LONG_PTR GetWindowStyle(const HWND Window);
    static void ShowAsync(const HWND Window, const int Command);
    static void SendMinimizeCommand(const HWND Window);
    static bool GetPlacement(const HWND Window, WINDOWPLACEMENT* const Placement);
    static void SetPlacement(const HWND Window, const WINDOWPLACEMENT& Placement);
    static void Move(const HWND Window, const int Left, const int Top, const int Width, const int Height);
    static RECT GetWindowRectangle(const HWND Window);
    static RECT GetClientRectangle(const HWND Window);
    static bool IsMaximized(const HWND Window);
    static bool IsMinimized(const HWND Window);
    static HDC AcquireDeviceContext(const HWND Window);
    static void ReleaseDeviceContext(const HWND Window, const HDC DeviceContext);
    static bool RegisterHotkey(const HWND Window, const int Id, const UINT Modifiers, const UINT VirtualKey);
    static void UnregisterHotkey(const HWND Window, const int Id);
};
