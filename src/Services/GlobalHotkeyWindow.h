#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

#include "Config/Hotkey.h"
#include "Views/HotkeyHandler.h"

// Message-only window that owns the hotkeys not tied to one preview and runs an action when one is pressed
class GlobalHotkeyWindow : public CWindowImpl<GlobalHotkeyWindow>
{
public:
    DECLARE_WND_CLASS_EX(L"EveOverlayGlobalHotkeyWindow", 0, 0)

    BEGIN_MSG_MAP(GlobalHotkeyWindow)
        MESSAGE_HANDLER(WM_HOTKEY, OnHotkey)
    END_MSG_MAP()

    GlobalHotkeyWindow() = default;
    ~GlobalHotkeyWindow();

    // Returns false when the hotkey is empty or another program already owns it
    bool Add(const Hotkey& Key, std::function<void()> Action);

    void Clear();
    size_t GetCount() const;

private:
    struct Binding
    {
        std::unique_ptr<HotkeyHandler> Handler;
        std::function<void()> Action;
    };

    LRESULT OnHotkey(UINT, WPARAM HotkeyId, LPARAM, BOOL& Handled);

    std::vector<Binding> Bindings;
};
