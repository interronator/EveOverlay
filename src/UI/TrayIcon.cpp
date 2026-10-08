#include "UI/TrayIcon.h"

TrayIcon::~TrayIcon()
{
    Remove();
}

bool TrayIcon::Add(const HWND Window, const UINT CallbackMessage, const HICON Icon, const std::wstring& Tooltip)
{
    Data = {};
    Data.cbSize = sizeof(Data);
    Data.hWnd = Window;
    Data.uID = ICON_ID;
    Data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    Data.uCallbackMessage = CallbackMessage;
    Data.hIcon = Icon;
    ::wcsncpy_s(Data.szTip, Tooltip.c_str(), _TRUNCATE);

    Added = ::Shell_NotifyIconW(NIM_ADD, &Data) != FALSE;
    return Added;
}

bool TrayIcon::Readd()
{
    if (Data.hWnd == nullptr)
    {
        return false;
    }

    Added = ::Shell_NotifyIconW(NIM_ADD, &Data) != FALSE;
    return Added;
}

void TrayIcon::Remove()
{
    if (Added == false)
    {
        return;
    }

    ::Shell_NotifyIconW(NIM_DELETE, &Data);
    Added = false;
}

bool TrayIcon::IsAdded() const
{
    return Added;
}

TrayIcon::MenuCommand TrayIcon::ShowMenu(const HWND Window, const bool MapChecked) const
{
    const HMENU Menu = ::CreatePopupMenu();
    if (Menu == nullptr)
    {
        return MenuCommand::None;
    }

    ::AppendMenuW(Menu, MF_STRING | MF_DISABLED, ID_TITLE, L"Eve Overlay");
    ::SetMenuDefaultItem(Menu, ID_TITLE, FALSE);
    ::AppendMenuW(Menu, MF_STRING, ID_RESTORE, L"Restore");
    ::AppendMenuW(Menu, MF_STRING | (MapChecked == true ? MF_CHECKED : MF_UNCHECKED), ID_UNIVERSE_MAP, L"Intel Watcher");
    ::AppendMenuW(Menu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(Menu, MF_STRING, ID_EXIT, L"Exit");

    POINT Cursor = {};
    ::GetCursorPos(&Cursor);

    // Without this the menu does not close when the user clicks elsewhere
    ::SetForegroundWindow(Window);
    const UINT Selected = static_cast<UINT>(::TrackPopupMenu(Menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, Cursor.x, Cursor.y, 0, Window, nullptr));
    ::PostMessageW(Window, WM_NULL, 0, 0);
    ::DestroyMenu(Menu);

    if (Selected == ID_RESTORE)
    {
        return MenuCommand::Restore;
    }

    if (Selected == ID_UNIVERSE_MAP)
    {
        return MenuCommand::UniverseMap;
    }

    if (Selected == ID_EXIT)
    {
        return MenuCommand::Exit;
    }

    return MenuCommand::None;
}
