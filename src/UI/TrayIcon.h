#pragma once

#include <string>

#include <Windows.h>
#include <shellapi.h>

class TrayIcon
{
public:
    enum class MenuCommand
    {
        None,
        Restore,
        UniverseMap,
        Exit
    };

    TrayIcon() = default;
    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    ~TrayIcon();

    bool Add(const HWND Window, const UINT CallbackMessage, const HICON Icon, const std::wstring& Tooltip);

    // The shell forgets all icons when explorer restarts
    bool Readd();

    void Remove();
    bool IsAdded() const;
    MenuCommand ShowMenu(const HWND Window, const bool MapChecked) const;

private:
    static constexpr UINT ICON_ID = 1;
    static constexpr UINT ID_TITLE = 1;
    static constexpr UINT ID_RESTORE = 2;
    static constexpr UINT ID_EXIT = 3;
    static constexpr UINT ID_UNIVERSE_MAP = 4;

    NOTIFYICONDATAW Data = {};
    bool Added = false;
};
